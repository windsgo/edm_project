#include "CanReceiveBuffer.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

static_assert(sizeof(Can1IOBoard407ServoData) == 8);
// static_assert(sizeof(Can1IOBoard407ServoData2) == 8);
static_assert(sizeof(Can1IOBoard407ADCInfo) == 8);

CanReceiveBuffer::CanReceiveBuffer(can::CanController::ptr can_ctrler,
                                   uint32_t can_device_index)
    : random_device_(), gen_(random_device_()),
      uniform_real_distribution_(-1.0, 1.0) {

    auto cb = std::bind_front(&CanReceiveBuffer::_listen_cb, this);

    can_ctrler->add_frame_received_listener(can_device_index, cb);
}

// void CanReceiveBuffer::load_servo_data(Can1IOBoard407ServoData &servo_data)
// const {
//     // auto temp = at_servo_data_.load();
//     // servo_data = *reinterpret_cast<Can1IOBoard407ServoData *>(&temp);
//     // *reinterpret_cast<dummy_8bytes *>(&servo_data) =
//     at_servo_data_.load(); servo_data = at_servo_data_.load();
// }

// void CanReceiveBuffer::load_adc_info(Can1IOBoard407ADCInfo &adc_info) const {
//     // auto temp = at_adc_info_.load();
//     // adc_info = *reinterpret_cast<Can1IOBoard407ADCInfo *>(&temp);
//     // *reinterpret_cast<dummy_8bytes *>(&adc_info) = at_adc_info_.load();
//     adc_info = at_adc_info_.load();
// }

void CanReceiveBuffer::_listen_cb(const QCanBusFrame &frame) {
    if (frame.frameId() == servodata_rxid_) [[likely]] {
        at_servo_data_is_new_ = true;

        at_servo_data_.store(
            *reinterpret_cast<dummy_8bytes *>(frame.payload().data()));

        // test
        Can1IOBoard407ServoData temp =
            *reinterpret_cast<Can1IOBoard407ServoData *>(
                frame.payload().data());
        EDM_CYCLIC_LOG(s_logger->debug, 500,
                       "touchdetected: {}, sdir: {}, sdis: {}",
                       (int)temp.touch_detected, (int)temp.servo_direction,
                       (int)temp.servo_distance_0_001um);

    } else if (frame.frameId() == adcinfo_rxid_) {
        at_adc_info_.store(
            *reinterpret_cast<dummy_8bytes *>(frame.payload().data()));

        // test
        Can1IOBoard407ADCInfo temp =
            *reinterpret_cast<Can1IOBoard407ADCInfo *>(frame.payload().data());
        EDM_CYCLIC_LOG(s_logger->debug, 500, "v: {}, i: {}",
                       (int)temp.new_voltage, (int)temp.new_current);
    }
}

void CanReceiveBuffer::load_servo_data(
    Can1IOBoard407ServoData &servo_data) const {
    *reinterpret_cast<dummy_8bytes *>(&servo_data) = at_servo_data_.load();
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

    servo_data.servo_direction = (int)(rd >= 0);
    uint16_t temp_0_001um =
        std::abs(rd * amplitude_um * 1000.0); // 转换到0.001um单位
    if (temp_0_001um > 10000)
        temp_0_001um = 10000;
    servo_data.servo_distance_0_001um = temp_0_001um;
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    servo_data.touch_detected = this->manual_touch_detect_flag_;
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

#ifdef EDM_OFFLINE_MANUAL_VOLTAGE
    servo_data.average_voltage = this->manual_voltage_value_;
#endif // EDM_OFFLINE_MANUAL_VOLTAGE
}

} // namespace edm
