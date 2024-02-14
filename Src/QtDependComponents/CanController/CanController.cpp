#include "CanController.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "Utils/Format/edm_format.h"
#include "Utils/Netif/netif_utils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QVariant>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace can {

struct metatype_register__ {
    metatype_register__() { qRegisterMetaType<QCanBusFrame>("QCanBusFrame"); }
};
static struct metatype_register__ mt_register__;

struct eventtype_register__ {
    eventtype_register__() {
        QEvent::registerEventType(CanSendFrameEvent::type);
        QEvent::registerEventType(CanStartEvent::type);
    }
};
static struct eventtype_register__ et_register;

CanController::CanController() {
    worker_thread_ = new QThread(this);

    worker_thread_->start();
}

CanController::~CanController() {
    s_logger->trace("CanController exit. func: {}", __PRETTY_FUNCTION__);

    worker_thread_->quit();
    worker_thread_->wait();

    for (auto& i : worker_vec_) {
        auto worker = i.second;
        worker->terminate(); // ! Set can down, if needed here.
    }
}

CanWorker *CanController::_get_device(int index) const {
    std::lock_guard guard(mutex_worker_map_and_vec_);
    if (index >= worker_vec_.size()) {
        return nullptr;
    }

    return worker_vec_[index].second;
}

CanWorker *CanController::_get_device(const QString &name) const {
    std::lock_guard guard(mutex_worker_map_and_vec_);
    auto find_ret = worker_map_.find(name);
    if (find_ret == worker_map_.end()) {
        return nullptr;
    }

    return find_ret->second;
}

int CanController::add_device(const QString &name, uint32_t bitrate) {
    std::lock_guard guard(mutex_worker_map_and_vec_);
    if (worker_map_.contains(name)) {
        s_logger->error("CanController::add_device: already exist: {}",
                        name.toStdString());
        return -1;
    }

    int index = worker_vec_.size();

    CanWorker *worker = new CanWorker(name, bitrate);
    worker->moveToThread(worker_thread_);

    // Qt::DirectConnection is needed for this, or worker won't be deleted
    // in fact, this won't call dtor of worker, because the controller is
    // static, and thread is finished after the main function is ended, so that
    // at that time, there is no other eventloop exist to delete worker
    QObject::connect(worker_thread_, &QThread::finished, worker,
                     &QObject::deleteLater, Qt::DirectConnection);

    QCoreApplication::postEvent(worker, new CanStartEvent());

    worker_vec_.push_back({name, worker});
    worker_map_.insert(name, {index, worker});

    return index;
}

void CanController::send_frame(int index, const QCanBusFrame &frame) {
    auto device = _get_device(index);
    if (!device)
        return;

    QCoreApplication::postEvent(device, new CanSendFrameEvent(frame));
}

void CanController::send_frame(const QString &device_name,
                               const QCanBusFrame &frame) {
    auto device = _get_device(device_name);
    if (!device)
        return;

    QCoreApplication::postEvent(device, new CanSendFrameEvent(frame));
}

bool CanController::is_connected(int index) const {
    auto device = _get_device(index);
    if (!device)
        return false;

    return device->is_connected();
}

bool CanController::is_connected(const QString &device_name) const {
    auto device = _get_device(device_name);
    if (!device)
        return false;

    return device->is_connected();
}

void CanController::add_frame_received_listener(
    int index, std::function<void(const QCanBusFrame &)> listener_cb) {
    auto device = _get_device(index);
    if (!device)
        return;

    device->add_listener(listener_cb);
}

void CanController::add_frame_received_listener(
    const QString &device_name,
    std::function<void(const QCanBusFrame &)> listener_cb) {
    auto device = _get_device(device_name);
    if (!device)
        return;

    device->add_listener(listener_cb);
}

CanWorker::CanWorker(const QString &can_if_name, uint32_t bitrate)
    : can_if_name_(can_if_name), bitrate_(bitrate) {
    can_if_name_std_ = can_if_name_.toStdString();

    can_down_shell_cmd_ =
        EDM_FMT::format("sudo ip link set {} down", can_if_name_std_);
    can_up_shell_cmd_ =
        EDM_FMT::format("sudo ip link set {} up type can bitrate {}",
                        can_if_name_std_, bitrate_);
}

CanWorker::~CanWorker() {
    s_logger->trace("{} set down.", can_if_name_std_);

    if (connected_ && device_)
        device_->disconnectDevice();
    device_->deleteLater();

    _set_can_down();
}

bool CanWorker::is_connected() const {
    std::lock_guard guard(mutex_connected_);
    return connected_;
}

void CanWorker::add_listener(
    std::function<void(const QCanBusFrame &)> listener_cb) {
    std::lock_guard guard(mutex_listener_vec_);
    listener_vec_.push_back(listener_cb);
}

void CanWorker::terminate() {
    s_logger->warn("{} terminated !", can_if_name_std_);
    
    {
        std::lock_guard guard(mutex_connected_);

        if (device_ && connected_)
            device_->disconnectDevice();

        connected_ = false;

        _set_can_down();
    }
}

void CanWorker::_slot_framesReceived() {
    while (auto frame_num = device_->framesAvailable()) {
        auto frame = device_->readFrame();
        // emit sig_frame_received(frame);

        std::lock_guard guard(mutex_listener_vec_);
        for (auto &listener : listener_vec_) {
            listener(frame);
        }
    }
}

void CanWorker::customEvent(QEvent *event) {
    QEvent::Type type = event->type();

    if (type == CanSendFrameEvent::type) {
        _process_sendframe_event(event);
        event->accept();
    } else if (type == CanStartEvent::type) {
        _process_start_event(event);
        event->accept();
    }
}

void CanWorker::_start_work() {
    std::stringstream ss;
    ss << QThread::currentThreadId();
    s_logger->info("{} start work thread: {}", can_if_name_std_, ss.str());

    if (!reconnect_timer_) {
        reconnect_timer_ = new QTimer(this);
        QObject::connect(reconnect_timer_, &QTimer::timeout, this,
                         &CanWorker::_slot_reconnect);
        reconnect_timer_->start(reconnect_timeout_);
    }

    if (device_) {
        return; // has been created
    }
    // create device
    QString err_msg;
    device_ =
        QCanBus::instance()->createDevice("socketcan", can_if_name_, &err_msg);
    if (!device_) {
        s_logger->critical("can device created failed: {}",
                           err_msg.toStdString());
        throw exception(EDM_FMT::format("can device created failed: {}",
                                        err_msg.toStdString())); // throw
    }

    // error call back
    QObject::connect(device_, &QCanBusDevice::errorOccurred, this,
                     [this](QCanBusDevice::CanBusError err) {
                         {
                             std::lock_guard guard(mutex_connected_);
                             connected_ = false;
                         }
                         //  reconnect_timer_->start(reconnect_timeout_);
                         device_->disconnectDevice();
                         s_logger->error("canbusdevice error: {}",
                                         device_->errorString().toStdString());
                     });

    // frames receive callback
    QObject::connect(device_, &QCanBusDevice::framesReceived, this,
                     &CanWorker::_slot_framesReceived);
}

void CanWorker::_process_sendframe_event(QEvent *event) {
    CanSendFrameEvent *e = dynamic_cast<CanSendFrameEvent *>(event);
    if (!e) {
        s_logger->critical("CanSendFrameEvent cast failed");
        return;
    }

    // not mutex_ lock here, since inside write and read won't be done at the
    // same time
    if (!connected_) {
        return;
    }

    device_->writeFrame(e->get_frame());
}

void CanWorker::_set_can_down() {
    std::ignore = system(can_down_shell_cmd_.c_str());
}

void CanWorker::_set_can_up_with_bitrate() {
    std::ignore = system(can_up_shell_cmd_.c_str());
}

void CanWorker::_process_start_event(QEvent *event [[maybe_unused]]) {
    _start_work();
}

void CanWorker::_slot_reconnect() {
    if (connected_) {
        // 已连接, 做检查连接工作
        auto busstatus = device_->busStatus();
        // qDebug() << busstatus;
        if (busstatus != QCanBusDevice::CanBusStatus::Good) {
            emit device_->errorOccurred(
                QCanBusDevice::CanBusError::ConnectionError);
        }

        return;
    }

    s_logger->trace("connecting can device: {}", can_if_name_std_);

    bool exist = util::is_netdev_exist(can_if_name_std_);
    if (!exist) {
        s_logger->trace("device \"{}\" does not exists", can_if_name_std_);
        return;
    }

    // set can0 up, just use shell command
    _set_can_down();
    _set_can_up_with_bitrate();

    auto available_sockeccan_devices =
        QCanBus::instance()->availableDevices("socketcan");
    if (available_sockeccan_devices.empty()) {
        s_logger->trace("no device avaiable");
        return; // no devices exist
    }

    for (const auto &d : available_sockeccan_devices) {
        if (d.name() == can_if_name_) {
            s_logger->info("found available device \"{}\" ", can_if_name_std_);

            // found, try to connect
            device_->setConfigurationParameter(QCanBusDevice::BitRateKey,
                                               QVariant()); // avoid warn
            // because we cannot set bitrate from qt
            bool connect_ret = device_->connectDevice();

            if (connect_ret) {
                // connected, stop reconnect timer
                {
                    std::lock_guard guard(mutex_connected_);
                    connected_ = true;
                }
                // reconnect_timer_->stop();
                s_logger->info("device \"{}\" connected.", can_if_name_std_);
            }
            return;
        }
    }

    s_logger->trace("no device named \"{}\" could be found", can_if_name_std_);
}

} // namespace can

} // namespace edm