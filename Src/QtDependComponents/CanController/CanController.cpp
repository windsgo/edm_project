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

CanController::~CanController() {}

int CanController::add_device(const QString &name, uint32_t bitrate) {

    if (worker_map_.contains(name)) {
        s_logger->error("CanController::add_device: already exist: {}",
                        name.toStdString());
        return -1;
    }

    int index = worker_vec_.size();

    CanWorker *worker = new CanWorker(name, bitrate);
    worker->moveToThread(worker_thread_);

    QObject::connect(worker_thread_, &QThread::finished, worker,
                     &CanWorker::deleteLater);

    QCoreApplication::postEvent(worker, new CanStartEvent());

    worker_vec_.push_back({name, worker});
    worker_map_.insert(name, {index, worker});

    return index;
}

void CanController::send_frame(int index, const QCanBusFrame &frame) {
    if (index >= worker_vec_.size()) {
        return;
    }

    QCoreApplication::postEvent(worker_vec_[index].second,
                                new CanSendFrameEvent(frame));
}

void CanController::send_frame(const QString &device_name,
                               const QCanBusFrame &frame) {

    auto find_ret = worker_map_.find(device_name);

    if (find_ret == worker_map_.end()) {
        return;
    }

    QCoreApplication::postEvent((*find_ret).second,
                                new CanSendFrameEvent(frame));
}

CanWorker::CanWorker(const QString &can_if_name, uint32_t bitrate)
    : can_if_name_(can_if_name), bitrate_(bitrate) {
    can_if_name_std_ = can_if_name_.toStdString();
}

bool CanWorker::is_connected() { return connected_; }

void CanWorker::customEvent(QEvent *event) {
    QEvent::Type type = event->type();

    if (type == CanSendFrameEvent::type) {
        event->accept();
        CanSendFrameEvent *e = dynamic_cast<CanSendFrameEvent *>(event);
        if (!e) {
            s_logger->critical("CanSendFrameEvent cast failed");
            return;
        }

        if (!connected_) {
            return;
        }

        device_->writeFrame(e->get_frame());
    } else if (type == CanStartEvent::type) {
        event->accept();
        _start_work();
    }
}

void CanWorker::_start_work() {
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
                         connected_ = false;
                         //  reconnect_timer_->start(reconnect_timeout_);
                         device_->disconnectDevice();
                         s_logger->error("canbusdevice error: {}",
                                         device_->errorString().toStdString());
                     });
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
    int dummy;
    auto dowm_cmd =
        EDM_FMT::format("sudo ip link set {} down", can_if_name_std_);
    auto up_cmd = EDM_FMT::format("sudo ip link set {} up type can bitrate {}",
                                  can_if_name_std_, bitrate_);
    dummy = system(dowm_cmd.c_str());
    dummy = system(up_cmd.c_str()); // bitrate here

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
                connected_ = true;
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