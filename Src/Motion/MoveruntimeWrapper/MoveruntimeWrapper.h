#pragma once

#include "Motion/Moveruntime/Moveruntime.h"

namespace edm {

namespace move {

class MoveruntimeWrapper final {
public:
    enum class State {
        NotStarted, // 未运动, 初始化状态
        Running,
        Pausing,  // 外部调用暂停, 减速的过程
        Paused,   // 暂停的最终状态, 无论是否还有剩余长度
        Stopping, // 外部调用停止, 减速的过程
        Stopped // 停止的最终状态, 包括外部减速停止, 正常走完停止,
                // 暂停后重新开始运行时无剩余长度时切换到已停止
    };
    static const char * GetStateString(State s);

public:
    using ptr = std::shared_ptr<MoveruntimeWrapper>;
    MoveruntimeWrapper() noexcept;
    ~MoveruntimeWrapper() noexcept = default;

    bool start(const MoveRuntimePlanSpeedInput &speed_param,
               unit_t target_length);
    bool pause();
    bool resume();
    bool stop(bool immediate = false);
    bool change_speed(unit_t new_speed);

    void clear(); // 和直接停止是不一样的, 直接停止不会清空最后的状态(当前长度,
                  // 目标长度)

    unit_t run_once(); // 运行一次, 返回本次运行的增量值

    unit_t get_current_length() const { return curr_length_; }
    unit_t get_target_length() const { return target_length_; }

    inline auto state() const { return state_; }
    inline bool is_started() const { return state_ != State::NotStarted; }
    inline bool is_normal_running() const { return state_ == State::Running; }
    inline bool is_paused() const { return state_ == State::Paused; }

    // 如果规划错误/启动错误, 也认为处于stopped状态
    inline bool is_stopped() const {
        return state_ == State::Stopped || state_ == State::NotStarted;
    }
    inline bool is_over() const { return is_stopped(); }

private:
    Moveruntime mrt_; // 用于每段的加减速规划

    MoveRuntimePlanSpeedInput record_speed_param_; // 保存输入的速度参数

    //! 此记录变量的 entry_v, exit_v, cruise_v 也可能会改变, 中途进行暂停时,
    //! 不可依赖
    // 改变速度时, 修改 temp_using_speed_param_ 的cruise_v,
    // 后面如果再有重新规划的情况, 沿用此速度
    MoveRuntimePlanSpeedInput
        temp_using_speed_param_; // 用于减速, 重新规划等的情况的输入参数

    State state_;

    unit_t curr_length_;   // 当前长度(暂停时, 停止时也有效)
    unit_t target_length_; // 目标长度(暂停, 停止不会改变此值)
};

} // namespace move

} // namespace edm
