#include "DataRecordInstance1.h"

namespace edm {
namespace move {

std::string DataRecordInstance1::generate_data_header() const {
    std::stringstream ss;

    ss << "tick_us" << '\t';

    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << "cmd" << i << '\t';
    }
    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << "act" << i << '\t';
    }
    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << "err" << i << '\t';
    }
    ss << "servocmd" << '\t' << "isg01" << '\t' << "avgvol" << '\t' << "current"
       << '\t' << "normal" << '\t' << "short" << '\t' << "open";

    return ss.str();
}

std::string DataRecordInstance1::bin_to_string(const RecordData1 &bin) const {
    std::stringstream ss;

    ss << bin.thread_tick_us << '\t';

    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << bin.new_cmd_axis[i] << '\t';
    }
    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << bin.act_axis[i] << '\t';
    }
    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << bin.following_error_axis[i] << '\t';
    }
    ss << bin.g01_servo_cmd << '\t' << (int)bin.is_g01_normal_servoing << '\t'
       << (int)bin.average_voltage << '\t' << (int)bin.current << '\t'
       << (int)bin.normal_charge_rate << '\t' << (int)bin.short_charge_rate
       << '\t' << (int)bin.open_charge_rate ; // << '\n';

    return ss.str();
}

} // namespace move
} // namespace edm