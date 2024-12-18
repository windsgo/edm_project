#pragma once

#include "Motion/MoveDefines.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include <memory>

namespace edm {
namespace move {

class SpindleController {
public:
    using ptr = std::shared_ptr<SpindleController>;
    SpindleController() = default;
    ~SpindleController() = default;

    void init_current_axis(unit_t current_axis) {
        spindle_axis_current_axis_ = current_axis;
    }

    void start_spindle() { spindle_on_ = true; }
    void stop_spindle() { spindle_on_ = false; }

    void set_spindle_target_speed_blu_ms(unit_t speed_blu_ms) {
        spindle_target_speed_blu_per_peroid_ =
            util::UnitConverter::blu_ms2blu_p(speed_blu_ms);

        if (spindle_target_speed_blu_per_peroid_ < 0.0) {
            spindle_target_speed_blu_per_peroid_ = 0.0;
        }
    }

    void set_spindle_max_acc_blu_ms(unit_t acc_blu_ms) {
        spindle_max_acc_blu_per_peroid_ =
            util::UnitConverter::blu_ms2blu_p(acc_blu_ms);
    }

    void run_once();

public:
    auto is_spindle_on() const { return spindle_on_; }
    auto current_axis() const { return spindle_axis_current_axis_; }

    auto current_speed_blu_per_peroid() const {
        return spindle_current_speed_blu_per_peroid_;
    }

    auto target_speed_blu_per_peroid() const {
        return spindle_target_speed_blu_per_peroid_;
    }

private:
    void _acc();
    void _dec();

private:
    // 主轴旋转控制
    bool spindle_on_{false}; // 主轴开关
    // 当前主轴旋转坐标(需要在初始化ethercat后进行初始化)
    // (用double类型以控制速度)
    unit_t spindle_axis_current_axis_{0};
    // 主轴速度
    unit_t spindle_current_speed_blu_per_peroid_{0};
    // 主轴设定目标速度
    unit_t spindle_target_speed_blu_per_peroid_{util::UnitConverter::blu_ms2blu_p(40)};
    // 主轴最大加速度 (每周期速度的最大改变量) 默认每毫秒变化1个blu
    unit_t spindle_max_acc_blu_per_peroid_{
        util::UnitConverter::blu_ms2blu_p(1)};
};

} // namespace move
} // namespace edm