#include "SpindleControl.h"
#include <qglobal.h>

namespace edm {
namespace move {

void SpindleController::run_once() {
    if (spindle_on_) {
        _acc();
    } else {
        _dec();
    }
}

void SpindleController::_acc() {
    Q_ASSERT(spindle_current_speed_blu_per_peroid_ >= 0);

    if (spindle_current_speed_blu_per_peroid_ <
        spindle_target_speed_blu_per_peroid_) {
        // 继续加速
        spindle_current_speed_blu_per_peroid_ +=
            spindle_max_acc_blu_per_peroid_;

        // 不超过最大速度
        if (spindle_current_speed_blu_per_peroid_ >
            spindle_target_speed_blu_per_peroid_) {
            spindle_current_speed_blu_per_peroid_ =
                spindle_target_speed_blu_per_peroid_;
        }
    }

    // 位置控制, 加上当前周期的速度
    spindle_axis_current_axis_ += spindle_current_speed_blu_per_peroid_;
}

void SpindleController::_dec() {
    Q_ASSERT(spindle_current_speed_blu_per_peroid_ >= 0);

    if (spindle_current_speed_blu_per_peroid_ > 0) {
        // 继续减速
        spindle_current_speed_blu_per_peroid_ -=
            spindle_max_acc_blu_per_peroid_;

        // 不低于0
        if (spindle_current_speed_blu_per_peroid_ < 0) {
            spindle_current_speed_blu_per_peroid_ = 0;
        }
    }

    if (spindle_current_speed_blu_per_peroid_ > 0) {
        // 位置控制, 加上当前周期的速度
        spindle_axis_current_axis_ += spindle_current_speed_blu_per_peroid_;
    }
}

} // namespace move
} // namespace edm