#pragma once

#include "MotionAutoTask.h"

namespace edm {

namespace move {

class G00AutoTask : public AutoTask {
public:
    G00AutoTask(const axis_t &target_axis,
                const MoveRuntimePlanSpeedInput &speed_param,
                bool enable_touch_detect,
                TouchDetectHandler::ptr touch_detect_handler);

    bool pause() override;
    bool resume() override;
    bool stop(bool immediate = false) override;

    bool is_normal_running() const override {
        return pm_handler_.is_normal_running();
    }
    bool is_pausing() const override { return pm_handler_.is_pausing(); }
    bool is_paused() const override { return pm_handler_.is_paused(); }
    bool is_resuming() const override { return is_normal_running(); }
    bool is_stopping() const override { return pm_handler_.is_stopping(); }
    bool is_stopped() const override { return pm_handler_.is_stopped(); }
    bool is_over() const override { return pm_handler_.is_over(); }

    void run_once() override;

private:
    TouchDetectHandler::ptr
        touch_detect_handler_; // 用于给G00任务的接触感知控制体

    PointMoveHandler pm_handler_;

    bool enable_touch_detect_;
};

} // namespace move

} // namespace edm
