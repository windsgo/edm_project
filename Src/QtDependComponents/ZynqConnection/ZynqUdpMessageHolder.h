#pragma once

#include "ZynqConnectController.h"
#include "UdpMessageDefine.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include "config.h"
#include <atomic>
#include <memory>
#include <random>


namespace edm {
namespace zynq {

class ZynqUdpMessageHolder {
public:
    using ptr = std::shared_ptr<ZynqUdpMessageHolder>;

public:
    ZynqUdpMessageHolder(ZynqConnectController::ptr zynq_ctrler);

    void get_udp_message(servo_return_converted_data_t& output) const;

private:
    void _init_udp_listener();
    
    void _udp_listener_cb(const QByteArray& ba);

private:
    ZynqConnectController::ptr zynq_ctrler_;

    std::atomic<servo_return_converted_data_t> at_udp_message_converted_cached_;

/**************************/

private: // 用于离线测试时的随机数发生器
#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    // 离线测试时, 给此类输入一个幅值和进给概率, 每次调用, 随机给出一个进给量
    std::atomic<double> manual_servo_cmd_feed_probability_ {0.75}; // 进给概率
    std::atomic<double> manual_servo_cmd_feed_amplitude_um_ {0.5}; // 幅值
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

    mutable std::random_device random_device_;
    mutable std::mt19937 gen_;
    mutable std::uniform_real_distribution<> uniform_real_distribution_;

#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    std::atomic_bool manual_touch_detect_flag_{false};
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

public:
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    inline void set_manual_touch_detect_flag(bool detected) {
        manual_touch_detect_flag_ = detected;
    }
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    inline void set_manual_servo_cmd(double feed_probability, double feed_amplitude_um) {
        manual_servo_cmd_feed_probability_ = feed_probability;
        manual_servo_cmd_feed_amplitude_um_ = feed_amplitude_um;
    }
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

#ifdef EDM_OFFLINE_MANUAL_VOLTAGE
    // 离线手动设置电压
private:
    std::atomic_uint16_t manual_voltage_value_ {10};
public:
    inline void set_manual_voltage(uint16_t voltage) { manual_voltage_value_.store(voltage); }
#endif // EDM_OFFLINE_MANUAL_VOLTAGE
};

}
}
