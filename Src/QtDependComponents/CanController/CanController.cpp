#include "CanController.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "Utils/Format/edm_format.h"
#include "Utils/Netif/netif_utils.h"

#include <QDebug>
#include <QVariant>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace can {

struct metatype_register__ {
    metatype_register__() { qRegisterMetaType<QCanBusFrame>("QCanBusFrame"); }
};
static struct metatype_register__ register__;

CanController::CanController(const QString &can_if_name, uint32_t bitrate) {
    can_worker_ = new CanWorker(can_if_name, bitrate);
    worker_thread_ = new QThread(this);
    can_worker_->moveToThread(worker_thread_);

    QObject::connect(this, &CanController::_sig_worker_start_work, can_worker_,
                     &CanWorker::slot_start_work, Qt::QueuedConnection);
    QObject::connect(this, &CanController::_sig_worker_send_frame, can_worker_,
                     &CanWorker::slot_send_frame, Qt::QueuedConnection);

    QObject::connect(worker_thread_, &QThread::finished, can_worker_,
                     &CanWorker::deleteLater);

    worker_thread_->start();
    emit _sig_worker_start_work();
}

CanController::~CanController() {}

void CanController::send_frame(const QCanBusFrame &frame) {
    emit _sig_worker_send_frame(frame);
}

CanWorker::CanWorker(const QString &can_if_name, uint32_t bitrate)
    : can_if_name_(can_if_name), bitrate_(bitrate) {
    can_if_name_std_ = can_if_name_.toStdString();
}

void CanWorker::slot_send_frame(const QCanBusFrame &frame) {
    device_->writeFrame(frame);
}

void CanWorker::slot_start_work() {
    if (!reconnect_timer_) {
        reconnect_timer_ = new QTimer(this);
        QObject::connect(reconnect_timer_, &QTimer::timeout, this,
                         &CanWorker::_slot_reconnect);
        reconnect_timer_->start(reconnect_timeout_);
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
    dummy = system("sudo ip link set can0 down");
    auto up_cmd = EDM_FMT::format(
        "sudo ip link set can0 up type can bitrate {}", bitrate_);
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