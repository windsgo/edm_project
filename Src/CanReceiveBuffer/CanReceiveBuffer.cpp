#include "CanReceiveBuffer.h"

namespace edm {

CanReceiveBuffer::CanReceiveBuffer(can::CanController::ptr can_ctrler,
                                   uint32_t can_device_index) {

    auto cb = std::bind_front(&CanReceiveBuffer::_listen_cb, this);

    can_ctrler->add_frame_received_listener(can_device_index, cb);
}

void CanReceiveBuffer::load_servo_data(Can1IOBoard407ServoData &servo_data) {
    // auto temp = at_servo_data_.load();
    // servo_data = *reinterpret_cast<Can1IOBoard407ServoData *>(&temp);
    *reinterpret_cast<dummy_8bytes*>(&servo_data) = at_servo_data_.load();
}

void CanReceiveBuffer::load_adc_info(Can1IOBoard407ADCInfo &adc_info) {
    // auto temp = at_adc_info_.load();
    // adc_info = *reinterpret_cast<Can1IOBoard407ADCInfo *>(&temp);
    *reinterpret_cast<dummy_8bytes*>(&adc_info) = at_adc_info_.load();
}

void CanReceiveBuffer::_listen_cb(const QCanBusFrame &frame) {
    if (frame.frameId() == servodata_rxid_) {
        at_servo_data_.store(
            *reinterpret_cast<dummy_8bytes *>(frame.payload().data()));
    } else if (frame.frameId() == adcinfo_rxid_) {
        at_adc_info_.store(
            *reinterpret_cast<dummy_8bytes *>(frame.payload().data()));
    }
}

} // namespace edm
