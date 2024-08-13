#pragma once

#include "config.h"
#include <cstdint>

// define UDP message struct received from ZYNQ board

namespace edm {

namespace zynq {

// UDP返回结构体
typedef struct {
    // PL端回传的滤波后的电压(可视作实时电压, 可用于接触感知等)
    int16_t realtime_voltage;

    // PS端滑动滤波之后的平均电压
    int16_t averaged_voltage;

    // 计算出的伺服进给速度(mm/min) * 1000 返回  (负数表示回退)
    int32_t servo_calced_speed_mm_min_times_1000;
} servo_return_data_t;

} // namespace zynq

} // namespace edm