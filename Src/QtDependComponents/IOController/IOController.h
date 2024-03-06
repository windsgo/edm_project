#pragma once

#include "QtDependComponents/CanController/CanController.h"
#include <cstdint>
#include <memory>
#include <mutex>

#include <QByteArray>

#include "config.h"

namespace edm {

namespace io {

class IOController final {
public:
    using ptr = std::shared_ptr<IOController>;
    // static IOController* instance();

    IOController(can::CanController::ptr can_ctrler, int can_device_index);
    ~IOController() noexcept = default;

    // io settings
    // if one of the io is changed
    // canframe send will be triggered
    void set_can_machineio_1(uint32_t can_io_1);
    void set_can_machineio_2(uint32_t can_io_2);
    void set_can_machineio(uint32_t can_io_1, uint32_t can_io_2);

    // 带演码设定io, 以达到只覆盖一部分io的目的 // TODO
    void set_can_machineio_1_withmask(uint32_t part_of_can_io_1, uint32_t mask);
    void set_can_machineio_2_withmask(uint32_t part_of_can_io_2, uint32_t mask);

    // trigger send once: send io to canbus once
    // this function will finally call QCoreApplication::post_event()
    // in order to send a can frame in another thread slightly later
    void trigger_send_current_io();

    // get io value which is currently set
    uint32_t get_can_machineio_1() const { return can_machineio_1_; }
    uint32_t get_can_machineio_2() const { return can_machineio_2_; }

    // get with lock, safer
    uint32_t get_can_machineio_1_safe() const;
    uint32_t get_can_machineio_2_safe() const;

private:
    // 无锁的设置函数, 仅仅是比对值和设置值
    // 如果需要trigger发送(io发生变动), 返回true, 否则返回false
    bool _set_can_machineio_1_no_lock_no_trigger(uint32_t can_io_1);
    bool _set_can_machineio_2_no_lock_no_trigger(uint32_t can_io_2);

private:
private:
    // 触发发送, private方法, 无锁设计
    // 设计为输入要发送的io
    // 在设置io马上发送的场合, 可以节省锁的开销
    void _trigger_send_io_1(uint32_t io_1);
    void _trigger_send_io_2(uint32_t io_2);

    void _calc_endcheck(QByteArray &bytearray);

private:
    can::CanController::ptr can_ctrler_;

    uint32_t can_machineio_1_{0x0};
    uint32_t can_machineio_2_{0x0};

    int can_device_index_ = -1; // used to send can frames by can::CanController

    mutable std::mutex mutex_can_io_; // used to protect both 2 io variable

private:
    constexpr static const int CANIO1_TXID = EDM_CAN_TXID_CANIO_1;
    constexpr static const int CANIO2_TXID = EDM_CAN_TXID_CANIO_2;

    // QByteArray canio1_bytearray_ {8, 0x00};
    // QByteArray canio2_bytearray_ {8, 0x00};

    static const uint8_t canio1_raw_bytes_[8];
    static const uint8_t canio2_raw_bytes_[8];

};

} // namespace io

} // namespace edm
