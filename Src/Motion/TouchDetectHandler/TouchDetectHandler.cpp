#include "TouchDetectHandler.h"

namespace edm {

namespace move {
TouchDetectHandler::TouchDetectHandler(
    const std::function<bool(void)> physical_touch_detect_cb)
    : physical_touch_detect_cb_(physical_touch_detect_cb) {}

void TouchDetectHandler::reset() {
    detect_enabled_ = false;
    warning_ = false;
}

bool TouchDetectHandler::run_detect_once() {
    if (warning_) {
        return true;
    }

    if (!detect_enabled_) {
        return false;
    }

    if (physical_touch_detect_cb_()) {
        warning_ = true;
        return true;
    } else {
        return false;
    }
}

} // namespace move

} // namespace edm
