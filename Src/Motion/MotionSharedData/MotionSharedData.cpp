#include "MotionSharedData.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"
#include "Utils/UnitConverter/UnitConverter.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

void MotionSharedData::set_global_cmd_axis(const axis_t &cmd_axis) {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    // 检查位置是否有突变
    static axis_t prev_cmd_axis;
    static bool flag = false;
    static const double diff_length_threshold = util::UnitConverter::um2blu(100);

    prev_cmd_axis = cmd_axis;

    if (flag) [[likely]] {
        auto diff_cmd_vec =
            move::MotionUtils::CalcAxisVector(prev_cmd_axis, cmd_axis);
        
        auto diff_length = move::MotionUtils::CalcAxisLength(diff_cmd_vec);

        if (diff_length > diff_length_threshold) {
            s_logger->warn("diff_length: {}", diff_length);
        }
    } else {
        flag = true;
    }

#endif // EDM_OFFLINE_RUN_NO_ECAT

    global_cmd_axis_ = cmd_axis;
}

axis_t MotionSharedData::get_act_axis() const {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    return this->global_cmd_axis_; // 离线, 返回指令位置
#else                              // EDM_OFFLINE_RUN_NO_ECAT
    axis_t axis;
    if (!ecat_manager_->is_ecat_connected()) 
    {
        move::MotionUtils::ClearAxis(axis);
        return axis;
    }

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] = this->ecat_manager_->get_servo_actual_position(i);
    }

    return axis;
#endif                             // EDM_OFFLINE_RUN_NO_ECAT
}

bool MotionSharedData::get_act_axis(axis_t &axis) const {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    axis = this->global_cmd_axis_; // 离线, 返回指令位置
#else                              // EDM_OFFLINE_RUN_NO_ECAT
    if (!ecat_manager_->is_ecat_connected()) 
    {
        return false;
    }

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] = this->ecat_manager_->get_servo_actual_position(i);
    }
#endif                             // EDM_OFFLINE_RUN_NO_ECAT
    return true;
}

} // namespace move

} // namespace edm
