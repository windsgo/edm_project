#include "MotionSharedData.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"
#include "Utils/UnitConverter/UnitConverter.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

MotionSharedData::MotionSharedData() {
    data_record_instance1_ = std::make_shared<DataRecordInstance1>(
        RecordData1BinDir, RecordData1DecodeDir);

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    data_record_instance2_ = std::make_shared<DataRecordInstance2>(
        RecordData1BinDir, RecordData1DecodeDir);
    data_record_instance2_->set_drill_param(drill_params_);
#endif

    MotionUtils::ClearAxis(global_cmd_axis_);
    MotionUtils::ClearAxis(global_v_offsets_);

    for (size_t i = 0; i < EDM_SERVO_NUM; ++i) {
        gear_ratios_[i] = 1.0;

        global_v_offsets_forced_zero_[i] = false; // 默认不强制为0
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    gear_ratios_[EDM_DRILL_S_AXIS_IDX] = 100.0; // S轴100倍

    spindle_control_ = std::make_shared<SpindleController>();

    breakout_filter_ = std::make_shared<util::BreakoutFilter>();
    breakout_filter_->set_breakout_params(drill_params_.breakout_params);
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
}

void MotionSharedData::set_global_cmd_axis(const axis_t &cmd_axis) {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    // 检查位置是否有突变
    static axis_t prev_cmd_axis;
    static bool flag = false;
    static const double diff_length_threshold =
        util::UnitConverter::um2blu(100);

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
    if (!ecat_manager_->is_ecat_connected()) {
        move::MotionUtils::ClearAxis(axis);
        return axis;
    }

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] =
            (double)(this->ecat_manager_->get_servo_actual_position(i)) / gear_ratios_[i];
    }

    return axis;
#endif                             // EDM_OFFLINE_RUN_NO_ECAT
}

bool MotionSharedData::get_act_axis(axis_t &axis) const {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    axis = this->global_cmd_axis_; // 离线, 返回指令位置
#else                              // EDM_OFFLINE_RUN_NO_ECAT
    if (!ecat_manager_->is_ecat_connected()) {
        return false;
    }

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] =
            (double)(this->ecat_manager_->get_servo_actual_position(i)) / gear_ratios_[i];
    }
#endif                             // EDM_OFFLINE_RUN_NO_ECAT
    return true;
}

void MotionSharedData::set_g01_speed_ratio(double ratio) {
    // if (ratio < 0.0) {
    //     s_logger->error("G01 speed ratio must be no less than 0, set to 0.0");
    //     ratio = 0.0;
    // }
    g01_speed_ratio_ = ratio;
}

} // namespace move

} // namespace edm
