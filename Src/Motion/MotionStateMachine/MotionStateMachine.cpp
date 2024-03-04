#include "MotionStateMachine.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "Motion/MotionUtils/MotionUtils.h"

#include <cassert>

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {
MotionStateMachine::MotionStateMachine(
    const std::function<bool(axis_t &)> &cb_get_act_axis,
    const std::function<bool(void)> &cb_touch_detected,
    SignalBuffer::ptr signal_buffer)
    : cb_get_act_axis_(cb_get_act_axis), cb_touch_detected_(cb_touch_detected),
      signal_buffer_(signal_buffer) {
    reset();

    //! use assert more than exception is better
    if (!cb_get_act_axis_) {
        throw exception("no cb_get_act_axis_");
    }

    if (!cb_touch_detected_) {
        throw exception("no cb_touch_detected_");
    }

    if (!signal_buffer_) {
        throw exception("no signal_buffer_");
    }

    // 初始化计算坐标
    // 先清0 (离线调试模式时实际坐标会直接返回当前值)
    MotionUtils::ClearAxis(cmd_axis_);
    cb_get_act_axis_(cmd_axis_);
}

void MotionStateMachine::run_once() {
    if (!enabled_) {
        return;
    }

    switch (main_mode_) {
    case MotionMainMode::Idle:
        _mainmode_idle();
        break;
    case MotionMainMode::Manual:
        _mainmode_manual();
        break;
    case MotionMainMode::Auto:
        _mainmode_auto();
        break;
    default:
        s_logger->error("{}: unknown main mode: {}", __FUNCTION__,
                        static_cast<int>(main_mode_));
        assert(false);
        enabled_ = false;
        break;
    }
}

void MotionStateMachine::reset() {
    cmd_axis_.fill(0);

    enabled_ = false;
    main_mode_ = MotionMainMode::Idle;
    auto_state_ = MotionAutoState::Stopped;

    pm_handler_.clear();
}

void MotionStateMachine::set_cmd_axis(const axis_t& init_cmd_axis) {
    s_logger->info("MotionStateMachine::set_cmd_axis.");
    cmd_axis_ = init_cmd_axis;
}

void MotionStateMachine::refresh_axis_using_actpos() {
    s_logger->info("MotionStateMachine::refresh_axis_using_actpos.");
    if (cb_get_act_axis_) {
        cb_get_act_axis_(this->cmd_axis_);
    }
}

bool MotionStateMachine::start_manual_pointmove(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &target_pos) {
    switch (main_mode_) {
    case MotionMainMode::Idle: {
        s_logger->trace("{}: idle mode.", __FUNCTION__);
        auto ret = pm_handler_.start(speed_param, cmd_axis_, target_pos);
        if (ret) {
            _mainmode_switch_to(MotionMainMode::Manual);
        }
        return ret;
    }
    case MotionMainMode::Manual:
        s_logger->warn("{}: manual mode not stopped", __FUNCTION__);
        return false;
    case MotionMainMode::Auto:
        s_logger->trace("{}: in auto mode", __FUNCTION__);
        return false;
        // TODO
        // 处理暂停点动的启动.
        break;
    default:
        return false;
    }
}

bool MotionStateMachine::stop_manual_pointmove(bool immediate) {
    switch (main_mode_) {
    case MotionMainMode::Idle:
        s_logger->warn("{}: idle mode.", __FUNCTION__);
        return false;
    case MotionMainMode::Manual:
        s_logger->trace("{}: in manual mode, immediate = {}", __FUNCTION__,
                        immediate);
        return pm_handler_.stop(immediate);
    case MotionMainMode::Auto:
        s_logger->trace("{}: in auto mode, immediate = {}", __FUNCTION__,
                        immediate);
        return false;
        // TODO
        // 处理暂停点动.
        break;
    default:
        return false;
    }
}

void MotionStateMachine::_mainmode_idle() {
    // TODO
}

void MotionStateMachine::_mainmode_manual() {
    if (pm_handler_.is_over()) {
        // emit manual stopped signal
        signal_buffer_->set_signal(MotionSignal_ManualPointMoveStopped);
        _mainmode_switch_to(MotionMainMode::Idle);
        return;
    }

    // TODO touch detect
    // if (cb_touch_detected_) {
    //     if (cb_touch_detected_()) {
    //         pm_handler_.stop(true); // stop immediately
    //         // TODO touch detect signal / warn
    //         _mainmode_switch_to(MotionMainMode::Idle);
    //         return;
    //     }
    // }

    pm_handler_.run_once();

    cmd_axis_ = pm_handler_.get_current_pos();
}

void MotionStateMachine::_mainmode_auto() {
    // TODO
}

void MotionStateMachine::_mainmode_switch_to(MotionMainMode new_main_mode) {
    s_logger->trace("MainMode: {} -> {}", GetMainModeStr(main_mode_),
                    GetMainModeStr(new_main_mode));
    main_mode_ = new_main_mode;
}

const char *MotionStateMachine::GetMainModeStr(MotionMainMode mode) {
    switch (mode) {
#define XX_(m)              \
    case MotionMainMode::m: \
        return #m;
        XX_(Idle)
        XX_(Manual)
        XX_(Auto)
#undef XX_
    default:
        return "Unknow";
    }
}

} // namespace move

} // namespace edm
