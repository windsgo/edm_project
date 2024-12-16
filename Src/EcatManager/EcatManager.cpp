#include "EcatManager.h"

#include "EcatManager/Panasonic.h"
#include "EcatManager/ServoDefines.h"
#include "Exception/exception.h"
#include "Logger/LogMacro.h"

#include "ServoDevice.h"

#include <ctime>
#include <ecrt.h>
#include <memory>
#include <string.h>

#include "SystemSettings/SystemSettings.h"

#ifdef EDM_ECAT_DRIVER_SOEM
#include "ethercat.h"
#include "ethercatmain.h"
#endif // EDM_ECAT_DRIVER_SOEM

#define NSEC_PER_SEC   (1000000000L)
#define TIMESPEC2NS(T) ((uint64_t)(T).tv_sec * NSEC_PER_SEC + (T).tv_nsec)

EDM_STATIC_LOGGER_NAME(s_logger, "motion");
// EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace ecat {

#ifdef EDM_ECAT_DRIVER_IGH
ec_pdo_entry_info_t slave_0_pdo_entries[] = {
    {0x6040, 0x00, 16}, /* Controlword */
    {0x6060, 0x00, 8},  /* Modes of operation */
    {0x607a, 0x00, 32}, /* Target position */
    {0x60b8, 0x00, 16}, /* Touch probe function */
    // {0x60b1, 0x00, 32}, /* Velocity offset */

    {0x603f, 0x00, 16}, /* Error code */
    {0x6041, 0x00, 16}, /* Statusword */
    {0x6061, 0x00, 8},  /* Modes of operation display */
    {0x6064, 0x00, 32}, /* Position actual value */
    {0x60b9, 0x00, 16}, /* Touch probe status */
    {0x60ba, 0x00, 32}, /* Touch probe pos1 pos value */
    {0x60f4, 0x00, 32}, /* Following error actual value */
    {0x60fd, 0x00, 32}, /* Digital inputs */
};

ec_pdo_info_t slave_0_pdos[] = {
    // {0x1600, 5, slave_0_pdo_entries + 0}, /* Receive PDO mapping 1 */
    // {0x1a00, 8, slave_0_pdo_entries + 5}, /* Transmit PDO mapping 1 */
    {0x1600, 4, slave_0_pdo_entries + 0}, /* Receive PDO mapping 1 */
    {0x1a00, 8, slave_0_pdo_entries + 4}, /* Transmit PDO mapping 1 */
};

ec_sync_info_t slave_0_syncs[] = {
    {0, EC_DIR_OUTPUT, 0, NULL, EC_WD_DISABLE},
    {1, EC_DIR_INPUT, 0, NULL, EC_WD_DISABLE},
    {2, EC_DIR_OUTPUT, 1, slave_0_pdos + 0, EC_WD_ENABLE},
    {3, EC_DIR_INPUT, 1, slave_0_pdos + 1, EC_WD_DISABLE},
    {0xff}};

// 不同的松下驱动器也有不同的product_code
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU)
static std::vector<std::pair<uint32_t, uint32_t>>
    s_slave_vendor_and_product_code_vec = {
        {0x0000066f, 0x613C0007}, // X
        {0x0000066f, 0x613C0007}, // Y
        {0x0000066f, 0x60380007}, // Z
        {0x0000066f, 0x60380006}, // B
        {0x0000066f, 0x60380006}, // C
        {0x0000066f, 0x60380004}, // A
};
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
static std::vector<std::pair<uint32_t, uint32_t>> //! TODO 
    s_slave_vendor_and_product_code_vec = {
        {0x0000066f, 0x60380007}, // X
        {0x0000066f, 0x60380007}, // Y
        {0x0000066f, 0x613C0007}, // Z
        {0x0000066f, 0x613C0007}, // B
        {0x0000066f, 0x60380006}, // C
        {0x0000066f, 0x60380004}, // S
        {0x0000066f, 0x60380005}, // 主轴旋转 
};
#endif // EDM_POWER_TYPE
#endif // EDM_ECAT_DRIVER_IGH

EcatManager::EcatManager(std::string_view ifname, std::size_t iomap_size,
                         uint32_t servo_num, uint32_t io_num)
    : ifname_(ifname), iomap_size_{iomap_size}, servo_num_{servo_num},
      io_num_{io_num} {
#ifdef EDM_ECAT_DRIVER_SOEM
    iomap_ = new char[iomap_size_];
    if (!iomap_) {
        throw exception("iomap allocated failed");
    }
    memset(iomap_, 0, iomap_size_);
#endif // EDM_ECAT_DRIVER_SOEM

    s_logger->trace("servo num: {}", servo_num_);

#ifdef EDM_ECAT_DRIVER_IGH
#ifndef EDM_OFFLINE_RUN_NO_ECAT
    // request master (init); index default 0; configured in
    // /etc/sysconfig/ethercat
    igh_master_ = ecrt_request_master(0);
    if (!igh_master_) {
        s_logger->critical("igh ecrt_request_master failed");
        throw exception("igh ecrt_request_master failed");
    }
#endif // EDM_OFFLINE_RUN_NO_ECAT

    // resize member vectors according to servonum and ionum (unused now)
    // and assign each pointer to nullptr
    igh_domain_vec_.resize(servo_num_ + io_num_);
    for (auto &d : igh_domain_vec_) {
        d = nullptr;
    }

    igh_domain_pd_vec_.resize(servo_num_ + io_num_);
    for (auto &d_pd : igh_domain_pd_vec_) {
        d_pd = nullptr;
    }

    igh_sc_vec_.resize(servo_num_ + io_num_);

    igh_domain_regs_vec_.resize(servo_num_ + io_num_);
    igh_domain_pdo_input_offsets_vec_.resize(servo_num_ + io_num_);
    igh_domain_pdo_output_offsets_vec_.resize(servo_num_ + io_num_);
#endif // EDM_ECAT_DRIVER_IGH

    servo_devices_.resize(servo_num_);
}

EcatManager::~EcatManager() {
#ifdef EDM_ECAT_DRIVER_SOEM
    delete[] iomap_;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    if (igh_master_) {
        s_logger->trace("in ~EcatManager, release igh_master_");
        ecrt_release_master(igh_master_);
    }
#endif // EDM_ECAT_DRIVER_IGH
}

#ifdef EDM_ECAT_DRIVER_SOEM
bool EcatManager::_soem_wait_slaves_to_safe_op() {
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

bool EcatManager::_soem_wait_slaves_to_operational() {
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
#endif // EDM_ECAT_DRIVER_SOEM

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
    if (!_soem_wait_slaves_to_safe_op()) {
        s_logger->error("slaves to safe op failed");
        ec_close();
        return false;
    }

    /* wait for all slaves to reach OPERATIONAL state */
    if (!_soem_wait_slaves_to_operational()) {
        s_logger->error("slaves to operational failed");
        ec_close();
        return false;
    }

    _soem_conf_servo_pdo_map();
    _soem_conf_servo_default_op();

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

#ifdef EDM_ECAT_DRIVER_IGH

    for (uint32_t i = 0; i < igh_domain_vec_.size(); ++i) {
        igh_domain_vec_[i] = ecrt_master_create_domain(igh_master_);
        if (!igh_domain_vec_[i]) {
            s_logger->error("ecrt_master_create_domain failed: i = {}", i);
            return false;
        }
    }

    // configure Panasonic slaves
    auto motion_cycle_us = SystemSettings::instance().get_motion_cycle_us();
    auto ecat_sync0_shift_time_ns =
        SystemSettings::instance().get_ecat_sync0_shift_time_ns();
    for (uint16_t i = 0; i < servo_num_; ++i) {
        igh_sc_vec_[i] = ecrt_master_slave_config(
            igh_master_, 0, i, s_slave_vendor_and_product_code_vec[i].first,
            s_slave_vendor_and_product_code_vec[i].second);

        if (!igh_sc_vec_[i]) {
            s_logger->error("ecrt_master_slave_config error: servo {}", i);
            return false;
        }

        if (ecrt_slave_config_pdos(igh_sc_vec_[i], EC_END, slave_0_syncs)) {
            s_logger->error("ecrt_slave_config_pdos error: servo {}", i);
            return false;
        }

        // for each servo domain
        igh_domain_regs_vec_[i].clear();
        ec_pdo_entry_reg_t temp;
        temp.alias = 0;
        temp.position = i;
        temp.vendor_id = s_slave_vendor_and_product_code_vec[i].first;
        temp.product_code = s_slave_vendor_and_product_code_vec[i].second;
        temp.bit_position = NULL;

        // output
        // control word
        temp.index = 0x6040;
        temp.subindex = 0x00;
        temp.offset = &igh_domain_pdo_output_offsets_vec_[i].off_control_word;
        igh_domain_regs_vec_[i].push_back(temp);

        // op
        temp.index = 0x6060;
        temp.subindex = 0x00;
        temp.offset =
            &igh_domain_pdo_output_offsets_vec_[i].off_modes_of_operation;
        igh_domain_regs_vec_[i].push_back(temp);

        // target pos
        temp.index = 0x607a;
        temp.subindex = 0x00;
        temp.offset =
            &igh_domain_pdo_output_offsets_vec_[i].off_target_position;
        igh_domain_regs_vec_[i].push_back(temp);

        // v_offset (custom)
        // temp.index = 0x60b1;
        // temp.subindex = 0x00;
        // temp.offset = &igh_domain_pdo_output_offsets_vec_[i].off_v_offset;
        // igh_domain_regs_vec_[i].push_back(temp);

        // input
        // status word
        temp.index = 0x6041;
        temp.subindex = 0x00;
        temp.offset = &igh_domain_pdo_input_offsets_vec_[i].off_status_word;
        igh_domain_regs_vec_[i].push_back(temp);

        // actual position
        temp.index = 0x6064;
        temp.subindex = 0x00;
        temp.offset =
            &igh_domain_pdo_input_offsets_vec_[i].off_position_actual_value;
        igh_domain_regs_vec_[i].push_back(temp);

        // following error
        temp.index = 0x60f4;
        temp.subindex = 0x00;
        temp.offset = &igh_domain_pdo_input_offsets_vec_[i]
                           .off_following_error_actual_value;
        igh_domain_regs_vec_[i].push_back(temp);

        // final
        igh_domain_regs_vec_[i].push_back({});

        if (ecrt_domain_reg_pdo_entry_list(igh_domain_vec_[i],
                                           igh_domain_regs_vec_[i].data())) {
            s_logger->error("PDO entry registration failed: servo {}", i);
            return false;
        }

        s_logger->info("servo {} configured", i);
    }

    for (int i = 0; i < servo_num_; ++i) {
        // configure dc
        // Panasonic's `assign_activate` should be 0x0300
        // ecrt_slave_config_sdo16(igh_sc_vec_[i], 0x1c32, 1, 2);
        // ecrt_slave_config_sdo16(igh_sc_vec_[i], 0x1c33, 1, 2);
        ecrt_slave_config_dc(igh_sc_vec_[i], 0x0300, motion_cycle_us * 1000,
                             ecat_sync0_shift_time_ns, 0, 0);
        s_logger->info("ecrt_slave_config_dc {} : {}, {}", i,
                       motion_cycle_us * 1000, ecat_sync0_shift_time_ns);
    }

    // select the first slave to be the reference clock
    int ret = ecrt_master_select_reference_clock(igh_master_, igh_sc_vec_[0]);
    if (ret != 0) {
        s_logger->error("ecrt_master_select_reference_clock failed: {}",
                        strerror(-ret));
    }

    // TODO io domains

    // activate master
    s_logger->info("Activating master...");
    if (ecrt_master_activate(igh_master_)) {
        s_logger->error("ecrt_master_activate failed");
        return false;
    }

    for (uint32_t i = 0; i < igh_domain_vec_.size(); ++i) {
        igh_domain_pd_vec_[i] = ecrt_domain_data(igh_domain_vec_[i]);
        if (!igh_domain_pd_vec_[i]) {
            s_logger->error("ecrt_domain_data failed: slave {}", i);
            return false;
        }
    }

    // create servo device interface
    for (uint32_t i = 0; i < servo_num_; ++i) {
        servo_devices_[i] = std::make_shared<PanasonicServoDevice>(
            igh_domain_pd_vec_[i], igh_domain_pdo_input_offsets_vec_[i],
            igh_domain_pdo_output_offsets_vec_[i]);
    }

    return true;
#endif // EDM_ECAT_DRIVER_IGH

    assert(false); // should not be here
}

#ifdef EDM_ECAT_DRIVER_SOEM
void EcatManager::_soem_conf_servo_pdo_map() {
    for (int i = 0; i < servo_num_; ++i) {
        s_logger->trace("_conf_servo_pdo_map: {}", i);
        servo_devices_[i] = std::make_shared<PanasonicServoDevice>(
            (Panasonic_A5B_Ctrl *)(ec_slave[i + 1].outputs),
            (Panasonic_A5B_Stat *)(ec_slave[i + 1].inputs));
    }
}

void EcatManager::_soem_conf_servo_default_op() {
    for (int i = 0; i < servo_num_; ++i) {
        servo_devices_[i]->set_operation_mode(OM_CSP);
    }
}
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_SOEM
bool EcatManager::_soem_check_receive_valid(int wkc) {
    const int expected_wkc = (servo_num_ + io_num_) * 3;
    if (wkc < expected_wkc || wkc < 0) {
        s_logger->warn("soem wkc not ok: {}", wkc);
        return false;
    } else {
        return true;
    }
}
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
bool EcatManager::_igh_check_receive_valid() {
    // get master state
    ec_master_state_t master_state;
    ecrt_master_state(igh_master_, &master_state);

    if (master_state.slaves_responding != servo_num_ + io_num_) [[unlikely]] {
        s_logger->warn("igh slaves_responding: {}",
                       (int)master_state.slaves_responding);
        return false;
    }

    if (!master_state.link_up) [[unlikely]] {
        s_logger->warn("igh link down");
        return false;
    }

    if (master_state.al_states != 0b1000) [[unlikely]] {
        s_logger->warn("igh al_states: {:4b}", (uint8_t)master_state.al_states);
        return false;
    }

    // check domain state
    ec_domain_state_t domain_state;
    for (uint32_t i = 0; i < igh_domain_vec_.size(); ++i) {
        ecrt_domain_state(igh_domain_vec_[i], &domain_state);

        if (domain_state.working_counter != 3) [[unlikely]] {
            s_logger->warn("igh domain {} wkc {}", i,
                           (int)domain_state.working_counter);
            return false;
        }

        if (domain_state.wc_state != ec_wc_state_t::EC_WC_COMPLETE)
            [[unlikely]] {
            s_logger->warn("igh domain {} wc_state {}", i,
                           (int)domain_state.wc_state);
            return false;
        }
    }

    // EDM_CYCLIC_LOG(s_logger->debug, 1000, "master: {}, {}, {}",
    //                (int)master_state.slaves_responding,
    //                (bool)master_state.link_up,
    //                (int)master_state.al_states);
    // EDM_CYCLIC_LOG(s_logger->debug, 1000, "domain: {}, {}",
    //                (int)domain_state.working_counter,
    //                (bool)domain_state.wc_state);

    return true;
}
#endif // EDM_ECAT_DRIVER_IGH

bool EcatManager::connect_ecat(uint32_t max_try_times) {
    if (connected_)
        return true;

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

    connected_ = true;
    wkc_failed_sc.clear(); // 必须清除, 不然无法重联

    return true;
}

void EcatManager::disconnect_ecat() {
#ifdef EDM_ECAT_DRIVER_SOEM
    ec_slave[0].state = EC_STATE_INIT;
    /* request INIT state for all slaves */
    ec_writestate(0);
    ec_close();
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    if (igh_master_) {
        // write init state to each slave !
        // TODO

        s_logger->debug("deactivate igh master");
        ecrt_master_deactivate(igh_master_);
    }
#endif // EDM_ECAT_DRIVER_IGH
    s_logger->info("ecat disconnected.");

    connected_ = false;
}

// void EcatManager::ecat_sync(const std::function<void(void)>
// &do_pdo_assign_func,
//                             bool check_state_valid) {
//     /* receive progress data */
// #ifdef EDM_ECAT_DRIVER_SOEM
//     int wkc;
//     wkc = ec_receive_processdata(80); // at most 200 us
// #endif                                // EDM_ECAT_DRIVER_SOEM

// #ifdef EDM_ECAT_DRIVER_IGH
//     ecrt_master_receive(igh_master_);
//     for (auto domain : igh_domain_vec_) {
//         ecrt_domain_process(domain);
//     }
// #endif // EDM_ECAT_DRIVER_IGH

//     /* state check */
//     if (check_state_valid) {
// #ifdef EDM_ECAT_DRIVER_SOEM
//         auto state_check = _soem_check_receive_valid(wkc);
// #endif // EDM_ECAT_DRIVER_SOEM
// #ifdef EDM_ECAT_DRIVER_IGH
//         auto state_check = _igh_check_receive_valid();
// #endif // EDM_ECAT_DRIVER_IGH

//         if (!state_check) {
//             wkc_failed_sc.push_back_valid();
//         } else [[likely]] {
//             wkc_failed_sc.push_back_invalid();
//         }

//         if (wkc_failed_sc.valid_rate() > wkc_failed_threshold) [[unlikely]] {
//             s_logger->critical("ecat valid err: {}",
//                                wkc_failed_sc.valid_rate());
//             connected_ = false;
// #ifdef EDM_ECAT_DRIVER_SOEM
//             // disconnect_ecat();
//             ec_close();
// #endif // EDM_ECAT_DRIVER_SOEM

// #ifdef EDM_ECAT_DRIVER_IGH
//             ecrt_master_deactivate(igh_master_);
// #endif // EDM_ECAT_DRIVER_IGH
//             return;
//         }
//     }

//     /* Do PDO assignment using callback */
//     if (do_pdo_assign_func) [[likely]] {
//         do_pdo_assign_func();
//     }

//     // igh_sync_reference_clock_to();

//     /* send progress data */
// #ifdef EDM_ECAT_DRIVER_SOEM
//     int send_ret = ec_send_processdata();
// #endif // EDM_ECAT_DRIVER_SOEM

// #ifdef EDM_ECAT_DRIVER_IGH
//     for (auto domain : igh_domain_vec_) {
//         ecrt_domain_queue(domain);
//     }
//     ecrt_master_send(igh_master_);
// #endif // EDM_ECAT_DRIVER_IGH
// }

bool EcatManager::receive_check(int wkc [[maybe_unused]]) {
#ifdef EDM_ECAT_DRIVER_SOEM
    auto state_check = _soem_check_receive_valid(wkc);
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
    auto state_check = _igh_check_receive_valid();
#endif // EDM_ECAT_DRIVER_IGH

    if (!state_check) {
        wkc_failed_sc.push_back_valid();
    } else [[likely]] {
        wkc_failed_sc.push_back_invalid();
    }

    if (wkc_failed_sc.valid_rate() > wkc_failed_threshold) [[unlikely]] {
        s_logger->critical("igh ecat valid err: {}",
                           wkc_failed_sc.valid_rate());
        connected_ = false;
#ifdef EDM_ECAT_DRIVER_SOEM
        ec_close();
#endif // EDM_ECAT_DRIVER_SOEM
#ifdef EDM_ECAT_DRIVER_IGH
        ecrt_master_deactivate(igh_master_);
#endif // EDM_ECAT_DRIVER_IGH
        return false;
    }

    return true;
}

#ifdef EDM_ECAT_DRIVER_IGH
void EcatManager::igh_master_receive() { ecrt_master_receive(igh_master_); }
void EcatManager::igh_domain_process() {
    for (auto domain : igh_domain_vec_) {
        ecrt_domain_process(domain);
    }
}
void EcatManager::igh_domain_queue() {
    for (auto domain : igh_domain_vec_) {
        ecrt_domain_queue(domain);
    }
}
void EcatManager::igh_master_send() { ecrt_master_send(igh_master_); }

void EcatManager::igh_tell_application_time(uint64_t app_time) {
    ecrt_master_application_time(igh_master_, app_time);
}

void EcatManager::igh_sync_reference_clock_to(uint64_t time) {
    ecrt_master_sync_reference_clock_to(igh_master_, time);
}

void EcatManager::igh_sync_reference_clock() {
    ecrt_master_sync_reference_clock(igh_master_);
}

uint32_t EcatManager::igh_get_reference_clock_time() {
    uint32_t ref_time = 0;
    ecrt_master_reference_clock_time(igh_master_, &ref_time);
    return ref_time;
}

void EcatManager::igh_sync_slave_clocks() {
    ecrt_master_sync_slave_clocks(igh_master_);
}

bool EcatManager::igh_check_op() {
    // get master state
    ec_master_state_t master_state;
    ecrt_master_state(igh_master_, &master_state);

    return master_state.al_states == 0b1000;
}
#endif // EDM_ECAT_DRIVER_IGH

#ifdef EDM_ECAT_DRIVER_SOEM
int EcatManager::soem_master_receive() { return ec_receive_processdata(80); }

void EcatManager::soem_master_send() { ec_send_processdata(); }

/* PI calculation to get linux time synced to DC time */
static void ec_sync(int64_t reftime, int64_t cycletime, int64_t *offsettime) {
    static int64_t integral = 0;
    int64_t delta;
    /* set linux sync point 50us later than DC sync, just as example */
    delta = (reftime - 50000) % cycletime;
    if (delta > (cycletime / 2)) {
        delta = delta - cycletime;
    }
    if (delta > 0) {
        integral++;
    }
    if (delta < 0) {
        integral--;
    }
    *offsettime = -(delta / 100) - (integral / 20);
    //    gl_delta = delta;

    // EDM_CYCLIC_LOG(s_logger->debug, 500, "ref {}, delta: {}, integral: {}",
    // reftime, delta, integral);
}

/* PI calculation to get linux time synced to DC time */
void EcatManager::soem_dc_sync_time(int64_t cycletime, int64_t *offsettime) {
    if (!this->connected_) {
        *offsettime = 0;
        return;
    }

    if (!ec_slave[0].hasdc) {
        *offsettime = 0;
    } else {
        ec_sync(ec_DCtime, cycletime, offsettime);
    }
}
#endif // EDM_ECAT_DRIVER_SOEM

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
    int i = 0;
    for (auto &servo : servo_devices_) {
        servo->set_operation_mode(OM_CSP);
        // s_logger->debug("i:{}, sw:{:08b}", i, servo->get_status_word());

        if (servo->sw_fault()) {
            // s_logger->debug("sw_fault");
            servo->cw_fault_reset();
        } else if (servo->sw_switch_on_disabled()) {
            // s_logger->debug("sw_switch_on_disabled");
            servo->cw_shut_down();
        } else if (servo->sw_ready_to_switch_on()) {
            // s_logger->debug("sw_ready_to_switch_on");
            servo->cw_switch_on();
        } else if (servo->sw_switched_on() || servo->sw_operational_enabled()) {
            // s_logger->debug("sw_switched_on: {}, sw_operational_enabled: {}", servo->sw_switched_on(), servo->sw_operational_enabled());
            servo->cw_enable_operation();
            servo->sync_actual_position_to_target_position(); // to ensure the
                                                              // servo would not
                                                              // move suddenly
                                                              // or die again
        }
        ++i;
        // EDM_CYCLIC_LOG(s_logger->debug, 2000, "sw: {0:x}, {0:b}, fault: {1}", servo->get_status_word(), servo->sw_fault());
    }
}

void EcatManager::disable_cycle_run_once() {
    //! disabled 状态定义为 switch_on_disabled 状态
    for (auto &servo : servo_devices_) {
        servo->set_operation_mode(OM_CSP);

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