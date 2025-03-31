#include "IOController.h"

#include "Logger/LogMacro.h"
#include "QtDependComponents/CanController/CanController.h"

#include <QCanBusFrame>
#include <functional>
#include <qcanbusframe.h>

#include "QtDependComponents/PowerController/EleparamDefine.h"
#include "config.h"

namespace edm {

namespace io {

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
const uint8_t IOController::canio1_raw_bytes_[8] = {0xED, 0x00, 0xDE, 0x00,
                                                    0,    0,    0,    0};
const uint8_t IOController::canio2_raw_bytes_[8] = {0xDE, 0x00, 0xED, 0x00,
                                                    0,    0,    0,    0};
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
const uint8_t IOController::canio_output_raw_bytes_[8] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

// static auto s_can_ctrler = can::CanController::instance();
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

// IOController *IOController::instance() {
//     static IOController instance;
//     return &instance;
// }

IOController::IOController(can::CanController::ptr can_ctrler,
                           int can_device_index)
    : can_ctrler_(can_ctrler), can_device_index_(can_device_index) {
    // init bytearray
    // canio1_bytearray_[0] = 0xED;
    // canio1_bytearray_[1] = 0x00;
    // canio1_bytearray_[2] = 0xDE;

    // canio2_bytearray_[0] = 0xDE;
    // canio2_bytearray_[1] = 0x00;
    // canio2_bytearray_[2] = 0xED;
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // register input io listener
    can_ctrler_->add_frame_received_listener(
        can_device_index_,
        std::bind_front(&IOController::_input_io_listener, this));
#endif

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    can_ctrler_->add_frame_received_listener(
        can_device_index_, [this](const QCanBusFrame &frame) {
            if (frame.frameId() == EDM_CAN_RXID_IOBOARD_DIRECTOR_STATE) {
                auto ptr = reinterpret_cast<power::CanIOBoardDirectorState *>(
                    &(frame.payload().data()[0]));
                at_local_director_state_.store(*ptr);

                s_logger->trace("director state: {}, {}", (int)(ptr->curr_step),
                                (uint32_t)(ptr->director_state));
            }
        });
#endif
}

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
// input io listener
void IOController::_input_io_listener(const QCanBusFrame &frame) {
    if (frame.frameId() == CANIO_INPUT_RXID) {
        auto io_ptr =
            reinterpret_cast<uint32_t *>(&(frame.payload().data()[0]));
        can_machineio_input_.store(*io_ptr);

        s_logger->trace("io input: {0:#010X} {0:032B}", *io_ptr);
    }
}
#endif

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
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

    s_logger->trace("set io 1 with mask {1:#010X}: io1: {0:#010X} {0:032B}",
                    new_io_1, mask);

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

    s_logger->trace("set io 2 with mask {1:#010X}: io2: {0:#010X} {0:032B}",
                    new_io_2, mask);

    _trigger_send_io_2(new_io_2);
}
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
void IOController::set_can_machineio_output(uint32_t can_io_output) {
    {
        std::lock_guard guard(mutex_can_io_);

        if (!_set_can_machineio_output_no_lock_no_trigger(can_io_output)) {
            // 返回false, 说明io没发生变化, 不重复trigger
            return;
        }
    }

    s_logger->trace("set io output: {0:#010X} {0:032B}", can_io_output);

    _trigger_send_io_output(can_io_output);
}

void IOController::set_can_machineio_output_withmask(
    uint32_t part_of_can_io_output, uint32_t mask) {
    uint32_t new_io_output;
    {
        std::lock_guard guard(mutex_can_io_);

        new_io_output =
            (can_machineio_output_ & (~mask)) | (part_of_can_io_output & mask);

        if (!_set_can_machineio_output_no_lock_no_trigger(new_io_output)) {
            return;
        }
    }

    s_logger->trace(
        "set io output with mask {1:#010X}: io1: {0:#010X} {0:032B}",
        new_io_output, mask);

    _trigger_send_io_output(new_io_output);
}
#endif

void IOController::trigger_send_current_io() {
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    uint32_t io1;
    uint32_t io2;
    {
        std::lock_guard guard(mutex_can_io_);
        io1 = can_machineio_1_;
        io2 = can_machineio_2_;
    }

    _trigger_send_io_1(io1);
    _trigger_send_io_2(io2);

    // s_logger->trace(
    //     "IOController::trigger_send_current_io: 1: {:032B}, 2: {:032B}", io1,
    //     io2);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    uint32_t io_output;
    {
        std::lock_guard guard(mutex_can_io_);
        io_output = can_machineio_output_;
    }

    _trigger_send_io_output(io_output);
#endif
}

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
uint32_t IOController::get_can_machineio_1_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_1_;
}

uint32_t IOController::get_can_machineio_2_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_2_;
}
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
uint32_t IOController::get_can_machineio_output_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_output_;
}
#endif

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
power::CanIOBoardDirectorState IOController::get_director_state() const {
    return at_local_director_state_.load();
}

void IOController::director_start_pointmove(int32_t target_inc,
                                            uint16_t speed) {
    power::CanIOBoardDirectorStartPointMove d;
    d.target_inc = target_inc;
    d.speed = speed;

    QByteArray bytearray(
        reinterpret_cast<const char *>(canio_output_raw_bytes_),
        8); // deep copy initialize

    auto d_ptr =
        reinterpret_cast<struct power::CanIOBoardDirectorStartPointMove *>(
            &(bytearray.data()[0]));
    *d_ptr = d; // set

    QCanBusFrame frame(EDM_CAN_TXID_IOBOARD_DIRECTOR_STARTPOINTMOVE, bytearray);
    can_ctrler_->send_frame(can_device_index_, frame);
}

void IOController::director_stop_pointmove() {
    power::CanIOBoardDirectorStopPointMove d;

    QByteArray bytearray(
        reinterpret_cast<const char *>(canio_output_raw_bytes_),
        8); // deep copy initialize

    auto d_ptr =
        reinterpret_cast<struct power::CanIOBoardDirectorStopPointMove *>(
            &(bytearray.data()[0]));
    *d_ptr = d; // set

    QCanBusFrame frame(EDM_CAN_TXID_IOBOARD_DIRECTOR_STOPPOINTMOVE, bytearray);
    can_ctrler_->send_frame(can_device_index_, frame);
}

void IOController::director_start_homemove(uint16_t back_speed,
                                           uint16_t forward_speed) {
    power::CanIOBoardDirectorStartHomeMove d;
    d.back_speed = back_speed;
    d.forward_speed = forward_speed;

    QByteArray bytearray(
        reinterpret_cast<const char *>(canio_output_raw_bytes_),
        8); // deep copy initialize

    auto d_ptr =
        reinterpret_cast<struct power::CanIOBoardDirectorStartHomeMove *>(
            &(bytearray.data()[0]));
    *d_ptr = d; // set

    QCanBusFrame frame(EDM_CAN_TXID_IOBOARD_DIRECTOR_STARTHOMEMOVE, bytearray);
    can_ctrler_->send_frame(can_device_index_, frame);
}
#endif

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
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
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
bool IOController::_set_can_machineio_output_no_lock_no_trigger(
    uint32_t can_io_output) {
    if (can_io_output == can_machineio_output_) {
        return false;
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    {
        static const uint32_t opump_bit =
            (1 << (power::ZHONGGU_IOOut_IOOUT1_OPUMP - 1));
        bool prev_opump_status = !!(can_machineio_output_ & opump_bit);
        bool new_opump_status = !!(can_io_output & opump_bit);
        if (prev_opump_status != new_opump_status) {
            s_logger->debug("prev_opump_status: {}, new_opump_status: {}",
                            prev_opump_status, new_opump_status);

            struct power::CanHandboxIOStatus d;
            d.pump_on = new_opump_status;
            // construct bytearray
            QByteArray bytearray(
                reinterpret_cast<const char *>(canio_output_raw_bytes_),
                8); // deep copy initialize

            auto io_ptr = reinterpret_cast<struct power::CanHandboxIOStatus *>(
                &(bytearray.data()[0]));
            *io_ptr = d; // set io

            QCanBusFrame frame(EDM_CAN_TXID_HANDBOX_IOSTATUS, bytearray);
            can_ctrler_->send_frame(can_device_index_, frame);
        }
    }
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU)
    {
        static const uint32_t opump_bit =
            (1 << (power::ZHONGGU_IOOut_IOOUT1_FULD - 1));
        bool prev_opump_status = !!(can_machineio_output_ & opump_bit);
        bool new_opump_status = !!(can_io_output & opump_bit);
        if (prev_opump_status != new_opump_status) {
            s_logger->debug("prev_opump_status: {}, new_opump_status: {}",
                            prev_opump_status, new_opump_status);

            struct power::CanHandboxIOStatus d;
            d.pump_on = new_opump_status;
            // construct bytearray
            QByteArray bytearray(
                reinterpret_cast<const char *>(canio_output_raw_bytes_),
                8); // deep copy initialize

            auto io_ptr = reinterpret_cast<struct power::CanHandboxIOStatus *>(
                &(bytearray.data()[0]));
            *io_ptr = d; // set io

            QCanBusFrame frame(EDM_CAN_TXID_HANDBOX_IOSTATUS, bytearray);
            can_ctrler_->send_frame(can_device_index_, frame);
        }
    }
#endif

    can_machineio_output_ = can_io_output;
    return true;
}
#endif

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
void IOController::_trigger_send_io_1(uint32_t io_1) {
    // construct bytearray
    QByteArray bytearray(reinterpret_cast<const char *>(canio1_raw_bytes_),
                         8); // deep copy initialize

    auto io_ptr = reinterpret_cast<uint32_t *>(&(bytearray.data()[3]));
    *io_ptr = io_1; // set io

    _calc_endcheck(bytearray);

    QCanBusFrame frame(CANIO1_TXID, bytearray);
    can_ctrler_->send_frame(can_device_index_, frame);
}

void IOController::_trigger_send_io_2(uint32_t io_2) {
    // construct bytearray
    QByteArray bytearray(reinterpret_cast<const char *>(canio2_raw_bytes_),
                         8); // deep copy initialize

    auto io_ptr = reinterpret_cast<uint32_t *>(&(bytearray.data()[3]));
    *io_ptr = io_2; // set io

    _calc_endcheck(bytearray);

    QCanBusFrame frame(CANIO2_TXID, bytearray);
    can_ctrler_->send_frame(can_device_index_, frame);
}

void IOController::_calc_endcheck(QByteArray &bytearray) {
    assert(bytearray.size() == 8);

    uint16_t end_check = 0x00;
    for (int i = 0; i < 7; ++i) {
        end_check += bytearray[i];
    }

    bytearray[7] = static_cast<uint8_t>(end_check);
}
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || \
    (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
void IOController::_trigger_send_io_output(uint32_t io_output) {
    // construct bytearray
    QByteArray bytearray(
        reinterpret_cast<const char *>(canio_output_raw_bytes_),
        8); // deep copy initialize

    auto io_ptr = reinterpret_cast<uint32_t *>(&(bytearray.data()[0]));
    *io_ptr = io_output; // set io
    io_ptr[1] = 0x12345678; // check crc

    QCanBusFrame frame(CANIO_OUTPUT_TXID, bytearray);
    can_ctrler_->send_frame(can_device_index_, frame);
}
#endif

} // namespace io

} // namespace edm