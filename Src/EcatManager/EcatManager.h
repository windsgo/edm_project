#pragma once

#include "EcatDefine.h"
#include "EcatManager/ServoDefines.h"
#include "ServoDevice.h"

#include "Utils/Filters/SlidingCounter/SlidingCounter.h"
#include "config.h"
#include <array>

#ifdef EDM_ECAT_DRIVER_IGH
#include "ecrt.h"
#endif // EDM_ECAT_DRIVER_IGH

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
namespace edm {

namespace ecat {

class EcatManager final {
public:
    using ptr = std::shared_ptr<EcatManager>;
    // TODO EcatManager(std::string_view conf_path);
    EcatManager(std::string_view ifname, std::size_t iomap_size,
                uint32_t servo_num, uint32_t io_num = 0);
    ~EcatManager();

    EcatManager(const EcatManager &) = delete;
    EcatManager &operator=(const EcatManager &) = delete;
    EcatManager(EcatManager &&) = delete;
    EcatManager &operator=(EcatManager &&) = delete;

    // do ethercat config and init
    // if all success, and slave numbers match
    // returns true, otherwise false
    bool connect_ecat(uint32_t max_try_times = 3);
    inline bool is_ecat_connected() const { return connected_; };

    void disconnect_ecat();

    // recv first, then assign the pdo tx data, then send
    // that is `recv - pdo assign - send - calc` sequence
    // which is much better than `send - recv - calc - pdo assign` sequence
    // void ecat_sync(const std::function<void(void)> &do_pdo_assign_func,
    //                bool check_state_valid = false);

    bool receive_check(int wkc = -1);

#ifdef EDM_ECAT_DRIVER_IGH
    /** IGH Test Interface */
    void igh_master_receive();
    void igh_domain_process();
    void igh_domain_queue();
    void igh_master_send();

    // igh time sync things
    void igh_tell_application_time(uint64_t app_time);

    // 以主战为同步时钟方法
    void igh_sync_reference_clock_to(uint64_t time);
    void igh_sync_reference_clock();

    // 获取从站的参考时钟
    uint32_t igh_get_reference_clock_time();

    // sync slaves
    void igh_sync_slave_clocks();

    // 检查是否所有从站到达OP
    bool igh_check_op();
#endif // EDM_ECAT_DRIVER_IGH

#ifdef EDM_ECAT_DRIVER_SOEM
    int soem_master_receive();
    void soem_master_send();

    // calulate toff to get linux time and DC synced
    void soem_dc_sync_time(int64_t cycletime, int64_t *offsettime);
#endif // EDM_ECAT_DRIVER_SOEM

    /* servo related interfaces */
    ServoDevice::ptr get_servo_device(uint32_t servo_index) const;
    void set_servo_target_position(uint32_t servo_index,
                                   int32_t target_position);
    int32_t get_servo_actual_position(uint32_t servo_index) const;

    bool servo_has_fault() const;
    bool servo_all_operation_enabled() const;
    bool servo_all_disabled() const;

    void clear_fault_cycle_run_once();

    //! note: if has fault, it will clear fault first.
    void disable_cycle_run_once();

private:
    bool _connect_ecat_try_once(int expected_slavecount);

#ifdef EDM_ECAT_DRIVER_SOEM
    bool _soem_wait_slaves_to_safe_op();
    bool _soem_wait_slaves_to_operational();

    void _soem_conf_servo_pdo_map();
    void _soem_conf_servo_default_op();
#endif // EDM_ECAT_DRIVER_SOEM

private:
#ifdef EDM_ECAT_DRIVER_SOEM
    bool _soem_check_receive_valid(int wkc);
#endif // EDM_ECAT_DRIVER_SOEM
#ifdef EDM_ECAT_DRIVER_IGH
    bool _igh_check_receive_valid();
#endif // EDM_ECAT_DRIVER_IGH

private: // soem things
    std::string ifname_;
    std::size_t iomap_size_;
    char *iomap_;

private:
    volatile bool connected_ = false;

    uint32_t servo_num_;
    std::vector<ServoDevice::ptr> servo_devices_;
    // TODO if io ecat devices added

    uint32_t io_num_;

    // used for wkc filter
    util::SlidingCounter<100> wkc_failed_sc;
    const double wkc_failed_threshold = 0.90;

private: // igh things
#ifdef EDM_ECAT_DRIVER_IGH
    ec_master_t *igh_master_{nullptr};

    std::vector<ec_slave_config_t *> igh_sc_vec_;

    // 每一个从站分配一个domain
    // std::vector<ec_domain_t *> igh_domain_vec_;
    ec_domain_t* igh_domain_instance_{nullptr}; // test single domain

    // 每一个从站的domain的pdo数据指针
    std::vector<uint8_t *> igh_domain_pd_vec_;

    // 每一个从站的domain的pdo注册结构体数组
    std::vector<std::vector<ec_pdo_entry_reg_t>> igh_domain_regs_vec_;

    // 每一个从站的pdo offset
    std::vector<Panasonic_A6B_InputDomainOffsets>
        igh_domain_pdo_input_offsets_vec_;
    std::vector<Panasonic_A6B_OutputDomainOffsets>
        igh_domain_pdo_output_offsets_vec_;

    int sync_ref_counter_{0};
#endif // EDM_ECAT_DRIVER_IGH
};

} // namespace ecat

} // namespace edm
