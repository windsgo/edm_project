#include "CanController.h"

#include "Logger/LogMacro.h"
#include "Exception/exception.h"
#include "Utils/Format/edm_format.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace can {
CanController::CanController(const QString &can_if_name) {}

CanController::~CanController() {}

CanWorker::CanWorker(const QString &can_if_name) : can_if_name_(can_if_name) {}

void CanWorker::_scan_devices() {
    auto available_sockeccan_devices =
        QCanBus::instance()->availableDevices("socketcan");
    if (available_sockeccan_devices.empty()) {
    }
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
        s_logger->critical("can device created failed: {}", err_msg.toStdString());
        throw exception(EDM_FMT::format("can device created failed: {}", err_msg.toStdString())); // throw
    }
    
    
}

void CanWorker::_slot_device_disconnected() {
    s_logger->warn("can device disconnected.");
    connected_ = false;

    reconnect_timer_->start(reconnect_timeout_);
}

void CanWorker::_slot_reconnect() {
    s_logger->trace("connecting can device: {}", can_if_name_.toStdString());

    auto available_sockeccan_devices =
        QCanBus::instance()->availableDevices("socketcan");
    if (available_sockeccan_devices.empty()) {
        s_logger->trace("no device avaiable");
        return; // no devices exist
    }

    for (const auto &d : available_sockeccan_devices) {
        if (d.name() == can_if_name_) {
            s_logger->info("found device \"{}\", ", can_if_name_.toStdString());

            // connected, stop reconnect timer
            connected_ = true;
            reconnect_timer_->stop();

            return;
        }
    }

    s_logger->trace("no device named \"{}\" could be found",
                    can_if_name_.toStdString());
}

} // namespace can

} // namespace edm