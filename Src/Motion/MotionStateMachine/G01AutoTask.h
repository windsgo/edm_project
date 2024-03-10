#pragma once

#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "MotionAutoTask.h"

#include "Motion/Trajectory/TrajectorySegement.h"

#include "Motion/JumpDefines.h"

namespace edm {

namespace move {

class G01AutoTask : public AutoTask {
public:
    G01AutoTask(TrajectoryLinearSegement::ptr line_traj,
                unit_t max_jump_height_from_begin,
                const std::function<bool(axis_t &)> &cb_get_real_axis,
                const std::function<unit_t(void)> &cb_get_servo_cmd,
                const std::function<void(JumpParam &)> &cb_get_jump_param,
                const std::function<void(bool)> &cb_enable_votalge_gate)
        : AutoTask(AutoTaskType::G01, line_traj->start_pos()),
          line_traj_(line_traj),
          max_jump_height_from_begin_(max_jump_height_from_begin),
          cb_get_real_axis_(cb_get_real_axis),
          cb_get_servo_cmd_(cb_get_servo_cmd),
          cb_get_jump_param(cb_get_jump_param),
          cb_enable_votalge_gate_(cb_enable_votalge_gate) {}

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

    enum class ServoSubState {
        // 伺服中, 在暂停中和停止中时, 要等待返回伺服中的子状态
        Servoing,

        JumpUping,   // 抬刀回退
        JumpDowning, // 抬刀前进到缓冲距离之前

        // 缓冲距离以伺服方式进行前进, 如果发生短路回退指令或缓冲段已走完,
        // 则退出缓冲状态, 转入正常NormalRunning
        JumpDowningBuffer,
    };

private:
    State state_{State::NormalRunning};
    ServoSubState sub_state_{ServoSubState::Servoing};

    JumpParam jumping_param_;

    // G01的轨迹 (直线描述)
    TrajectoryLinearSegement::ptr line_traj_;

    // 当前坐标在 Base 类中

    // 从起始点起的最大抬刀高度, 外层计算好以后传入
    // 用于计算抬刀目标点
    unit_t max_jump_height_from_begin_;

    // 抬刀加减速控制
    PointMoveHandler jump_pm_handler_;

private:
    // 获取实际驱动器坐标的回调函数, 用于闭环控制
    std::function<bool(axis_t &)> cb_get_real_axis_;

    // 获取当前可用的伺服指令 (直接返回一个double值, 包含正负)
    std::function<unit_t(void)> cb_get_servo_cmd_;

    // 获取状态机缓存的抬刀参数
    std::function<void(JumpParam &)> cb_get_jump_param;

    // 抬刀操作电压位回调
    std::function<void(bool)> cb_enable_votalge_gate_;
};

} // namespace move

} // namespace edm