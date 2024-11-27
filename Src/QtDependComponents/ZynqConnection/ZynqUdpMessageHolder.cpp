#include "ZynqUdpMessageHolder.h"
#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "QtDependComponents/ZynqConnection/UdpMessageDefine.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
#include "Utils/Format/edm_format.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include <functional>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace zynq {

ZynqUdpMessageHolder::ZynqUdpMessageHolder(
    ZynqConnectController::ptr zynq_ctrler)
    : zynq_ctrler_(zynq_ctrler) {
    _init_udp_listener();
}

void ZynqUdpMessageHolder::_init_udp_listener() {
    zynq_ctrler_->add_udp_listener(
        std::bind_front(&ZynqUdpMessageHolder::_udp_listener_cb, this));
}

void ZynqUdpMessageHolder::_udp_listener_cb(const QByteArray &ba) {
    //! this call-back cb is called in the Zynq Controller Thread

    auto udp_message_ptr =
        reinterpret_cast<const servo_return_data_t *>(ba.data() + 4);

    // do convert
    servo_return_converted_data_t temp;
    temp.averaged_voltage = (double)udp_message_ptr->averaged_voltage_times_10 / 10.0;
    temp.realtime_voltage = (double)udp_message_ptr->realtime_voltage_times_10 / 10.0;
    temp.servo_calced_speed_mm_min = (double)udp_message_ptr->servo_calced_speed_mm_min_times_1000 / 1000.0;
    temp.touch_detected = temp.realtime_voltage <= 3;

    // atomic operation
    at_udp_message_converted_cached_.store(temp);
}

void ZynqUdpMessageHolder::get_udp_message(servo_return_converted_data_t &output) const {
    // atomic operation
    output = at_udp_message_converted_cached_.load();

#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    // 离线随机伺服指令发生
    double amplitude_um = this->manual_servo_cmd_feed_amplitude_um_;
    double probability_offset =
        (this->manual_servo_cmd_feed_probability_ - 0.50);

    double rd = uniform_real_distribution_(gen_); // rd 在 -1.0, 1.0之间正太分布

    // 根据 probability_offset 进行概率偏移
    rd += probability_offset * 2;
    if (rd > 1.0)
        rd = 1.0;
    else if (rd < -1.0)
        rd = -1.0;

    output.servo_calced_speed_mm_min = util::UnitConverter::um_ms2mm_min(rd * amplitude_um);
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    output.touch_detected = this->manual_touch_detect_flag_;
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

#ifdef EDM_OFFLINE_MANUAL_VOLTAGE
    output.averaged_voltage = this->manual_voltage_value_;
#endif // EDM_OFFLINE_MANUAL_VOLTAGE
}

} // namespace zynq
} // namespace edm