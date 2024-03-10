#include "G01AutoTask.h"

namespace edm {

namespace move {
bool G01AutoTask::pause() { return false; }

bool G01AutoTask::resume() { return false; }

bool G01AutoTask::stop(bool immediate) { return false; }

bool G01AutoTask::is_normal_running() const { return false; }

bool G01AutoTask::is_pausing() const { return false; }

bool G01AutoTask::is_paused() const { return false; }

bool G01AutoTask::is_resuming() const { return false; }

bool G01AutoTask::is_stopping() const { return false; }

bool G01AutoTask::is_stopped() const { return false; }

bool G01AutoTask::is_over() const { return false; }

void G01AutoTask::run_once() {}

} // namespace move

} // namespace edm