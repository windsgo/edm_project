#include "MoveruntimeWrapper.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

// 获取从当前速度减速到0的需要的大致长度, 使用该长度可以进行减速规划
// v: 当前速度值(>0), dec0: 减速使用的加速度(<0)
static inline unit_t _get_dec_length(double v, double dec0) /*input blu/s*/
{
    assert(dec0 < 0.0);
    assert(v > 0.0);
    return ((v * v) / ((-2.0) * dec0)); /*return blu.*/
}

const char *MoveruntimeWrapper::GetStateString(State s) {
    switch (s) {
#define XX123_(s__)  \
    case State::s__: \
        return #s__;
        XX123_(NotStarted);
        XX123_(Running);
        XX123_(Pausing);
        XX123_(Paused);
        XX123_(Stopping);
        XX123_(Stopped);

    default:
        return "Unknown";
#undef XX123_
    }
}

MoveruntimeWrapper::MoveruntimeWrapper() noexcept
    : mrt_{}, state_{State::NotStarted}, curr_length_{0.0},
      target_length_{0.0} {}

bool MoveruntimeWrapper::start(const MoveRuntimePlanSpeedInput &speed_param,
                               unit_t target_length) {
    if (!is_over()) {
        s_logger->warn("MoveruntimeWrapper start warn: already started");
        //! warn, but continue to plan new move
    }

    // 规划初始运动
    bool mrt_plan_ret = mrt_.plan(speed_param, target_length);
    if (!mrt_plan_ret) {
        s_logger->error(
            "MoveruntimeWrapper start error: plan error, length: {}",
            target_length);

        return false;
    }

    // 规划成功
    // 拷贝输入速度参数, 输入的其他参数
    record_speed_param_ = speed_param;
    temp_using_speed_param_ = record_speed_param_;
    curr_length_ = 0.0; // 清空当前长度
    target_length_ = target_length;

    state_ = State::Running;

    return true;
}

bool MoveruntimeWrapper::pause() {
    switch (state_) {
    default:
        assert(false);
        s_logger->critical("MoveruntimeWrapper pause error: unknown state: {}",
                           static_cast<int>(state_));
        return false;
    case State::Pausing:
        s_logger->trace("MoveruntimeWrapper pause: already pausing");
        return true;
    case State::Paused:
        s_logger->trace("MoveruntimeWrapper pause: already paused");
        return true;
    case State::Stopping:
    case State::Stopped:
        s_logger->error(
            "MoveruntimeWrapper pause error: not running, state: {}",
            GetStateString(state_));
        return false;
    case State::Running: {
        // 处理暂停速度规划和计算
        unit_t curr_speed = mrt_.get_current_speed();
        if (curr_speed <= 0.0) {
            // 速度为0, 直接暂停
            s_logger->trace("MoveruntimeWrapper pause: curr_speed <= 0.0, {}",
                            curr_speed);
            state_ = State::Paused;
            return true;
        }

        unit_t remaining_length = target_length_ - curr_length_;
        if (remaining_length <= 0.0) {
            // 剩余长度为0, 直接暂停
            s_logger->trace(
                "MoveruntimeWrapper pause: remaining_length <= 0.0, {}",
                remaining_length);
            state_ = State::Paused;
            return true;
        }

        unit_t dec_length =
            _get_dec_length(curr_speed, record_speed_param_.dec0);
        if (dec_length > remaining_length) {
            dec_length = remaining_length;
        }
        s_logger->trace("MoveruntimeWrapper pause: dec_length: {}", dec_length);

        // 规划暂停的减速
        temp_using_speed_param_.entry_v = curr_speed;
        temp_using_speed_param_.exit_v = 0.0;
        bool replan_ret = mrt_.plan(temp_using_speed_param_, dec_length);

        if (!replan_ret) {
            s_logger->critical(
                "MoveruntimeWrapper pause error: replan failed!");
            state_ = State::Paused;
            return false;
        }

        state_ = State::Pausing;
        return true;
    }
    }
}

bool MoveruntimeWrapper::resume() {
    switch (state_) {
    default:
        assert(false);
        s_logger->critical("MoveruntimeWrapper resume error: unknown state: {}",
                           static_cast<int>(state_));
        return false;
    case State::NotStarted:
    case State::Running:
    case State::Pausing:
    case State::Stopping:
    case State::Stopped:
        s_logger->warn("MoveruntimeWrapper resume: not paused, curr: {}",
                       GetStateString(state_));
        return false;
    case State::Paused: {
        unit_t remaining_length = target_length_ - curr_length_;
        if (remaining_length <= 0.0) {
            s_logger->trace(
                "MoveruntimeWrapper resume: remaining length <= 0.0, {}",
                remaining_length);
            state_ = State::Stopped; // 运行完毕
            return true;
        }

        temp_using_speed_param_.entry_v = 0.0;
        temp_using_speed_param_.exit_v = 0.0;
        bool replan_ret = mrt_.plan(temp_using_speed_param_, remaining_length);
        if (!replan_ret) {
            s_logger->critical(
                "MoveruntimeWrapper resume error: replan failed!");
            state_ = State::Stopped;
            return false;
        }

        state_ = State::Running;
        return true;
    }
    }
}

bool MoveruntimeWrapper::stop(bool immediate) {
    // 如果immediate急停 直接置为stopped状态, 不管
    if (immediate) {
        s_logger->trace("MoveruntimeWrapper immediate stop.");
        state_ = State::Stopped;
        return true;
    }

    switch (state_) {
    default:
        assert(false);
        s_logger->critical("MoveruntimeWrapper stop error: unknown state: {}",
                           static_cast<int>(state_));
        return false;
    case State::NotStarted:
        s_logger->warn("MoveruntimeWrapper stop warn: not started.");
        return false;
    case State::Paused:
        state_ = State::Stopped; // 已暂停情况, 直接切换为已停止
        return true;
    case State::Pausing:
        state_ = State::Stopping; // 暂停中情况, 直接切换为停止中,
                                  // 沿用暂停中运行的速度规划
        return true;
    case State::Stopping:
        s_logger->trace("MoveruntimeWrapper stop: already stopping.");
        return true;
    case State::Stopped:
        s_logger->trace("MoveruntimeWrapper stop: already stopped.");
        return true;
    case State::Running: {
        unit_t curr_speed = mrt_.get_current_speed();
        if (curr_speed <= 0.0) {
            // 速度为0, 直接停止
            s_logger->trace("MoveruntimeWrapper stop: curr_speed <= 0.0, {}",
                            curr_speed);
            state_ = State::Stopped;
            return true;
        }

        unit_t remaining_length = target_length_ - curr_length_;
        if (remaining_length <= 0.0) {
            // 剩余长度为0, 直接停止
            s_logger->trace(
                "MoveruntimeWrapper stop: remaining_length <= 0.0, {}",
                remaining_length);
            state_ = State::Stopped;
            return true;
        }

        unit_t dec_length =
            _get_dec_length(curr_speed, record_speed_param_.dec0);
        if (dec_length > remaining_length) {
            dec_length = remaining_length;
        }
        s_logger->trace("MoveruntimeWrapper stop: dec_length: {}", dec_length);

        // 规划减速
        temp_using_speed_param_.entry_v = curr_speed;
        temp_using_speed_param_.exit_v = 0.0;
        bool replan_ret = mrt_.plan(temp_using_speed_param_, dec_length);

        s_logger->debug(
            "stop replan: entry_v: {}, cruise_v: {}, acc: {}, dec: {}, nacc: "
            "{}",
            temp_using_speed_param_.entry_v, temp_using_speed_param_.cruise_v,
            temp_using_speed_param_.acc0, temp_using_speed_param_.dec0,
            temp_using_speed_param_.nacc);

        if (!replan_ret) {
            s_logger->critical("MoveruntimeWrapper stop error: replan failed!");
            state_ = State::Stopped;
            return false;
        }

        state_ = State::Stopping;
        return true;
    }
    }
}

bool MoveruntimeWrapper::change_speed(unit_t new_speed) {
    if (new_speed <= 0.0) {
        s_logger->error(
            "MoveruntimeWrapper change_speed error: speed invalid: {}",
            new_speed);
        return false;
    }

    switch (state_) {
    default:
        assert(false);
        s_logger->critical(
            "MoveruntimeWrapper change_speed error: unknown state: {}",
            static_cast<int>(state_));
        return false;
    case State::NotStarted:
    case State::Stopping:
    case State::Stopped:
        s_logger->error(
            "MoveruntimeWrapper change_speed error: state invalid: {}",
            GetStateString(state_));
        return false;
    case State::Paused:
    case State::Pausing:
        // 先把速度赋值到当前使用的临时速度参数中, 暂停后恢复用新的速度
        temp_using_speed_param_.cruise_v = new_speed;
        return true;
    case State::Running: {
        // 重新规划
        unit_t curr_speed = mrt_.get_current_speed();
        if (curr_speed <= 0.0) {
            // 速度已经为0, 不作重新规划, 不作处理, 不切换状态
            s_logger->trace(
                "MoveruntimeWrapper change_speed: curr_speed <= 0.0, {}",
                curr_speed);
            return false;
        }

        unit_t remaining_length = target_length_ - curr_length_;
        if (remaining_length <= 0.0) {
            // 剩余长度为0, 不作重新规划, 不作处理, 不切换状态
            s_logger->trace(
                "MoveruntimeWrapper change_speed: remaining_length <= 0.0, {}",
                remaining_length);
            return true;
        }

        temp_using_speed_param_.cruise_v = new_speed;
        temp_using_speed_param_.entry_v = curr_speed;
        temp_using_speed_param_.exit_v = 0.0;
        bool replan_ret = mrt_.plan(temp_using_speed_param_, remaining_length);
        if (!replan_ret) {
            s_logger->critical(
                "MoveruntimeWrapper change_speed error: replan failed!");
            state_ = State::Stopped;
            return false;
        }

        // state_ 不变
        return true;
    }
    }
}

void MoveruntimeWrapper::clear() {
    mrt_.clear();
    state_ = State::NotStarted;
    curr_length_ = 0.0;
    target_length_ = 0.0;
}

unit_t MoveruntimeWrapper::run_once() {
    unit_t result_inc = 0.0;
    switch (state_) {
    default:
        assert(false);
        s_logger->critical(
            "MoveruntimeWrapper run_once error: unknown state: {}",
            static_cast<int>(state_));
        state_ = State::Stopped;
        break;
    case State::NotStarted:
    case State::Paused:
    case State::Stopped:
        break;
    case State::Running: {
        result_inc = mrt_.run_once();
        curr_length_ += result_inc;
        if (mrt_.is_over()) {
            state_ = State::Stopped;
        }
        break;
    }
    case State::Pausing: {
        result_inc += mrt_.run_once();
        curr_length_ += result_inc;
        if (mrt_.is_over()) {
            state_ = State::Paused;
        }
        break;
    }
    case State::Stopping: {
        result_inc += mrt_.run_once();
        curr_length_ += result_inc;
        if (mrt_.is_over()) {
            state_ = State::Stopped;
        }
        break;
    }
    }

    return result_inc;
}

} // namespace move

} // namespace edm
