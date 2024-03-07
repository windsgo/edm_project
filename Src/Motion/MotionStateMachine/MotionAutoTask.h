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

    // Note: no start interface.
    // when constructed, task should be automatically inited as `started` or
    // `normal_running` state

    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool stop(bool immediate = false) = 0;

    // Note: no is_resumed interface.
    // `is_resumed` should be equivalent to `is_normal_running`.

    virtual bool is_normal_running() const = 0;
    virtual bool is_pausing() const = 0;
    virtual bool is_paused() const = 0;
    virtual bool is_resuming() const = 0;
    virtual bool is_stopping() const = 0;
    virtual bool is_stopped() const = 0;

    // Normally, `is_over` is equivalent to `is_stopped`
    // However, user can override this default implementation
    virtual bool is_over() const { return is_stopped(); }

    // 运行
    virtual void run_once() = 0;

    // get current task's axis
    // Note: the task should maintain and keep the axis in the whole running
    // routine even if the task `is_over`
    const axis_t &get_curr_cmd_axis() const { return curr_cmd_axis_; }

private:
    AutoTaskType type_;

protected:
    axis_t curr_cmd_axis_;
};

} // namespace move

} // namespace edm
