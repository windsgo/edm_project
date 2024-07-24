#pragma once

#include <cstdint>

// define UDP message struct received from ZYNQ board

namespace edm {

namespace zynq {

struct adc_data {
    uint16_t original_data_0_1023; // 原始AD采样数字数据 0~1023
    int16_t voltage_5v_times_1000; // 从原始AD采样转化到的-5~5V的AD板输入电压
    int32_t voltage_real_times_1000; // 转化为分压板输入的真实极间电压
};

struct servo_msg {
    struct adc_data realtime_adc_data; // 获取数据时PL端实时采集到的瞬时电压
    struct adc_data pl_filtered_adc_data; // PL端(ns~us级别)滤波的电压
    struct adc_data ps_filtered_adc_data; // PS端(宏观ms级)滤波的电压
    
    // TODO
};


// The Total Udp Message
struct udp_message {
    uint32_t input_io; // TODO, if there is input io

    // TODO other udp contents
};

}

}