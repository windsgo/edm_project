#pragma once

#include "Motion/MoveruntimeWrapper/MoveruntimeWrapper.h"

#include <memory>

namespace edm
{

namespace move
{

class PointMoveHandler final {
public:
    using ptr = std::shared_ptr<PointMoveHandler>;
    PointMoveHandler();
    ~PointMoveHandler() = default;

    bool start(const MoveRuntimePlanSpeedInput& speed_param,
        const axis_t& start_pos,
        const axis_t& target_pos);
    
    bool pause();
    bool resume();
    bool stop(bool immediate = false);
    bool change_speed(unit_t new_speed);

    void clear();

    //! call `get_current_pos` or 
    void run_once();

    // 绝对坐标获取
    const axis_t& get_current_pos() const { return curr_pos_; }
    const axis_t& get_start_pos() const { return start_pos_; };
    const axis_t& get_target_pos() const { return target_pos_; }

    // 获取单位方向向量
    const axis_t& get_unit_vector() const { return unit_vector_; }

    // 长度相对增量
    unit_t get_current_length() const { return curr_length_; }

    // 总长度值
    unit_t get_target_length() const { return target_length_; }

    inline auto state() const { return mrt_wrapper_.state(); }
    inline bool is_started() const { return mrt_wrapper_.is_started(); }
    inline bool is_normal_running() const { return mrt_wrapper_.is_normal_running(); }
    inline bool is_paused() const { return mrt_wrapper_.is_paused(); }
    inline bool is_stopped() const { return mrt_wrapper_.is_stopped(); }
    inline bool is_over() const { return mrt_wrapper_.is_over(); }
 
private:
    MoveruntimeWrapper mrt_wrapper_;

    axis_t start_pos_; // const after `start`
    axis_t target_pos_; // const after `start`

    // 当前位置, 每周期结束使用 `curr_length_` 和 `unit_vector_` 计算
    // update after `run_once`
    axis_t curr_pos_; 
    
    axis_t unit_vector_; // 单位方向向量

    unit_t target_length_; // const after `start`

    // 单独放一个curr_length_, 目的是在最后一步时, 将curr_length_直接置为target_length_
    // 减小舍入误差, 这在 moveruntime_wrapper 里是没有做的, 这里做一下
    unit_t curr_length_;
};

} // namespace move

} // namespace edm
