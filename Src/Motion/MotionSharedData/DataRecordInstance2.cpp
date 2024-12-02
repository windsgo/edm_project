#include "DataRecordInstance2.h"

namespace edm {
namespace move {

std::string DataRecordInstance2::generate_data_header() const {
    std::stringstream ss;

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    if (this->current_drill_params_opt_) {

        const auto &drill_params = this->current_drill_params_opt_.value();

        ss << "DrillParams:" << '\n';
        ss << "touch_return_um: " << drill_params.touch_return_um << '\n';
        ss << "touch_speed_um_ms: " << drill_params.touch_speed_um_ms << '\n';
        
        const auto& bo_params = drill_params.breakout_params;

        ss << "BreakOutParams:" << '\n';
        ss << "voltage_average_filter_window_size: " << bo_params.voltage_average_filter_window_size << '\n';
        ss << "stderr_filter_window_size: " << bo_params.stderr_filter_window_size << '\n';
        ss << "kn_valid_threshold: " << bo_params.kn_valid_threshold << '\n';
        ss << "kn_sc_window_size: " << bo_params.kn_sc_window_size << '\n';
        ss << "kn_valid_rate_threshold: " << bo_params.kn_valid_rate_threshold << '\n';
        ss << "kn_valid_rate_ok_cnt_threshold: " << bo_params.kn_valid_rate_ok_cnt_threshold << '\n';
        ss << "kn_valid_rate_ok_cnt_maximum: " << bo_params.kn_valid_rate_ok_cnt_maximum << '\n';

        ss << "max_move_um_after_breakout_start_detected: " << bo_params.max_move_um_after_breakout_start_detected << '\n';
        ss << "breakout_start_detect_length_percent: " << bo_params.breakout_start_detect_length_percent << '\n';
        ss << "speed_rate_after_breakout_start_detected: " << bo_params.speed_rate_after_breakout_start_detected << '\n';
        ss << "wait_time_ms_after_breakout_end_judged: " << bo_params.wait_time_ms_after_breakout_end_judged << '\n';

        ss << "ctrl_flags: " << bo_params.ctrl_flags << '\n';
    }
#endif

    ss << "tick_us" << '\t' << "cmd_s" << '\t' << "act_s" << '\t';

    ss << "is_drilling" << '\t' << "kn_detected" << '\t' << "breakout_detected" << '\t';

    ss << "detect_started" << '\t' << "detect_state" << '\t';

    ss << "realtime_voltage" << '\t' << "averaged_voltage" << '\t' << "kn" << '\t'
       << "kn_valid_rate" << '\t' << "kn_cnt"; // << '\n';

    return ss.str();
}

std::string DataRecordInstance2::bin_to_string(const RecordData2 &bin) const {
    std::stringstream ss;

    ss << bin.thread_tick_us << '\t' << bin.new_cmd_axis_s << '\t' << bin.act_axis_s << '\t';

    ss << bin.is_drilling << '\t' << bin.kn_detected << '\t' << bin.breakout_detected << '\t';

    ss << bin.detect_started << '\t' << bin.detect_state << '\t';

    ss << bin.realtime_voltage << '\t' << bin.averaged_voltage << '\t' << bin.kn << '\t'
       << bin.kn_valid_rate << '\t' << bin.kn_cnt; // << '\n';

    return ss.str();
}

} // namespace move
} // namespace edm