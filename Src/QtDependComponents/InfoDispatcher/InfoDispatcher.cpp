#include "InfoDispatcher.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

struct meta_type_register__ {
    meta_type_register__() {
        qRegisterMetaType<edm::move::MotionInfo>("edm::move::MotionInfo");
    }
};
static meta_type_register__ mtr_;

namespace edm {

InfoDispatcher::InfoDispatcher(
    move::MotionSignalQueue::ptr signal_queue,
    move::MotionThreadController::ptr motion_controller, QObject *parent,
    int info_get_time_peroid_ms)
    : QObject(parent), signal_queue_(signal_queue),
      motion_controller_(motion_controller) {
    info_get_timer_ = new QTimer(this);

    connect(info_get_timer_, &QTimer::timeout, this,
            &InfoDispatcher::_info_get_timer_slot);
    info_get_timer_->start(info_get_time_peroid_ms);
}

void InfoDispatcher::_info_get_timer_slot() {
    bool signal_handled = signal_queue_->consume_one(consumer_func_);

    // signal 会携带一份info, 所以如果处理了 signal 就不再拷贝info
    if (!signal_handled) {
        info_cache_ = motion_controller_->get_info_cache();
    }
}

void InfoDispatcher::_handle_signal_callback(const move::MotionSignal &signal) {
    // 拷贝info
    info_cache_ = signal.info;

    switch (signal.type) {
    case move::MotionSignalType::MotionSignal_ManualPointMoveStarted:
        emit manual_pointmove_started();
        break;
    case move::MotionSignalType::MotionSignal_ManualPointMoveStopped:
        emit manual_pointmove_stopped();
        break;
    default:
        s_logger->warn("recv unknown signal: {}", (int)signal.type);
        break;
    }
}

} // namespace edm