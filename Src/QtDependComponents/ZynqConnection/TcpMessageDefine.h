#pragma once

#include "config.h"
#include <cstdint>
namespace edm {

namespace zynq {

// define Tcp message struct sent to & received from ZYNQ board
typedef enum {
    // 请求回复(可用于连接、心跳检测， 或错误返回, 见CommReplyErr_t)
    COMM_FRAME_REPLY = 0,

    // 设定伺服参数
    COMM_FRAME_SET_SERVO_SETTINGS = 1,

    // 设定ADC标定参数
    COMM_FRAME_SET_ADC_SETTINGS = 2,

    COMM_FRAME_MAX
} CommFrameId_t;

typedef enum {
    COMM_REPLY_ERR_OK = 0,
    COMM_REPLY_ERR_CRC = -1,
    COMM_REPLY_ERR_HEADER = -2,
    COMM_REPLY_ERR_INVALID_CMD = -3,
} CommReplyErr_t;

// 设定参数结构体

// ADC回归参数, 从-5~5V回归到真实电压
// v_real = gain * v_sample + offset
typedef struct {
    int16_t adc_offset;
    int16_t adc_gain;
    uint32_t voltage_filter_window_time_us; // 平均电压滤波时间(us) ->
                                            // 会折算到滤波窗口大小
} upper_adc_settings_t;

// 用于伺服的参数
typedef struct {
    uint32_t servo_speed; // 伺服速度设定 (mm/min)

    uint16_t servo_sensitivity; // 伺服灵敏度 (0~100)
    uint16_t servo_ref_voltage; // 伺服参考电压 (0~300)

    uint16_t servo_voltage_diff_level; // 伺服电压区间档位分组
    uint16_t _reserved;                // 保留, 用于对齐结构体
} upper_servo_settings_t;

// 主要参数设定(上位机下发)
// typedef struct {
//     // adc回归设定 -> 用于从-5~5V计算真实的电压
//     upper_adc_settings_t adc_settings;

//     // 伺服设定
//     upper_servo_settings_t servo_settings;
// } upper_settings_t;

} // namespace zynq

} // namespace edm