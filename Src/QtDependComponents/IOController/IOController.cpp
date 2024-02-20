#include "IOController.h"

#include "Logger/LogMacro.h"
#include "QtDependComponents/CanController/CanController.h"

#include <QCanBusFrame>

namespace edm {

namespace io {

const uint8_t IOController::canio1_raw_bytes_[8] = {0xED, 0x00, 0xDE, 0x00,
                                                    0,    0,    0,    0};
const uint8_t IOController::canio2_raw_bytes_[8] = {0xDE, 0x00, 0xED, 0x00,
                                                    0,    0,    0,    0};

static auto s_can_ctrler = can::CanController::instance();
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

IOController *IOController::instance() {
    static IOController instance;
    return &instance;
}

void IOController::init(int can_device_index) {
    inited_ = true;
    can_device_index_ = can_device_index;
}

void IOController::set_can_machineio_1(uint32_t can_io_1) {
    {
        std::lock_guard guard(mutex_can_io_);

        if (!_set_can_machineio_1_no_lock_no_trigger(can_io_1)) {
            // 返回false, 说明io没发生变化, 不重复trigger
            return;
        }
    }

    s_logger->trace("set io 1: {0:#010X} {0:032B}", can_io_1);

    _trigger_send_io_1(can_io_1);
}

void IOController::set_can_machineio_2(uint32_t can_io_2) {
    {
        std::lock_guard guard(mutex_can_io_);

        if (!_set_can_machineio_2_no_lock_no_trigger(can_io_2)) {
            // 返回false, 说明io没发生变化, 不重复trigger
            return;
        }
    }

    s_logger->trace("set io 2: {0:#010X} {0:032B}", can_io_2);

    _trigger_send_io_2(can_io_2);
}

void IOController::set_can_machineio(uint32_t can_io_1, uint32_t can_io_2) {
    set_can_machineio_1(can_io_1);
    set_can_machineio_2(can_io_2);
}

void IOController::set_can_machineio_1_withmask(uint32_t part_of_can_io_1,
                                                uint32_t mask) {
    uint32_t new_io_1;
    {
        std::lock_guard guard(mutex_can_io_);

        new_io_1 = (can_machineio_1_ & (~mask)) | (part_of_can_io_1 & mask);

        if (!_set_can_machineio_1_no_lock_no_trigger(new_io_1)) {
            return;
        }
    }

    s_logger->trace("set io 1 with mask {1:#010X}: {0:#010X} {0:032B}", new_io_1, mask);

    _trigger_send_io_1(new_io_1);
}

void IOController::set_can_machineio_2_withmask(uint32_t part_of_can_io_2,
                                                uint32_t mask) {
    uint32_t new_io_2;
    {
        std::lock_guard guard(mutex_can_io_);

        new_io_2 = (can_machineio_2_ & (~mask)) | (part_of_can_io_2 & mask);

        if (!_set_can_machineio_2_no_lock_no_trigger(new_io_2)) {
            return;
        }
    }

    s_logger->trace("set io 2 with mask {1:#010X}: {0:#010X} {0:032B}", new_io_2, mask);

    _trigger_send_io_2(new_io_2);
}

void IOController::trigger_send_current_io() {
    if (!inited_)
        return;

    uint32_t io1;
    uint32_t io2;
    {
        std::lock_guard guard(mutex_can_io_);
        io1 = can_machineio_1_;
        io2 = can_machineio_2_;
    }

    _trigger_send_io_1(io1);
    _trigger_send_io_2(io2);
}

uint32_t IOController::get_can_machineio_1_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_1_;
}

uint32_t IOController::get_can_machineio_2_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_2_;
}

bool IOController::_set_can_machineio_1_no_lock_no_trigger(uint32_t can_io_1) {
    if (can_io_1 == can_machineio_1_) {
        return false;
    }

    can_machineio_1_ = can_io_1;
    return true;
}

bool IOController::_set_can_machineio_2_no_lock_no_trigger(uint32_t can_io_2) {
    if (can_io_2 == can_machineio_2_) {
        return false;
    }

    can_machineio_2_ = can_io_2;
    return true;
}

IOController::IOController() {
    // init bytearray
    // canio1_bytearray_[0] = 0xED;
    // canio1_bytearray_[1] = 0x00;
    // canio1_bytearray_[2] = 0xDE;

    // canio2_bytearray_[0] = 0xDE;
    // canio2_bytearray_[1] = 0x00;
    // canio2_bytearray_[2] = 0xED;
}

void IOController::_trigger_send_io_1(uint32_t io_1) {
    // construct bytearray
    QByteArray bytearray(reinterpret_cast<const char *>(canio1_raw_bytes_),
                         8); // deep copy initialize

    auto io_ptr = reinterpret_cast<uint32_t *>(&(bytearray.data()[3]));
    *io_ptr = io_1; // set io

    _calc_endcheck(bytearray);

    QCanBusFrame frame(CANIO1_TXID, bytearray);
    s_can_ctrler->send_frame(can_device_index_, frame);
}

void IOController::_trigger_send_io_2(uint32_t io_2) {
    // construct bytearray
    QByteArray bytearray(reinterpret_cast<const char *>(canio2_raw_bytes_),
                         8); // deep copy initialize

    auto io_ptr = reinterpret_cast<uint32_t *>(&(bytearray.data()[3]));
    *io_ptr = io_2; // set io

    _calc_endcheck(bytearray);

    QCanBusFrame frame(CANIO2_TXID, bytearray);
    s_can_ctrler->send_frame(can_device_index_, frame);
}

void IOController::_calc_endcheck(QByteArray &bytearray) {
    assert(bytearray.size() == 8);

    uint16_t end_check = 0x00;
    for (int i = 0; i < 7; ++i) {
        end_check += bytearray[i];
    }

    bytearray[7] = static_cast<uint8_t>(end_check);
}

} // namespace io

} // namespace edm