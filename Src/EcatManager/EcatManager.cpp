#include "EcatManager.h"

#include "config.h"

namespace edm {

namespace ecat {

EcatManager::EcatManager(std::size_t iomap_size, uint32_t servo_num,
                         uint32_t io_num)
    : iomap_size_{iomap_size}, servo_num_{servo_num}, io_num_{io_num} {
    iomap_ = new char[iomap_size_];
    servo_devices_.reserve(servo_num_);
}

EcatManager::~EcatManager() { delete[] iomap_; }

bool EcatManager::connect_ecat(uint32_t max_try_times) { 
    // TODO ec things
    return false; 
}

void EcatManager::ecat_sync() {
    // TODO ec things ...
#ifdef EDM_ECAT_DRIVER_SOEM
    // send ... 
    // recv ...
#endif // EDM_ECAT_DRIVER_SOEM

    // wkc ...
    const uint32_t expected_wkc = (servo_num_ + io_num_) * 3;
    // if wkc < expected wkc
    wkc_failed_sc.push_back_valid(); 
    // or 
    wkc_failed_sc.push_back_invalid();

    if (wkc_failed_sc.valid_rate() > wkc_failed_threshold) {
        connected_ = false;
        // ec_close() ... 
    }
}

ServoDevice::ptr
edm::ecat::EcatManager::get_servo_device(uint32_t servo_index) const {
    return servo_devices_[servo_index];
    // if (servo_index < servo_index) [[likely]] {
    //     return servo_devices_[servo_index];
    // } else {
    //     return nullptr;
    // }
}

} // namespace ecat

} // namespace edm