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
    TouchDetectHandler::ptr touch_detect_handler,
    SignalBuffer::ptr signal_buffer)
    : cb_get_act_axis_(cb_get_act_axis),
      touch_detect_handler_(touch_detect_handler),
      signal_buffer_(signal_buffer) {
    reset();

    //! use assert more than exception is better
    if (!cb_get_act_axis_) {
        throw exception("no cb_get_act_axis_");
    }

    if (!touch_detect_handler_) {
        throw exception("no touch_detect_handler_");
    }

    if (!signal_buffer_) {
        throw exception("no signal_buffer_");
    }

    auto_task_runner_ =
        std::make_shared<AutoTaskRunner>(touch_detect_handler_, signal_buffer_);

    // 初始化计算坐标
    // 先清0 (离线调试模式时实际坐标会直接返回当前值)
    MotionUtils::ClearAxis(cmd_axis_);
#ifndef EDM_OFFLINE_RUN_NO_ECAT
    cb_get_act_axis_(cmd_axis_);
#endif // EDM_OFFLINE_RUN_NO_ECAT
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

    touch_detect_handler_->reset();
}

void MotionStateMachine::set_cmd_axis(const axis_t &init_cmd_axis) {
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
            signal_buffer_->set_signal(MotionSignal_ManualPointMoveStarted);

            //! test
            // r_.start_record("output.bin");
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

bool MotionStateMachine::start_auto_g00(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &target_pos,
    bool enable_touch_detect) {
    s_logger->trace("{}", __FUNCTION__);

    if (main_mode_ != MotionMainMode::Idle) {
        return false;
    }

    auto new_g00_auto_task = std::make_shared<G00AutoTask>(
        cmd_axis_, target_pos, speed_param, enable_touch_detect,
        touch_detect_handler_);

    if (new_g00_auto_task->is_over()) {
        return false;
    }

    auto ret = auto_task_runner_->restart_task(new_g00_auto_task);
    if (!ret) {
        return false;
    }

    _mainmode_switch_to(MotionMainMode::Auto);
    return true;
}

bool MotionStateMachine::pause_auto() {
    s_logger->trace("{}", __FUNCTION__);

    if (main_mode_ != MotionMainMode::Auto) {
        return false;
    }

    return auto_task_runner_->pause();
}

bool MotionStateMachine::resume_auto() {
    s_logger->trace("{}", __FUNCTION__);

    if (main_mode_ != MotionMainMode::Auto) {
        return false;
    }

    return auto_task_runner_->resume();
}

bool MotionStateMachine::stop_auto(bool immediate) {
    s_logger->trace("{}", __FUNCTION__);

    if (main_mode_ != MotionMainMode::Auto) {
        return false;
    }

    return auto_task_runner_->stop(immediate);
}

void MotionStateMachine::_mainmode_idle() {
    // TODO
    touch_detect_handler_->set_detect_enable(false);
}

void MotionStateMachine::_mainmode_manual() {
    if (pm_handler_.is_over()) {
        // emit manual stopped signal
        signal_buffer_->set_signal(MotionSignal_ManualPointMoveStopped);
        _mainmode_switch_to(MotionMainMode::Idle);
        // r_.stop_record();
        return;
    }

    // touch detect
    if (touch_detect_handler_->run_detect_once()) {
        pm_handler_.stop(true);
        signal_buffer_->set_signal(MotionSignal_ManualPointMoveStopped);
        // 接触感知报警需要上层控制处理
        _mainmode_switch_to(MotionMainMode::Idle);
        return; // return 掉, 防止继续run 或产生赋值
    }

    pm_handler_.run_once();

    // double last = cmd_axis_[0];

    cmd_axis_ = pm_handler_.get_current_pos();

    // r_.emplace(cmd_axis_[0] - last, last, cmd_axis_[0]);
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
