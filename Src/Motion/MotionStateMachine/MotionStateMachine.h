#pragma once

#include <functional>
#include <memory>

#include "Motion/MoveDefines.h"
#include "Motion/MoveruntimeWrapper/MoveruntimeWrapper.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "Motion/SignalBuffer/SignalBuffer.h"

namespace edm {

namespace move {

/**
 * 1. construct it
 * 2. `set_cmd_axis` (optional): init it use ecat's act pos
 * 3. `set_enable`: Finally, you can enable it
 * 4. `run_once`: run the statemachine, and get
 */
class MotionStateMachine final {
public:
    using ptr = std::shared_ptr<MotionStateMachine>;
    MotionStateMachine(const std::function<bool(axis_t &)> &cb_get_act_axis,
                       const std::function<bool(void)> &cb_touch_detected,
                       SignalBuffer::ptr signal_buffer);
    ~MotionStateMachine() = default;

    // run once
    //! try not to use increment
    //! get absolute axis value after calling `run_once`
    void run_once();

    // reset the statemachine
    void reset();

    // enable or disable the statemachine
    // set disable (or reset) when the ecat is down
    void set_enable(bool enable) { enabled_ = enable; }

public: // setting interfaces
    // reset the local `cmd_axis_` to the input value
    void set_cmd_axis(const axis_t& init_cmd_axis);
    void refresh_axis_using_actpos();

public: // operate interfaces
    // input target_pos only, start_pos is default as current calc pos
    //! 绝对式坐标输入!
    bool start_manual_pointmove(const MoveRuntimePlanSpeedInput &speed_param,
                                const axis_t &target_pos);

    bool stop_manual_pointmove(bool immediate = false);

public: // state interfaces
    const axis_t &get_cmd_axis() const { return cmd_axis_; }
    bool is_enabled() const { return enabled_; }

    MotionMainMode main_mode() const { return main_mode_; }

    MotionAutoState auto_state() const { return auto_state_; }

private: // inside functions: state process
    void _mainmode_idle();
    void _mainmode_manual();
    void _mainmode_auto();

    void _mainmode_switch_to(MotionMainMode new_main_mode);

private:              // state data
    axis_t cmd_axis_; // 指令位值 (驱动器值, 单位blu)

    bool enabled_{
        false}; // 使能位 (防止reset后, 还未重新设置初始指令位置, 就进行运算)
    MotionMainMode main_mode_{MotionMainMode::Idle};       // 主模式
    MotionAutoState auto_state_{MotionAutoState::Stopped}; // Auto状态

private: // motion runtime data
    // mainmode manual:
    // pointmove handle
    PointMoveHandler pm_handler_;

private: // callbacks
    // TODO, get real axis
    // Note: use operator bool to determine if it is callable / valid.
    std::function<bool(axis_t &)> cb_get_act_axis_;
    std::function<bool(void)> cb_touch_detected_;

private: // signal dispatcher
    // TODO, 用于接收当前周期需要输出的信号,
    // 在外层周期结束时统一和info状态一同发出
    SignalBuffer::ptr signal_buffer_;

public:
    static const char *GetMainModeStr(MotionMainMode mode);
};

} // namespace move

} // namespace edm
