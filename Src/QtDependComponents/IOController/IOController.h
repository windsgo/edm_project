#pragma once

#include "QtDependComponents/CanController/CanController.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include <QByteArray>
#include <qcanbusframe.h>

#include "QtDependComponents/PowerController/EleparamDefine.h"
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
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    void set_can_machineio_1(uint32_t can_io_1);
    void set_can_machineio_2(uint32_t can_io_2);
    void set_can_machineio(uint32_t can_io_1, uint32_t can_io_2);

    // 带演码设定io, 以达到只覆盖一部分io的目的 // TODO
    void set_can_machineio_1_withmask(uint32_t part_of_can_io_1, uint32_t mask);
    void set_can_machineio_2_withmask(uint32_t part_of_can_io_2, uint32_t mask);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    void set_can_machineio_output(uint32_t can_io_output);
    void set_can_machineio_output_withmask(uint32_t part_of_can_io_output, uint32_t mask);
#endif

    // trigger send once: send io to canbus once
    // this function will finally call QCoreApplication::post_event()
    // in order to send a can frame in another thread slightly later
    void trigger_send_current_io();

    // get io value which is currently set
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    uint32_t get_can_machineio_1() const { return can_machineio_1_; }
    uint32_t get_can_machineio_2() const { return can_machineio_2_; }

    // get with lock, safer
    uint32_t get_can_machineio_1_safe() const;
    uint32_t get_can_machineio_2_safe() const;
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    uint32_t get_can_machineio_output() const { return can_machineio_output_; }
    uint32_t get_can_machineio_input() const { return can_machineio_input_.load(); }

    // get with lock, safer
    uint32_t get_can_machineio_output_safe() const;
#endif

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    power::CanIOBoardDirectorState get_director_state() const;

    void director_start_pointmove(int32_t target_inc, uint16_t speed);
    void director_stop_pointmove();
    void director_start_homemove(uint16_t back_speed, uint16_t forward_speed);
#endif

private:
    // 无锁的设置函数, 仅仅是比对值和设置值
    // 如果需要trigger发送(io发生变动), 返回true, 否则返回false
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    bool _set_can_machineio_1_no_lock_no_trigger(uint32_t can_io_1);
    bool _set_can_machineio_2_no_lock_no_trigger(uint32_t can_io_2);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    bool _set_can_machineio_output_no_lock_no_trigger(uint32_t can_io_output);
#endif

private:
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // input io listener
    void _input_io_listener(const QCanBusFrame& frame);
#endif

private:
    // 触发发送, private方法, 无锁设计
    // 设计为输入要发送的io
    // 在设置io马上发送的场合, 可以节省锁的开销
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    void _trigger_send_io_1(uint32_t io_1);
    void _trigger_send_io_2(uint32_t io_2);

    void _calc_endcheck(QByteArray &bytearray);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    void _trigger_send_io_output(uint32_t io_output);
#endif

private:
    can::CanController::ptr can_ctrler_;

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    uint32_t can_machineio_1_{0x0};
    uint32_t can_machineio_2_{0x0};
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    uint32_t can_machineio_output_{0x0};
    std::atomic_uint32_t can_machineio_input_{0x0}; // can listener modify , safe atomic
#endif

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // 把小孔机的导向器功能先放在这里吧
    std::atomic<power::CanIOBoardDirectorState> at_local_director_state_;
#endif

    int can_device_index_ = -1; // used to send can frames by can::CanController

    mutable std::mutex mutex_can_io_; // used to protect both 2 io variable

private:
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    constexpr static const int CANIO1_TXID = EDM_CAN_TXID_CANIO_1;
    constexpr static const int CANIO2_TXID = EDM_CAN_TXID_CANIO_2;

    static const uint8_t canio1_raw_bytes_[8];
    static const uint8_t canio2_raw_bytes_[8];
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    constexpr static const int CANIO_OUTPUT_TXID = EDM_CAN_ZHONGGU_IO_OUTPUT_TXID;
    constexpr static const int CANIO_INPUT_RXID = EDM_CAN_ZHONGGU_IO_INPUT_RXID;

    static const uint8_t canio_output_raw_bytes_[8];
#endif

};

} // namespace io

} // namespace edm
