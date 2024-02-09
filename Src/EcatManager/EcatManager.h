#pragma once

#include "ServoDevice.h"

#include "Utils/Filters/SlidingCounter/SlidingCounter.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace edm {

namespace ecat {

class EcatManager {
public:
    using ptr = std::shared_ptr<EcatManager>;
    EcatManager(std::size_t iomap_size, uint32_t servo_num,
                uint32_t io_num = 0);
    ~EcatManager();

    // do ethercat config and init
    // if all success, and slave numbers match
    // returns true, otherwise false
    bool connect_ecat(uint32_t max_try_times = 3);
    inline bool is_ecat_connected() const { return connected_; };

    // do send and receive progress data object
    // check wkc when receiving
    void ecat_sync();

    /* servo related interfaces */
    ServoDevice::ptr get_servo_device(uint32_t servo_index) const;
    void set_servo_target_position(uint32_t servo_index,
                                   int32_t target_position);
    int32_t get_servo_actual_position(uint32_t servo_index) const;

    bool servo_has_fault() const;
    bool servo_all_operation_enabled() const;

    void clear_fault_cycle_run_once();

private:
    std::size_t iomap_size_;
    char *iomap_;

    bool connected_ = false;

    uint32_t servo_num_;
    std::vector<ServoDevice::ptr> servo_devices_;
    // TODO if io ecat devices added

    uint32_t io_num_;

    // used for wkc filter
    utils::SlidingCounter<100> wkc_failed_sc;
    const double wkc_failed_threshold = 0.95;
};

} // namespace ecat

} // namespace edm
