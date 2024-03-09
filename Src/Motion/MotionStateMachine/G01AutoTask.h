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
    G01AutoTask(const axis_t &init_axis, const axis_t &target_axis,
                unit_t max_jump_height_from_begin,
                const std::function<bool(axis_t)> &cb_get_real_axis,
                const std::function<unit_t(void)> &cb_get_servo_cmd;)
        : AutoTask(AutoTaskType::G01, init_axis),
          max_jump_height_from_begin_(max_jump_height_from_begin)
              cb_get_real_axis_(cb_get_real_axis) {
        //! 目前先保持和G00基本一致的传参方式,
        //! 后面Nurbs肯定要在外部初始化好传递进来
        line_traj_segement_ =
            std::make_shared<TrajectoryLinearSegement>(init, target_axis);
    }

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
    // 抬刀接口
    // 设定抬刀up, dn, 速度, 加速度, 加加速度等参数
    void set_jump_param(const JumpParam& jump_param) { jump_param_set_ = jump_param; }

public:
    // G01 内部的 running_state, 可能比外层复杂, 还会涉及到抬刀,
    // 暂停回加工起点等
    enum class State {
        NormalRunning,
        Pausing, // 如果在抬刀, 等待抬刀完成, (回到加工起点)(先不实现), 暂停
        Paused,
        Resuming, // 如果有暂停回到加工起点, 这里就是恢复加工位置
        Stopping, // 如果在抬刀, 等待抬刀完成
        Stopped,

        Jumping // 抬刀状态, 如果抬刀中暂停或停止, 先完成当前抬刀再停止
    };

    enum class JumpingState {
        Uping, // 抬刀回退
        Downing, // 抬刀前进到缓冲距离之前

        // 缓冲距离以伺服方式进行前进, 如果发生短路回退指令或缓冲段已走完, 则退出缓冲状态, 转入正常NormalRunning
        DowningBuffer 
    };

private:
    State state_ {State::NormalRunning};
    JumpingState jump_state_ {JumpingState::Uping};

    // 保存两组抬刀参数, 一组是随时设定的, 一组是抬刀时的
    // 保证一次抬刀的参数是一致的
    JumpParam jump_param_set_;
    JumpParam jumping_param_;

    // G01的轨迹 (直线描述)
    TrajectoryLinearSegement::ptr line_traj_segement_;

    // 当前坐标在 Base 类中

    // 获取实际驱动器坐标的回调函数, 用于闭环控制
    std::function<bool(axis_t)> cb_get_real_axis_;

    // 获取当前可用的伺服指令 (直接返回一个double值, 包含正负)
    std::function<unit_t(void)> cb_get_servo_cmd_;

    // 从起始点起的最大抬刀高度, 外层计算好以后传入
    // 用于计算抬刀目标点
    unit_t max_jump_height_from_begin_;

    // 抬刀加减速控制
    PointMoveHandler jump_pm_handler_;
};

} // namespace move

} // namespace edm