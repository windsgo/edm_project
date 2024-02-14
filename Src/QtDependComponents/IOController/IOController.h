#pragma once

#include <cstdint>
#include <mutex>

namespace edm
{

namespace io
{
    
class IOController {
public:
    static IOController* instance();

    // init device
    void init(int can_device_index);

    // io settings
    // if one of the io is changed
    // canframe send will be triggered
    void set_can_machineio_1(uint32_t can_io_1);
    void set_can_machineio_2(uint32_t can_io_2);
    void set_can_machineio(uint32_t can_io_1, uint32_t can_io_2);

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

    IOController() = default;
    ~IOController() = default;

private:
    uint32_t can_machineio_1_ {0x0};
    uint32_t can_machineio_2_ {0x0};

    bool inited_ = false; // if not inited, no io can frame will be sent

    int can_device_index_ = -1; // used to send can frames by can::CanController

    mutable std::mutex mutex_can_io_; // used to protect both 2 io variable
};

} // namespace io

} // namespace edm
