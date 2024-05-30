#pragma once

#include <chrono>

#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "MotionAutoTask.h"

#include "Motion/Trajectory/TrajectorySegement.h"

#include "Motion/JumpDefines.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include "Motion/MotionSharedData/MotionSharedData.h"

#include <cassert>

namespace edm {

namespace move {

class G01AutoTask : public AutoTask {
public:
    G01AutoTask(TrajectoryLinearSegement::ptr line_traj,
                unit_t max_jump_height_from_begin,
                const std::function<void(JumpParam &)> &cb_get_jump_param,
                const std::function<void(bool)> &cb_enable_votalge_gate,
                const std::function<void(bool)> &cb_mach_on);

    bool pause() override;
    bool resume() override;
    bool stop(bool immediate = false) override;

    bool is_normal_running() const override;
    bool is_pausing() const override;
    bool is_paused() const override;
    bool is_resuming() const override;
    bool is_stopping() const override;
    bool is_stopped() const override;
    bool is_over() const override;

    void run_once() override;

public: // custom interfaces
public:
    // G01 内部的 running_state, 可能比外层复杂, 还会涉及到抬刀,
    // 暂停回加工起点等
    enum class State {
        NormalRunning, // 正常运行, 包括抬刀
        Pausing, // 如果在抬刀, 等待抬刀完成, (回到加工起点)(先不实现), 暂停
        Paused,
        Resuming, // 如果有暂停回到加工起点, 这里就是恢复加工位置
        Stopping, // 如果在抬刀, 等待抬刀完成
        Stopped
    };

    // 用于正常执行动作的SubState
    enum class ServoSubState {
        // 伺服中, 在暂停中和停止中时, 要等待返回伺服中的子状态
        Servoing,

        JumpUping,   // 抬刀回退
        JumpDowning, // 抬刀前进到缓冲距离之前

        // 缓冲距离以伺服方式进行前进, 如果发生短路回退指令或缓冲段已走完,
        // 则退出缓冲状态, 转入正常NormalRunning
        JumpDowningBuffer,
    };

    // 用于暂停Pausing, 停止Stopping过程的SubState
    enum class PauseOrStopSubState {
        WaitingForJumpEnd,
        BackingToBegin, // if needed or set
        // Other Pause Sub State
    };

    enum class ResumeSubState {
        RecoveringToLastMachingPos, // if is backed to begin
    };

    static const char *GetStateStr(State s);
    static const char *GetServoSubStateStr(ServoSubState s);
    static const char *GetPauseOrStopSubStateStr(PauseOrStopSubState s);
    static const char *GetResumeSubStateStr(ResumeSubState s);

private:
    static inline int64_t GetCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

private:
    void _state_changeto(State new_s);
    void _servo_substate_changeto(ServoSubState new_s);
    void _pauseorstop_substate_changeto(PauseOrStopSubState new_s);
    void _resume_substate_changeto(ResumeSubState new_s);

private:
    void _state_normal_running();
    void _state_pausing();
    void _state_paused();
    void _state_resuming();
    void _state_stopping();
    void _state_stopped();

    void _state_pausing_or_stopping(State target_state /* Paused or Stopped */);

    void _servo_substate_servoing();
    void _servo_substate_jumpuping();
    void _servo_substate_jumpdowning();
    void _servo_substate_jumpdowningbuffer();

    bool _servoing_check_and_plan_jump();
    bool _servoing_do_servothings(); // 计算伺服, 返回是否可以结束G01

private:
    bool _plan_jump_up();

    bool _check_and_validate_jump_height();

private:
    double _get_servo_cmd_from_shared();

private:
    State state_{State::NormalRunning};
    ServoSubState servo_sub_state_{ServoSubState::Servoing};
    PauseOrStopSubState pause_or_stop_sub_state_{
        PauseOrStopSubState::WaitingForJumpEnd};
    ResumeSubState resume_sub_state_{
        ResumeSubState::RecoveringToLastMachingPos};

    // 抬刀变量
    JumpParam jumping_param_;
    int64_t last_jump_end_time_ms_{0}; // 上一次抬刀结束时间
    unit_t servoing_length_before_jump_{
        0.0}; // 抬刀前的伺服位置 (加工方向上的长度), 用于计算down目标点,
              // 以及抬刀缓冲段控制
    axis_t jump_up_target_pos_;

    // TODO 高抬刀

    // G01的轨迹 (直线描述) // 直线的可过原点抬刀目前先不依赖此轨迹描述对象,
    // 后面可能还要给一个可跨过起点回退的属性或轨迹描述方法
    TrajectoryLinearSegement::ptr line_traj_;

    // 当前坐标在 Base 类中

    // 从起始点起的最大抬刀高度, 外层计算好以后传入
    // 用于计算抬刀目标点
    unit_t max_jump_height_from_begin_; //! unit: blu

    // 抬刀加减速控制
    PointMoveHandler jump_pm_handler_;

private:
    bool back_to_begin_when_pause_{false};
    bool back_to_begin_when_stop_{false};

private:
    // 获取实际驱动器坐标的回调函数, 用于闭环控制
    // std::function<bool(axis_t &)> cb_get_real_axis_;

    // 获取状态机缓存的抬刀参数
    std::function<void(JumpParam &)> cb_get_jump_param_;

    // 抬刀操作电压位回调
    std::function<void(bool)> cb_enable_votalge_gate_;
    // 上一次发送的时间点 (ms)
    std::chrono::high_resolution_clock::time_point
        last_send_enable_votalge_gate_time_;

    // 高频使能回调 (做在Motion内部更方便, 更好是做在外面, 但是判断复杂)
    std::function<void(bool)> cb_mach_on_;
};

} // namespace move

} // namespace edm