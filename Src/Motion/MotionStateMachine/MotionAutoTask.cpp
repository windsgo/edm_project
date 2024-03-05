#include "MotionAutoTask.h"

namespace edm {

namespace move {

bool G00AutoTask::pause() { return pm_handler_.pause(); }

bool G00AutoTask::resume() { return pm_handler_.resume(); }

bool G00AutoTask::stop(bool immediate) { return pm_handler_.stop(immediate); }

void G00AutoTask::run_once() {
    //! 每运行一次都设置一下
    touch_detect_handler_->set_detect_enable(enable_touch_detect_);

    if (touch_detect_handler_->run_detect_once()) {
        pm_handler_.stop(true);
    }

    pm_handler_.run_once();

    curr_cmd_axis_ = pm_handler_.get_current_pos();
}

} // namespace move

} // namespace edm
