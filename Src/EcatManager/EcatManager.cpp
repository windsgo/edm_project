#include "EcatManager.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "config.h"

#include <string.h>

#ifdef EDM_ECAT_DRIVER_SOEM
#include "ethercat.h"
#endif // EDM_ECAT_DRIVER_SOEM

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace ecat {

EcatManager::EcatManager(std::string_view ifname, std::size_t iomap_size,
                         uint32_t servo_num, uint32_t io_num)
    : ifname_(ifname), iomap_size_{iomap_size}, servo_num_{servo_num},
      io_num_{io_num} {
    iomap_ = new char[iomap_size_];
    if (!iomap_) {
        throw exception("iomap allocated failed");
    }
    memset(iomap_, 0, sizeof(iomap_));

    servo_devices_.resize(servo_num_);
}

EcatManager::~EcatManager() { delete[] iomap_; }

bool EcatManager::_wait_slaves_to_safe_op() {
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 4);

    if (ec_slave[0].state != EC_STATE_SAFE_OP) {
        s_logger->error("Not all slaves reached safe operational state.");
        ec_readstate();
        for (int i = 1; i <= ec_slavecount; i++) {
            if (ec_slave[i].state != EC_STATE_SAFE_OP) {
                s_logger->warn(
                    "Slave {} State={:02x} StatusCode={:04x} : {}", i,
                    ec_slave[i].state, ec_slave[i].ALstatuscode,
                    ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
            }
        }
        return false;
    } else {
        return true;
    }
}

bool EcatManager::_wait_slaves_to_operational() {
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    /* send one valid process data to make outputs in slaves happy*/
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    /* request OP state for all slaves */
    ec_writestate(0);
    int chk = 200;
    /* wait for all slaves to reach OP state */
    do {
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        ec_statecheck(0, EC_STATE_OPERATIONAL, 50000);
    } while (chk-- && (ec_slave[0].state != EC_STATE_OPERATIONAL));

    if (ec_slave[0].state == EC_STATE_OPERATIONAL) {
        s_logger->info("Operational state reached for all slaves.");
        return true;
    } else {
        s_logger->error("Not all slaves reached operational state.");
        ec_readstate();
        for (int i = 1; i <= ec_slavecount; i++) {
            if (ec_slave[i].state != EC_STATE_OPERATIONAL) {
                s_logger->warn(
                    "Slave {} State={:02x} StatusCode={:04x} : {}", i,
                    ec_slave[i].state, ec_slave[i].ALstatuscode,
                    ec_ALstatuscode2string(ec_slave[i].ALstatuscode));
            }
        }
        return false;
    }
}

bool EcatManager::_connect_ecat_try_once(int expected_slavecount) {
#ifdef EDM_ECAT_DRIVER_SOEM
    // init netif
    if (!ec_init(ifname_.c_str())) {
        s_logger->error("ec_init on {} failed", ifname_);
        return false;
    }

    // configure slaves and pdo maps
    if (ec_config(FALSE, static_cast<void *>(iomap_)) <= 0) {
        s_logger->error("no slaves found on {}", ifname_);
        return false;
    }

    // configure dc (?) // TODO
    bool dc_ret = ec_configdc();
    if (!dc_ret) {
        s_logger->warn("ec_configdc failed.");
    }

    // handle and print ethercat errors
    while (EcatError) {
        printf("%s", ec_elist2string());
        // std::string error_str = ec_elist2string();
        // if (error_str.ends_with("\n")) {
        //     error_str.pop_back();
        // }
        // s_logger->trace("EcatError: {}", error_str);
    }

    s_logger->info("ec_config success, {} slaves found and configured",
                   ec_slavecount);

    if (ec_slavecount != expected_slavecount) {
        s_logger->error("ec_slavecount({}) != expected_slavecount({})",
                        ec_slavecount, expected_slavecount);
        return false;
    }

    int expected_wkc = (ec_group[0].outputsWKC * 2) + ec_group[0].inputsWKC;
    s_logger->info("calculated workcounter {}", expected_wkc);
    if (expected_wkc != ec_slavecount * 3) {
        s_logger->error("expected_wkc{} != ec_slavecount{} * 3", expected_wkc,
                        ec_slavecount);
        ec_close();
        return false;
    }

    /* wait for all slaves to reach SAFE_OP state */
    if (!_wait_slaves_to_safe_op()) {
        s_logger->error("slaves to safe op failed");
        ec_close();
        return false;
    }

    /* wait for all slaves to reach OPERATIONAL state */
    if (!_wait_slaves_to_operational()) {
        s_logger->error("slaves to operational failed");
        ec_close();
        return false;
    }

    _conf_servo_pdo_map();
    _conf_servo_default_op();

    // 进行一次发送和接受
    ec_send_processdata();
    int wkc = ec_receive_processdata(EC_TIMEOUTRET);
    if (wkc < expected_wkc) {
        s_logger->error("wkc not matched");
        ec_close();
        return false;
    }

    return true;

#endif // EDM_ECAT_DRIVER_SOEM

    assert(false); // should not be here
}

void EcatManager::_conf_servo_pdo_map() {
#ifdef EDM_ECAT_DRIVER_SOEM
    for (int i = 0; i < servo_num_; ++i) {
        s_logger->trace("_conf_servo_pdo_map: {}", i);
        servo_devices_[i] = std::make_shared<PanasonicServoDevice>(
            (Panasonic_A5B_Ctrl *)(ec_slave[i + 1].outputs),
            (Panasonic_A5B_Stat *)(ec_slave[i + 1].inputs));
    }
#endif // EDM_ECAT_DRIVER_SOEM
}

void EcatManager::_conf_servo_default_op() {
#ifdef EDM_ECAT_DRIVER_SOEM
    for (int i = 0; i < servo_num_; ++i) {
        servo_devices_[i]->set_operation_mode(OM_CSP);
    }
#endif // EDM_ECAT_DRIVER_SOEM
}

bool EcatManager::connect_ecat(uint32_t max_try_times) {
    // TODO ec things
    s_logger->debug("EcatManager connect_ecat start: {}, netif: {}",
                    max_try_times, ifname_);

    bool ret = false;

    for (int try_times = 1; try_times <= max_try_times; ++try_times) {
        s_logger->info("trying connect ecat, try times: {}", try_times);
        ret = _connect_ecat_try_once(servo_num_ + io_num_);
        if (!ret) {
            continue;
        }

        s_logger->info("connect ecat success.");
        break;
    }

    if (!ret) {
        s_logger->critical("connect ecat failed.");
        return false;
    }

    return true;
}

void EcatManager::disconnect_ecat() {
    ec_slave[0].state = EC_STATE_INIT;
    /* request INIT state for all slaves */
    ec_writestate(0);
    ec_close();
    s_logger->info("ecat disconnected.");

    connected_ = false;
}

void EcatManager::ecat_sync() {
    int wkc;
#ifdef EDM_ECAT_DRIVER_SOEM
    ec_send_processdata();
    wkc = ec_receive_processdata(200); // at most 200 us
#endif                                 // EDM_ECAT_DRIVER_SOEM

    const uint32_t expected_wkc = (servo_num_ + io_num_) * 3;
    if (wkc < expected_wkc) {
        wkc_failed_sc.push_back_valid();
    } else {
        wkc_failed_sc.push_back_invalid();
    }

    if (wkc_failed_sc.valid_rate() > wkc_failed_threshold) {
        connected_ = false;
#ifdef EDM_ECAT_DRIVER_SOEM
        ec_close();
#endif // EDM_ECAT_DRIVER_SOEM
    }
}

ServoDevice::ptr
edm::ecat::EcatManager::get_servo_device(uint32_t servo_index) const {
    // TODO safe index
    return servo_devices_[servo_index];
    // if (servo_index < servo_index) [[likely]] {
    //     return servo_devices_[servo_index];
    // } else {
    //     return nullptr;
    // }
}

void EcatManager::set_servo_target_position(uint32_t servo_index,
                                            int32_t target_position) {
    // TODO safe index
    servo_devices_[servo_index]->set_target_position(target_position);
}

int32_t EcatManager::get_servo_actual_position(uint32_t servo_index) const {
    // TODO safe index
    return servo_devices_[servo_index]->get_actual_position();
}

bool EcatManager::servo_has_fault() const {
    for (const auto &servo : servo_devices_) {
        if (servo->sw_fault()) {
            return true;
        }
    }

    return false;
}

bool EcatManager::servo_all_operation_enabled() const {
    for (const auto &servo : servo_devices_) {
        if (!servo->sw_operational_enabled()) {
            return false;
        }
    }

    return true;
}

bool EcatManager::servo_all_disabled() const {
    for (const auto &servo : servo_devices_) {
        if (!servo->sw_switch_on_disabled()) {
            return false;
        }
    }

    return true;
}

void EcatManager::clear_fault_cycle_run_once() {
    for (auto &servo : servo_devices_) {
        if (servo->sw_fault()) {
            servo->cw_fault_reset();
        } else if (servo->sw_switch_on_disabled()) {
            servo->cw_shut_down();
        } else if (servo->sw_ready_to_switch_on()) {
            servo->cw_switch_on();
        } else if (servo->sw_switched_on() || servo->sw_operational_enabled()) {
            servo->cw_enable_operation();
            servo->sync_actual_position_to_target_position(); // to ensure the
                                                              // servo would not
                                                              // move suddenly
                                                              // or die again
        }
    }
}

void EcatManager::disable_cycle_run_once() {
    //! disabled 状态定义为 switch_on_disabled 状态
    for (auto &servo : servo_devices_) {
        s_logger->debug("sw: {:016b}", servo->get_status_word());
        if (servo->sw_fault()) {
            servo->cw_fault_reset();
        } else if (servo->sw_switch_on_disabled()) {
            // servo->cw_shut_down();
            // Do nothing
        } else if (servo->sw_ready_to_switch_on()) {
            servo->cw_disable_voltage();
        } else if (servo->sw_switched_on()) {
            servo->cw_shut_down();
        } else if (servo->sw_operational_enabled()) {
            servo->cw_disable_operation();
        }
    }
}

} // namespace ecat

} // namespace edm