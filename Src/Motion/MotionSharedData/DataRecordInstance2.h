#pragma once

#include "DataRecordInstance.h"
#include "Motion/MoveDefines.h"

namespace edm {
namespace move {

struct RecordData2 {
    // 本地us时间
    uint64_t thread_tick_us{0};

    // 周期结束时新的指令位置 (S轴)
    move::unit_t new_cmd_axis_s{0.0};

    // 周期开始时获取的实际位置(S轴)(上一周期给出指令后的执行状态)
    move::unit_t act_axis_s{0.0};

    int is_drilling{0}; // 0 未开始 1 开始 2 暂停
    bool kn_detected{false};
    bool breakout_detected{false};
    
    bool detect_started {false};
    int detect_state {0};

    // 穿透检测算法数据
    int realtime_voltage{-1};
    int averaged_voltage{-1};
    double kn{0.0}; // 标准差 值
    double kn_valid_rate{0.0};
    int kn_cnt{0};

    inline void clear() {
        thread_tick_us = 0;

        new_cmd_axis_s = 0.0;
        act_axis_s = 0.0;

        is_drilling = 0;
        kn_detected = false;
        breakout_detected = false;

        detect_started = false;
        detect_state = 0;

        realtime_voltage = -1;
        averaged_voltage = -1;
        kn = 0.0;
        kn_valid_rate = 0.0;
        kn_cnt = 0;
    }
};

class DataRecordInstance2 final : public DataRecordInstanceBase<RecordData2> {
public:
    DataRecordInstance2(const QString &bin_dir, const QString &decode_dir)
        : DataRecordInstanceBase("Data2", bin_dir, decode_dir) {}

    std::string generate_data_header() const override;

    std::string bin_to_string(const RecordData2 &bin) const override;

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    void set_drill_param(const move::DrillParams &drill_params)
    { current_drill_params_opt_ = drill_params; }

private:
    std::optional<move::DrillParams> current_drill_params_opt_; 
    // 用于打印header, 记录算法参数(要求记录开始后不更改参数)
#endif
};

} // namespace move
} // namespace edm