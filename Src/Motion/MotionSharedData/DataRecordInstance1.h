#pragma once

#include "DataRecordInstance.h"
#include "Motion/MoveDefines.h"

namespace edm {
namespace move {

struct RecordData1 {
    // 本地us时间
    uint64_t thread_tick_us{0};

    // move::axis_t last_cmd_axis; // 周期开始时(上一周期的指令位置)

    // 周期结束时新的指令位置
    move::axis_t new_cmd_axis{0.0};

    // 周期开始时获取的实际位置(上一周期给出指令后的执行状态)
    move::axis_t act_axis{0.0};

    // 周期开始时获取驱动器返回的跟随误差值
    move::axis_t following_error_axis{0.0};

    // TODO 后续速度辅助控制, 速度值返回等记录

    // 是否为G01直线伺服加工(排除抬刀等状态)
    bool is_g01_normal_servoing{false};

    // 当前周期g01伺服指令值 (默认Z轴负方向加工)
    move::unit_t g01_servo_cmd{0.0};

    // 放电状态数据
    uint8_t normal_charge_rate{0};
    uint8_t short_charge_rate{0};
    uint8_t open_charge_rate{0};
    uint8_t current{0}; // 电流
    uint16_t average_voltage{0};

    inline void clear() {
        thread_tick_us = 0;
        new_cmd_axis.fill(0.0);
        act_axis.fill(0.0);
        following_error_axis.fill(0.0);
        is_g01_normal_servoing = false;
        g01_servo_cmd = 0.0;

        normal_charge_rate = 0;
        short_charge_rate = 0;
        open_charge_rate = 0;
        current = 0;
        average_voltage = 0;
    }
};

class DataRecordInstance1 final : public DataRecordInstanceBase<RecordData1> {
public:
    DataRecordInstance1(const QString &bin_dir, const QString &decode_dir)
        : DataRecordInstanceBase("Data1", bin_dir, decode_dir) {}

    std::string generate_data_header() const override;

    std::string bin_to_string(const RecordData1 &bin) const override;
};

} // namespace move
} // namespace edm