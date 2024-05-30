#include "MotionAutoTaskRunner.h"
#include "Motion/MotionSharedData/MotionSharedData.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

AutoTaskRunner::AutoTaskRunner(TouchDetectHandler::ptr touch_detect_handler,
                               SignalBuffer::ptr signal_buffer)
    : signal_buffer_(signal_buffer) {
    pausemove_controller_ = std::make_shared<PauseMoveController>(
        touch_detect_handler, signal_buffer);
}

void AutoTaskRunner::reset() {
    curr_task_ = nullptr;
    _autostate_switch_to(MotionAutoState::Stopped);

    pausemove_controller_->init();
    // curr_cmd_axis_ = init_axis;
}

bool AutoTaskRunner::restart_task(AutoTask::ptr task) {
    if (!curr_task_) {
        curr_task_ = task;
        // curr_cmd_axis_ = task->get_curr_cmd_axis();
        _autostate_switch_to(MotionAutoState::NormalMoving);
        _dominated_state_switch_to(DominatedState::AutoTaskRunning);
        return true;
    }

    if (!curr_task_->is_over()) {
        s_logger->warn("{}: curr task not over", __PRETTY_FUNCTION__);
    }

    curr_task_ = task;
    // curr_cmd_axis_ = task->get_curr_cmd_axis();
    _autostate_switch_to(MotionAutoState::NormalMoving);
    _dominated_state_switch_to(DominatedState::AutoTaskRunning);
    return true;
}

void AutoTaskRunner::run_once() {
    if (!curr_task_) {
        return;
    }

    // 暂停恢复中的状态
    if (domin_state_ == DominatedState::PauseMoveRecoverRunning) {
        _dominated_state_pmrecovering_run_once();
        return;
    }

    // domin_state_ == DominatedState::AutoTaskRunning

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
        s_logger->critical("{}: unknow state: {}", __PRETTY_FUNCTION__,
                           (int)state_);
        break;
    }
}

bool AutoTaskRunner::pause() {
    if (!curr_task_) {
        return false;
    }

    if (domin_state_ == DominatedState::PauseMoveRecoverRunning) {
        switch (state_) {
        case MotionAutoState::Paused:
        case MotionAutoState::Pausing:
            return true;
        case MotionAutoState::Stopping:
            return false;
        case MotionAutoState::NormalMoving: {
            auto ret = pausemove_controller_->pause_recover();
            if (ret) {
                _autostate_switch_to(MotionAutoState::Pausing);
            }

            return ret;
        }

        default:
            assert(false);
            return false;
        }
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
    case MotionAutoState::Paused:
        return true;
    case MotionAutoState::Stopping:
    case MotionAutoState::Stopped:
        return false;
    }

    s_logger->warn("{}: should not here", __PRETTY_FUNCTION__);
    return false;
}

bool AutoTaskRunner::resume() {
    if (!curr_task_) {
        return false;
    }

    if (domin_state_ == DominatedState::PauseMoveRecoverRunning) {
        switch (state_) {
        case MotionAutoState::NormalMoving:
            return true;
        case MotionAutoState::Pausing:
        case MotionAutoState::Stopping:
            return false;
        case MotionAutoState::Paused: {
            auto ret = pausemove_controller_->resume_recover();
            if (ret) {
                _autostate_switch_to(MotionAutoState::NormalMoving);
                signal_buffer_->set_signal(MotionSignal_AutoResumed);
            }

            return ret;
        }

        default:
            assert(false);
            return false;
        }
    }

    switch (state_) {
    case MotionAutoState::NormalMoving:
    case MotionAutoState::Pausing:
    case MotionAutoState::Stopping:
    case MotionAutoState::Stopped:
        return false;

    case MotionAutoState::Paused: {
        auto ret = pausemove_controller_->activate_recover();
        if (ret) {
            _dominated_state_switch_to(DominatedState::PauseMoveRecoverRunning);
            _autostate_switch_to(MotionAutoState::NormalMoving);
            signal_buffer_->set_signal(MotionSignal_AutoResumed);
        }

        return ret;
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

    if (domin_state_ == DominatedState::PauseMoveRecoverRunning) {
        switch (state_) {
        case MotionAutoState::Stopping:
            return true;
        case MotionAutoState::NormalMoving:
        case MotionAutoState::Pausing:
        case MotionAutoState::Paused: {
            auto ret = pausemove_controller_->stop(immediate);
            if (ret) {
                _autostate_switch_to(MotionAutoState::Stopping);
            }

            return ret;
        }

        default:
            assert(false);
            return false;
        }
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
    case MotionAutoState::Paused: {
        // 2024.04.22 fix
        //！ paused 时, 停止, 直接转到stopped
        //！ 不再执行 curr_task 的 stop 动作,
        // 以解决暂停点动之后, 还没resume直接stop时, 坐标跳变到curr task的坐标去

        _autostate_switch_to(MotionAutoState::Stopped);
        return true;
    }
    case MotionAutoState::Stopping:
    case MotionAutoState::Stopped:
        return true;
    }

    s_logger->warn("{}: should not here", __PRETTY_FUNCTION__);
    return false;
}

bool AutoTaskRunner::start_manual_pointmove(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &start_pos,
    const axis_t &target_pos) {
    if (state_ != MotionAutoState::Paused ||
        domin_state_ != DominatedState::AutoTaskRunning) {
        return false;
    }

    return pausemove_controller_->start_manual_pointmove(speed_param, start_pos,
                                                         target_pos);
}

bool AutoTaskRunner::stop_manual_pointmove(bool immediate) {
    if (state_ != MotionAutoState::Paused ||
        domin_state_ != DominatedState::AutoTaskRunning) {
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
    pausemove_controller_->init(); //! 初始化坐标
}

void AutoTaskRunner::_normal_moving() {
    //! 要防止G00暂停时少走一个周期
    //! 运行放在前面

    curr_task_->run_once();
    // curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

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
    // curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

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
    // 这里run_once是运行手动点动
    pausemove_controller_->run_once();
    // curr_cmd_axis_ = pausemove_controller_->get_cmd_axis();

    // // 在这个状态处理 pausemove 的状态变化
    // if (pausemove_controller_->is_stopped()) {
    //     signal_buffer_->set_signal(MotionSignal_AutoStopped);
    //     _autostate_switch_to(MotionAutoState::Stopped);
    //     return;
    // }

    // if (pausemove_controller_->is_recover_over()) {
    //     auto ret = curr_task_->resume();
    //     if (ret) {
    //         _autostate_switch_to(MotionAutoState::Resuming);
    //     } else {
    //         s_logger->critical("{}: resume auto task failed",
    //                            __PRETTY_FUNCTION__);
    //     }
    // }
}

void AutoTaskRunner::_resuming() {
    curr_task_->run_once();
    // 暂停恢复后的点一定与task暂停完成时的点一致, 应该不会出问题
    // curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

    //! 要防止G00少走一个周期
    //! 运行放在前面

    if (curr_task_->is_normal_running()) {
        signal_buffer_->set_signal(MotionSignal_AutoResumed);
        _autostate_switch_to(MotionAutoState::NormalMoving);
        return;
    }

    if (curr_task_->is_stopping() || curr_task_->is_stopped()) {
        signal_buffer_->set_signal(
            MotionSignal_AutoResumed); // Fix M00 resumed signal
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
    // curr_cmd_axis_ = curr_task_->get_curr_cmd_axis();

    if (curr_task_->is_stopped()) {
        signal_buffer_->set_signal(MotionSignal_AutoStopped);
        _autostate_switch_to(MotionAutoState::Stopped);
        return;
    }
}

void AutoTaskRunner::_dominated_state_pmrecovering_run_once() {
    switch (state_) {
    case MotionAutoState::NormalMoving: {
        // 恢复中正常的状态

        // 运行一次恢复过程
        pausemove_controller_->run_once();
        // curr_cmd_axis_ = pausemove_controller_->get_cmd_axis();

        // 处理 pausemove 的状态变化

        // 停止(不是正常的恢复完成)
        if (pausemove_controller_->is_stopped()) {
            signal_buffer_->set_signal(MotionSignal_AutoStopped);
            _autostate_switch_to(MotionAutoState::Stopped);
            return;
        }

        // 恢复完成
        if (pausemove_controller_->is_recover_over()) {
            auto ret = curr_task_->resume();
            if (ret) {
                _dominated_state_switch_to(DominatedState::AutoTaskRunning);
                _autostate_switch_to(MotionAutoState::Resuming);
            } else {
                s_logger->critical("{}: resume auto task failed",
                                   __PRETTY_FUNCTION__);
                return;
            }
        }
        break;
    }

    case MotionAutoState::Pausing: {
        // 运行一次恢复过程
        pausemove_controller_->run_once();
        // curr_cmd_axis_ = pausemove_controller_->get_cmd_axis();

        if (pausemove_controller_->is_recover_paused()) {
            signal_buffer_->set_signal(MotionSignal_AutoPaused);
            _autostate_switch_to(MotionAutoState::Paused);
        }

        break;
    }

    case MotionAutoState::Paused: {
        break;
    }

    case MotionAutoState::Stopping: {
        // 运行一次恢复过程
        pausemove_controller_->run_once();
        // curr_cmd_axis_ = pausemove_controller_->get_cmd_axis();
        
        if (pausemove_controller_->is_stopped()) {
            signal_buffer_->set_signal(MotionSignal_AutoStopped);
            _autostate_switch_to(MotionAutoState::Stopped);
            _dominated_state_switch_to(DominatedState::AutoTaskRunning);
        }

        break;
    }

    default:
        assert(false);
        break;
    }

    return;
}

void AutoTaskRunner::_dominated_state_switch_to(
    DominatedState new_domin_state) {
    s_logger->trace("dominated state: {} -> {}", GetDominStateStr(domin_state_),
                    GetDominStateStr(new_domin_state));

    domin_state_ = new_domin_state;
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

const char *AutoTaskRunner::GetDominStateStr(DominatedState domin_state) {
    switch (domin_state) {
#define XX_(m)              \
    case DominatedState::m: \
        return #m;
        XX_(AutoTaskRunning)
        XX_(PauseMoveRecoverRunning)
#undef XX_
    default:
        return "Unknow";
    }
}

} // namespace move

} // namespace edm
