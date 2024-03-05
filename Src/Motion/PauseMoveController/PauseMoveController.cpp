#include "PauseMoveController.h"

#include "Logger/LogMacro.h"
#include "Motion/MotionUtils/MotionUtils.h"

#include <cassert>

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

PauseMoveController::PauseMoveController(
    TouchDetectHandler::ptr touch_detect_handler)
    : touch_detect_handler_(touch_detect_handler) {}

void PauseMoveController::init(const axis_t &init_axis) {
    pm_handler_.clear();
    ClearStackOrQueue(axis_recorder_);
    curr_cmd_axis_ = init_axis; //! 重要, 更新初始坐标
    state_ = State::AllowManualPointMove;
}

bool PauseMoveController::start_manual_pointmove(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &start_pos,
    const axis_t &target_pos) {
    s_logger->trace("{}", __FUNCTION__);

    if (state_ != State::AllowManualPointMove) {
        return false;
    }

    bool ret = pm_handler_.start(speed_param, start_pos, target_pos);

    if (!ret) {
        return false;
    }

    if (pm_handler_.is_over()) {
        return false;
    }

    state_ = State::ManualPointMoving;
    return true;

    //! 记录点位在每次停止之后记录, 从pm_handler可以读取起点和终点
    //! 记录的是每一个过程
}

bool PauseMoveController::stop_manual_pointmove(bool immediate) {
    s_logger->trace("{}", __FUNCTION__);
    if (state_ != State::ManualPointMoving) {
        return false;
    }

    return pm_handler_.stop(immediate);
}

bool PauseMoveController::pause() {
    if (state_ == State::RecoveringPausing || state_ == State::RecoveringPaused) {
        return true;
    }

    if (state_ != State::Recovering) {
        return false;
    }

    if (!pm_handler_.pause()) {
        s_logger->error("{}: pm pause failed", __FUNCTION__);
        return false;
    }

    state_ = State::RecoveringPausing;
    return true;
}

bool PauseMoveController::resume() {
    if (state_ == State::Recovering || state_ == State::RecorverOver) {
        return true;
    }

    if (state_ != State::RecoveringPaused) {
        return false;
    }

    if (!pm_handler_.resume()) {
        s_logger->error("{}: pm resume failed", __FUNCTION__);
        return false;
    }

    state_ = State::Recovering;
    return true;
}

bool PauseMoveController::stop(bool immediate) {
    switch (state_)
    {
    case State::NotInited:
        s_logger->error("{}: not inited", __FUNCTION__);
        return false;

    case State::AllowManualPointMove:
    case State::OutSideOrErrorStopped:
    case State::RecorverOver:
    case State::RecoveringPaused:
        state_ = State::OutSideOrErrorStopped;
        return true;

    case State::OutSideStopping:
        return true;

    case State::ManualPointMoving:
    case State::Recovering:
    case State::RecoveringPausing:
        pm_handler_.stop(immediate);
        state_ = State::OutSideStopping;
        return true;
    default:
        s_logger->warn("{}: unknow pmc state: {}", __FUNCTION__, (int)state_);
        return false;
    }
}

bool PauseMoveController::activate_recover() {
    s_logger->trace("{}", __FUNCTION__);
    if (state_ == State::ManualPointMoving) {
        return false;
    }

    if (state_ == State::Recovering || state_ == State::RecorverOver) {
        return true;
    }

    if (state_ == State::RecoveringPaused) {
        s_logger->warn("{} state_ == State::RecoveringPaused", __FUNCTION__);
        return resume(); // 当作继续指令
    }

    if (axis_recorder_.empty()) {
        s_logger->trace("{}: axis_recorder_ empty", __FUNCTION__);
        state_ = State::RecorverOver;
        return true;
    }

    // 启动第一段恢复
    auto final_segment = axis_recorder_.top();
    axis_recorder_.pop();

    bool ret =
        pm_handler_.start(final_segment.plan_param_, final_segment.stop_point_,
                          final_segment.start_point_);

    if (!ret) {
        s_logger->critical("{}: plan first segement failed", __FUNCTION__);
        state_ = State::OutSideOrErrorStopped;
        return false;
    }

    state_ = State::Recovering;
    return true;
}

void PauseMoveController::run_once() {
    switch (state_) {
    case State::NotInited:
        // should not here !! //! init before call run_once
        s_logger->critical("{}: need inited", __FUNCTION__);
        break;
    case State::AllowManualPointMove:
        break;
    case State::ManualPointMoving:
        _manual_pointmoving();
        break;
    case State::Recovering:
        _recovering();
        break;
    case State::RecoveringPausing:
        _recovering_pausing();
        break;
    case State::RecoveringPaused:
        break;
    case State::RecorverOver:
        break;
    case State::OutSideStopping:
        _outside_stopping();
        break;
    case State::OutSideOrErrorStopped:
        break;
    default:
        s_logger->critical("unknow PauseMoveController state: {}", (int)state_);
        assert(false);
        exit(-1);
        break;
    }
}

void PauseMoveController::_manual_pointmoving() {
    if (pm_handler_.is_over()) {
        // 当前点动已停止
        if (pm_handler_.get_start_pos() == pm_handler_.get_target_pos()) {
            // 起点终点相同
            s_logger->warn("{}: pos the same", __FUNCTION__);
            state_ = State::AllowManualPointMove;
            return;
        }

        // 记录
        axis_recorder_.emplace(pm_handler_.get_start_pos(),
                               pm_handler_.get_target_pos(),
                               pm_handler_.get_speed_param());

        state_ = State::AllowManualPointMove;

        return;
    }

    // 检查接触感知
    if (touch_detect_handler_->run_detect_once()) {
        pm_handler_.stop(true);
        // 下一个周期处理停止
    }

    // 还没停止, 走一下
    pm_handler_.run_once();

    // 更新坐标
    curr_cmd_axis_ = pm_handler_.get_current_pos();
}

void PauseMoveController::_recovering() {
    if (pm_handler_.is_over()) {
        // 当前段恢复已结束

        if (axis_recorder_.empty()) {
            s_logger->info("pm recover over");
            state_ = State::RecorverOver;
            return;
        }

        auto segement = axis_recorder_.top();
        axis_recorder_.pop();

        bool ret = pm_handler_.start(segement.plan_param_, segement.stop_point_,
                                     segement.start_point_);

        if (!ret) {
            s_logger->warn("{}: start pm failed", __FUNCTION__);
        }

        return;
    }

    // 继续运动, 不判断接触感知
    pm_handler_.run_once();

    curr_cmd_axis_ = pm_handler_.get_current_pos();
}

void PauseMoveController::_recovering_pausing() {
    if (pm_handler_.is_paused()) {
        state_ = State::RecoveringPaused;
        return;
    }

    pm_handler_.run_once();

    curr_cmd_axis_ = pm_handler_.get_current_pos();
}

void PauseMoveController::_outside_stopping() {
    if (pm_handler_.is_stopped()) {
        state_ = State::OutSideOrErrorStopped;
        return;
    }

    pm_handler_.run_once();

    curr_cmd_axis_ = pm_handler_.get_current_pos();
}

} // namespace move

} // namespace edm
