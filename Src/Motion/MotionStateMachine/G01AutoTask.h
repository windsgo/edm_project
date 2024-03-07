#pragma once

#include "Motion/MotionUtils/MotionUtils.h"
#include "MotionAutoTask.h"

namespace edm {

namespace move {

//! G01AutoTask 之后可能要作为一组G01多段加工的单个单元
//! 实现时，要注意这一点
class G01AutoTask : public AutoTask {
public:
    G01AutoTask(const axis_t &init_axis, const axis_t &target_axis,
                const std::function<bool(axis_t)> &cb_get_real_axis)
        : AutoTask(AutoTaskType::G01, init_axis), start_pos_(init_axis),
          target_pos_(target_axis),
          target_length_(MotionUtils::CalcAxisLength(init_axis, target_axis)),
          curr_length_(0),
          cb_get_real_axis_(cb_get_real_axis) {}

    bool pause() override;
    bool resume() override;
    bool stop(bool immediate = false) override;

    bool is_normal_running() const override ;
    bool is_pausing() const override ;
    bool is_paused() const override ;
    bool is_resuming() const override ;
    bool is_stopping() const override ;
    bool is_stopped() const override ;
    bool is_over() const override ;

    void run_once() override;

public: // custom interfaces


public:
    // G01 内部的 running_state, 可能比外层复杂, 还会涉及到抬刀,
    // 暂停回加工起点等
    enum class State {

    };

private:
    // G01 task 就在这个类里面实现吧
    axis_t start_pos_;
    axis_t target_pos_;

    unit_t target_length_;
    unit_t curr_length_;

    // 当前坐标在 Base 类中

    std::function<bool(axis_t)> cb_get_real_axis_;
};

} // namespace move

} // namespace edm