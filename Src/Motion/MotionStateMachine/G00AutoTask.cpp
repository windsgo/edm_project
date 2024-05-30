#include "G00AutoTask.h"
#include "Motion/MotionSharedData/MotionSharedData.h"

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

G00AutoTask::G00AutoTask(const axis_t &target_axis,
                         const MoveRuntimePlanSpeedInput &speed_param,
                         bool enable_touch_detect,
                         TouchDetectHandler::ptr touch_detect_handler)
    : AutoTask(AutoTaskType::G00), enable_touch_detect_(enable_touch_detect),
      touch_detect_handler_(touch_detect_handler) {
    pm_handler_.start(speed_param, s_motion_shared->get_global_cmd_axis(),
                      target_axis);
}

bool G00AutoTask::pause() { return pm_handler_.pause(); }

bool G00AutoTask::resume() { return pm_handler_.resume(); }

bool G00AutoTask::stop(bool immediate) { return pm_handler_.stop(immediate); }

void G00AutoTask::run_once() {
    //! 每运行一次都设置一下, 防止暂停后全局接触感知状态被改变
    touch_detect_handler_->set_detect_enable(enable_touch_detect_);

    if (touch_detect_handler_->run_detect_once()) {
        pm_handler_.stop(true);
    }

    pm_handler_.run_once();

    // curr_cmd_axis_ = pm_handler_.get_current_pos();
    s_motion_shared->set_global_cmd_axis(pm_handler_.get_current_pos());

    //! 如果不在运行, 就置为默认false即可, 只是显示用
    if (pm_handler_.is_stopped() || pm_handler_.is_paused()) {
        touch_detect_handler_->set_detect_enable(false);
    }
}

} // namespace move

} // namespace edm