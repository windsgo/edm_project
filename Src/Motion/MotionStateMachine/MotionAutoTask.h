#pragma once

#include <memory>

#include "Motion/MoveDefines.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "Motion/TouchDetectHandler/TouchDetectHandler.h"

namespace edm {

namespace move {

enum class AutoTaskType {
    Unknow,
    G00,
    G01,
    // TODO
};

class AutoTask {
public:
    using ptr = std::shared_ptr<AutoTask>;
    AutoTask(AutoTaskType type, const axis_t &init_axis) noexcept
        : type_(type), curr_cmd_axis_(init_axis) {}
    virtual ~AutoTask() noexcept = default;

    AutoTaskType type() const { return type_; }

    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool stop(bool immediate = false) = 0;

    virtual bool is_normal_running() const = 0;
    virtual bool is_pausing() const = 0;
    virtual bool is_paused() const = 0;
    virtual bool is_resuming() const = 0;
    virtual bool is_stopping() const = 0;
    virtual bool is_stopped() const = 0;

    virtual bool is_over() const { return is_over(); }

    // 运行
    virtual void run_once() = 0;

    // 坐标相关
    const axis_t &get_curr_cmd_axis() const { return curr_cmd_axis_; }

private:
    AutoTaskType type_;

protected:
    axis_t curr_cmd_axis_;
};

class G00AutoTask : public AutoTask {
public:
    G00AutoTask(const axis_t &init_axis, const axis_t &target_axis,
                const MoveRuntimePlanSpeedInput &speed_param,
                bool enable_touch_detect,
                TouchDetectHandler::ptr touch_detect_handler)
        : AutoTask(AutoTaskType::G00, init_axis),
          enable_touch_detect_(enable_touch_detect),
          touch_detect_handler_(touch_detect_handler) {
        pm_handler_.start(speed_param, init_axis, target_axis);
    }

    bool pause();
    bool resume();
    bool stop(bool immediate = false);

    bool is_normal_running() const { return pm_handler_.is_normal_running(); }
    bool is_pausing() const { return pm_handler_.is_pausing(); }
    bool is_paused() const { return pm_handler_.is_paused(); }
    bool is_resuming() const { return is_normal_running(); }
    bool is_stopping() const { return pm_handler_.is_stopping(); }
    bool is_stopped() const { return pm_handler_.is_stopped(); }
    bool is_over() const { return pm_handler_.is_over(); }

    void run_once();

private:
    TouchDetectHandler::ptr
        touch_detect_handler_; // 用于给G00任务的接触感知控制体

    PointMoveHandler pm_handler_;

    bool enable_touch_detect_;
};

} // namespace move

} // namespace edm
