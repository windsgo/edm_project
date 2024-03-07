#include "MotionAutoTaskRunner.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {
AutoTaskRunner::AutoTaskRunner(TouchDetectHandler::ptr touch_detect_handler,
                               SignalBuffer::ptr signal_buffer)
    : signal_buffer_(signal_buffer) {
    pausemove_controller_ =
        std::make_shared<PauseMoveController>(touch_detect_handler);
}

void AutoTaskRunner::reset(const axis_t &init_axis) {
    curr_task_ = nullptr;
    _autostate_switch_to(MotionAutoState::Stopped);
    pausemove_controller_->init(init_axis);
    curr_cmd_axis_ = init_axis;
}

bool AutoTaskRunner::restart_task(AutoTask::ptr task) {
    if (!curr_task_) {
        curr_task_ = task;
        curr_cmd_axis_ = task->get_curr_cmd_axis();
        _autostate_switch_to(MotionAutoState::NormalMoving);
        return true;
    }

    if (!curr_task_->is_over()) {
        s_logger->warn("{}: curr task not over", __PRETTY_FUNCTION__);
    }

    curr_task_ = task;
    curr_cmd_axis_ = task->get_curr_cmd_axis();
    _autostate_switch_to(MotionAutoState::NormalMoving);
    return true;
}

void AutoTaskRunner::run_once() {
    if (!curr_task_) {
        return;
    }

    switch (state_) {
    case MotionAutoState::NormalMoving:
        _normal_moving();
        break;
    case MotionAutoState::Pausing:
        _pausing();
        break;
    case MotionAutoState::Paused:
        _paused();
        break;
    case MotionAutoState::Resuming:
        _resuming();
        break;
    case MotionAutoState::Stopping:
        _stopping();
        break;
    case MotionAutoState::Stopped:
        // Do nothing
        break;
    default:
        s_logger->critical("{}: unknow state: {}", __PRETTY_FUNCTION__, (int)state_);
        break;
    }
}

bool AutoTaskRunner::pause() {
    if (!curr_task_) {
        return false;
    }

    switch (state_) {
    case MotionAutoState::NormalMoving:
    case MotionAutoState::Resuming: {
        auto ret = curr_task_->pause();
        if (ret) {
            _autostate_switch_to(MotionAutoState::Pausing);
            return true;
        } else {
            return false;
        }
    }

    case MotionAutoState::Pausing:
    case MotionAutoState::Stopping:
    case MotionAutoState::Stopped:
        return true;
    
    case MotionAutoState::Paused:
        return pausemove_controller_->pause_recover();

    }

    s_logger->warn("{}: should not here", __PRETTY_FUNCTION__);
    return false;
}

bool AutoTaskRunner::resume() {
    if (!curr_task_) {
        return false;
    }

    switch (state_) {
    case MotionAutoState::NormalMoving:
    case MotionAutoState::Pausing:
    case MotionAutoState::Stopping:
    case MotionAutoState::Stopped:
        return false;
    
    case MotionAutoState::Paused: {

        if (pausemove_controller_->is_recover_activated()) {
            return pausemove_controller_->resume_recover();
        } else {
            return pausemove_controller_->activate_recover();
        }

        // 暂停点动恢复完后会自动恢复curr task
    }

    case MotionAutoState::Resuming:
        return true;

    }

    s_logger->warn("{}: should not here", __PRETTY_FUNCTION__);
    return false;
}

bool AutoTaskRunner::stop(bool immediate) {
    if (!curr_task_) {
        return false;
    }

    switch (state_) {
    case MotionAutoState::NormalMoving:
    case MotionAutoState::Resuming:
    case MotionAutoState::Pausing: {
        auto ret = curr_task_->stop(immediate);
        if (ret) {
            _autostate_switch_to(MotionAutoState::Stopping);
            return true;
        } else {
            return false;
        }
    }
    case MotionAutoState::Stopping:
    case MotionAutoState::Stopped:
        return true;
    
    case MotionAutoState::Paused:
        // 暂停点动停止完后会自动切换到Stopped
        return pausemove_controller_->stop(immediate);

    }

    s_logger->warn("{}: should not here", __PRETTY_FUNCTION__);
    return false;
}

bool AutoTaskRunner::start_manual_pointmove(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &start_pos,
    const axis_t &target_pos) {
    if (state_ != MotionAutoState::Paused) {
        return false;
    }

    return pausemove_controller_->start_manual_pointmove(speed_param, start_pos,
                                                         target_pos);
}

bool AutoTaskRunner::stop_manual_pointmove(bool immediate) {
    if (state_ != MotionAutoState::Paused) {
        return false;
    }

    return pausemove_controller_->stop_manual_pointmove(immediate);
}

void AutoTaskRunner::_autostate_switch_to(MotionAutoState new_state) {
    s_logger->trace("AutoState: {} -> {}", GetAutoStateStr(state_),
                    GetAutoStateStr(new_state));
    state_ = new_state;
}

void AutoTaskRunner::_autostate_switch_to_paused() {
    _autostate_switch_to(MotionAutoState::Paused);
    pausemove_controller_->init(curr_cmd_axis_); //! 初始化坐标
}

void AutoTaskRunner::_normal_moving() {
    //! 要防止G00暂停时少走一个周期
    //! 运行放在前面

    curr_task_->run_once();
    curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

    if (curr_task_->is_pausing() || curr_task_->is_paused()) {
        _autostate_switch_to(MotionAutoState::Pausing);
        return;
    }

    if (curr_task_->is_stopping() || curr_task_->is_stopped()) {
        _autostate_switch_to(MotionAutoState::Stopping);
        return;
    }
}

void AutoTaskRunner::_pausing() {
    //! 要防止G00暂停时少走一个周期
    //! 运行放在前面

    curr_task_->run_once();
    curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

    if (curr_task_->is_paused()) {
        signal_buffer_->set_signal(MotionSignal_AutoPaused);
        _autostate_switch_to_paused();
        return;
    }

    if (curr_task_->is_stopping() || curr_task_->is_stopped()) {
        _autostate_switch_to(MotionAutoState::Stopping);
        return;
    }

    //!
    if (curr_task_->is_normal_running()) {
        s_logger->warn("{}: should not be here, in pausing, but task is "
                       "running",
                       __PRETTY_FUNCTION__);
        _autostate_switch_to(MotionAutoState::Resuming);
        return;
    }
}

void AutoTaskRunner::_paused() {
    pausemove_controller_->run_once();
    curr_cmd_axis_ = pausemove_controller_->get_cmd_axis();

    // 在这个状态处理 pausemove 的状态变化
    if (pausemove_controller_->is_stopped()) {
        signal_buffer_->set_signal(MotionSignal_AutoStopped);
        _autostate_switch_to(MotionAutoState::Stopped);
        return;
    }

    if (pausemove_controller_->is_recover_over()) {
        auto ret = curr_task_->resume();
        if (ret) {
            _autostate_switch_to(MotionAutoState::Resuming);
        } else {
            s_logger->critical("{}: resume auto task failed", __PRETTY_FUNCTION__);
        }
    }
}

void AutoTaskRunner::_resuming() {
    curr_task_->run_once();
    // 暂停恢复后的点一定与task暂停完成时的点一致, 应该不会出问题
    curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

    //! 要防止G00少走一个周期
    //! 运行放在前面

    if (curr_task_->is_normal_running()) {
        signal_buffer_->set_signal(MotionSignal_AutoResumed);
        _autostate_switch_to(MotionAutoState::NormalMoving);
        return;
    }

    if (curr_task_->is_stopping() || curr_task_->is_stopped()) {
        _autostate_switch_to(MotionAutoState::Stopping);
        return;
    }
    if (curr_task_->is_pausing() || curr_task_->is_paused()) {
        _autostate_switch_to(MotionAutoState::Pausing);
        return;
    }
}

void AutoTaskRunner::_stopping() {
    curr_task_->run_once();
    curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

    if (curr_task_->is_stopped()) {
        signal_buffer_->set_signal(MotionSignal_AutoStopped);
        _autostate_switch_to(MotionAutoState::Stopped);
        return;
    }
}

const char *AutoTaskRunner::GetAutoStateStr(MotionAutoState state) {
    switch (state) {
#define XX_(m)               \
    case MotionAutoState::m: \
        return #m;
        XX_(NormalMoving)
        XX_(Pausing)
        XX_(Paused)
        XX_(Resuming)
        XX_(Stopping)
        XX_(Stopped)
#undef XX_
    default:
        return "Unknow";
    }
}

} // namespace move

} // namespace edm
