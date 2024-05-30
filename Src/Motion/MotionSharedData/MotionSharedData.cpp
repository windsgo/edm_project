#include "MotionSharedData.h"

namespace edm {

namespace move {

axis_t MotionSharedData::get_act_axis() const {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    return this->global_cmd_axis_; // 离线, 返回指令位置
#else                              // EDM_OFFLINE_RUN_NO_ECAT
    axis_t axis;

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] = this->ecat_manager_->get_servo_actual_position(i);
    }

    return axis;
#endif                             // EDM_OFFLINE_RUN_NO_ECAT
}

void MotionSharedData::get_act_axis(axis_t &axis) const {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    axis = this->global_cmd_axis_; // 离线, 返回指令位置
#else                              // EDM_OFFLINE_RUN_NO_ECAT
    for (int i = 0; i < axis.size(); ++i) {
        axis[i] = this->ecat_manager_->get_servo_actual_position(i);
    }
#endif                             // EDM_OFFLINE_RUN_NO_ECAT
}

} // namespace move

} // namespace edm
