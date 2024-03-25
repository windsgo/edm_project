#include "CanReceiveBuffer.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

static_assert(sizeof(Can1IOBoard407ServoData) == 8);
static_assert(sizeof(Can1IOBoard407ServoData2) == 8);
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
    if (frame.frameId() == servodata_rxid_) {
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
                       (int)temp.servo_distance_0_01um);

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

} // namespace edm
