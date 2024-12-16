#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Exception/exception.h"
#include "Utils/Format/edm_format.h"

#include "config.h" // EDM_SYSTEM_SETTINGS_CONFIG_FILE
#include "reflection/jsonization.hpp"

#include <json.hpp>

namespace edm {

namespace _sys {

struct _ecat_setting {
    uint32_t ecat_iomap_size{4096};
    std::string ecat_netif_name; // need to specify, based on different netif
    uint32_t ecat_sync0_shift_time_ns{90000};
    uint32_t dc_filter_cnt{1024};
    uint32_t igh_op_wait_count_max{10000};

    MEO_JSONIZATION(MEO_OPT ecat_iomap_size, ecat_netif_name,
                    MEO_OPT ecat_sync0_shift_time_ns, MEO_OPT dc_filter_cnt,
                    MEO_OPT igh_op_wait_count_max);
};

struct _fast_move_param {
    double max_acc_um_s2{300000}; // um / s
    uint32_t nacc_ms{100};        // nacc ms

    double speed_0_um_s{33000}; // 0档速度
    double speed_1_um_s{6600};  // 1档速度
    double speed_2_um_s{330};   // 2档速度
    double speed_3_um_s{16};
    // 3 档速度是um移动, 但是也给出, 用于非按钮点动时

    MEO_JSONIZATION(MEO_OPT max_acc_um_s2, MEO_OPT nacc_ms,
                    MEO_OPT speed_0_um_s, MEO_OPT speed_1_um_s,
                    MEO_OPT speed_2_um_s, MEO_OPT speed_3_um_s);
};

struct _jump_param {
    double max_acc_um_s2{1440000}; // um / s
    uint32_t nacc_ms{60};          // nacc ms
    uint32_t buffer_um{30};        // um

    MEO_JSONIZATION(MEO_OPT max_acc_um_s2, MEO_OPT nacc_ms, MEO_OPT buffer_um);
};

struct _motion_settings {
    bool enable_g01_run_each_servo_cmd{true};
    bool enable_g01_half_closed_loop{true};

    MEO_JSONIZATION(MEO_OPT enable_g01_run_each_servo_cmd,
                    MEO_OPT enable_g01_half_closed_loop);
};

struct _zynq_settings {
    std::string zynq_tcp_server_ip{"192.168.1.130"};
    uint32_t zynq_tcp_server_port{12355};
    uint32_t zynq_udp_local_port{12365};

    MEO_JSONIZATION(MEO_OPT zynq_tcp_server_ip, MEO_OPT zynq_tcp_server_port,
                    MEO_OPT zynq_udp_local_port);
};

struct _zynq_adc_settings {
    double adc_offset;
    double adc_gain;
    uint32_t voltage_filter_window_time_us;

    MEO_JSONIZATION(MEO_OPT adc_offset, MEO_OPT adc_gain,
                    MEO_OPT voltage_filter_window_time_us);
};

struct _can_settings {
    std::string can_device_name{"can0"};
    std::string helper_can_device_name{"can1"};
    uint32_t can_device_bitrate{500000};

    MEO_JSONIZATION(MEO_OPT can_device_name, MEO_OPT helper_can_device_name,
                    MEO_OPT can_device_bitrate);
};

struct _file_settings {
    std::string coord_config_file{"coord.json"};
    std::string log_config_file{"logdefine.json"};
    std::string qss_file{"gui.qss"};
    std::string power_database_file{"power.db"};
    std::string interp_module_path_relative_to_root{
        "Src/Interpreter/rs274pyInterpreter/pymodule/"};
    std::string datasave_dir{"Data/"};

    MEO_JSONIZATION(MEO_OPT coord_config_file, MEO_OPT log_config_file,
                    MEO_OPT qss_file, MEO_OPT power_database_file,
                    MEO_OPT datasave_dir,
                    MEO_OPT interp_module_path_relative_to_root)
};

struct _time_settings {
    uint32_t info_dispatcher_peroid_ms{20};
    uint32_t motion_cycle_us{1000};
    uint32_t monitor_peroid_ms{50};

    MEO_JSONIZATION(MEO_OPT info_dispatcher_peroid_ms, MEO_OPT motion_cycle_us,
                    MEO_OPT monitor_peroid_ms);
};

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
struct _breakout_settings {
    uint32_t voltage_average_filter_window_size{200};
    uint32_t stderr_filter_window_size{200};
    double kn_valid_threshold{20};
    uint32_t kn_sc_window_size{300};
    double kn_valid_rate_threshold{0.01};
    uint32_t kn_valid_rate_ok_cnt_threshold{320};
    uint32_t kn_valid_rate_ok_cnt_maximum{600};

    double max_move_um_after_breakout_start_detected{200};
    double breakout_start_detect_length_percent{0.25};
    double speed_rate_after_breakout_start_detected{0.8};
    uint32_t wait_time_ms_after_breakout_end_judged{1000};

    uint32_t ctrl_flags{0};

    MEO_JSONIZATION(MEO_OPT voltage_average_filter_window_size,
                    MEO_OPT stderr_filter_window_size,
                    MEO_OPT kn_valid_threshold, MEO_OPT kn_sc_window_size,
                    MEO_OPT kn_valid_rate_threshold,
                    MEO_OPT kn_valid_rate_ok_cnt_threshold,
                    MEO_OPT kn_valid_rate_ok_cnt_maximum,
                    MEO_OPT max_move_um_after_breakout_start_detected,
                    MEO_OPT breakout_start_detect_length_percent,
                    MEO_OPT speed_rate_after_breakout_start_detected,
                    MEO_OPT wait_time_ms_after_breakout_end_judged,
                    MEO_OPT ctrl_flags);
};

struct _drill_settings {
    double touch_return_um{500}; // 碰边后返回距离
    double touch_speed_um_ms{2}; // 碰边速度
    
    bool auto_switch_opump{false};
    bool auto_switch_ipump{false};

    _breakout_settings breakout_params; // 穿透参数

    MEO_JSONIZATION(MEO_OPT touch_return_um, MEO_OPT touch_speed_um_ms,
                    MEO_OPT breakout_params, MEO_OPT auto_switch_opump,
                    MEO_OPT auto_switch_ipump);
};
#endif

class _SystemSettingsData final {
public:
    _SystemSettingsData() noexcept = default;

public: // settings
    _file_settings file;
    _can_settings can;
    _ecat_setting ecat;
    _fast_move_param fast_move_param;
    _jump_param jump_param;

    _motion_settings motion_settings;

    _zynq_settings zynq_settings;

    _zynq_adc_settings zynq_adc_settings;

    _time_settings time_settings;

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    _drill_settings drill_settings;
#endif

    MEO_JSONIZATION(MEO_OPT can, ecat, MEO_OPT fast_move_param,
                    MEO_OPT jump_param, MEO_OPT file, MEO_OPT time_settings,
                    MEO_OPT motion_settings, MEO_OPT zynq_settings,
                    MEO_OPT zynq_adc_settings,
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
                    MEO_OPT drill_settings
#endif
    );
};

}; // namespace _sys

class SystemSettings final {
public:
    static SystemSettings &instance() {
        static SystemSettings i;
        return i;
    }

    static const std::string &GetSysConfigPath() {
        static const std::string dir = EDM_SYSTEM_SETTINGS_CONFIG_FILE;
        return dir;
    }

public:
    const std::string &get_coord_config_file() const {
        return data_.file.coord_config_file;
    }
    const std::string &get_log_config_file() const {
        return data_.file.log_config_file;
    }
    const std::string &get_qss_file() const { return data_.file.qss_file; }

    // can
    const std::string &get_can_device_name() const {
        return data_.can.can_device_name;
    }
    uint32_t get_can_device_bitrate() const {
        return data_.can.can_device_bitrate;
    }

    const std::string &get_helper_can_device_name() const {
        return data_.can.helper_can_device_name;
    }

    // ecat
    uint32_t get_ecat_iomap_size() const { return data_.ecat.ecat_iomap_size; }
    const std::string &get_ecat_netif_name() const {
        return data_.ecat.ecat_netif_name;
    }

    // speed
    inline double get_fmparam_max_acc_um_s2() const {
        return data_.fast_move_param.max_acc_um_s2;
    }
    inline uint32_t get_fmparam_nacc_ms() const {
        return data_.fast_move_param.nacc_ms;
    }
    inline double get_fmparam_speed_0_um_s() const {
        return data_.fast_move_param.speed_0_um_s;
    }
    inline double get_fmparam_speed_1_um_s() const {
        return data_.fast_move_param.speed_1_um_s;
    }
    inline double get_fmparam_speed_2_um_s() const {
        return data_.fast_move_param.speed_2_um_s;
    }
    inline double get_fmparam_speed_3_um_s() const {
        return data_.fast_move_param.speed_3_um_s;
    }

    // power db
    const std::string &get_power_database_file() const {
        return data_.file.power_database_file;
    }

    // interp module
    inline const std::string &get_interp_module_path_relative_to_root() const {
        return data_.file.interp_module_path_relative_to_root;
    }

    inline uint32_t get_info_dispatcher_peroid_ms() const {
        return data_.time_settings.info_dispatcher_peroid_ms;
    }

    inline const auto &get_jump_param() const { return data_.jump_param; }

    inline const auto &get_datasave_dir() const {
        return data_.file.datasave_dir;
    }

    uint32_t get_motion_cycle_us() const {
        return data_.time_settings.motion_cycle_us;
    }

    uint32_t get_monitor_peroid_ms() const {
        return data_.time_settings.monitor_peroid_ms;
    }

    inline uint32_t get_ecat_sync0_shift_time_ns() const {
        return data_.ecat.ecat_sync0_shift_time_ns;
    }

    inline auto get_dc_filter_cnt() const { return data_.ecat.dc_filter_cnt; }

    inline auto get_igh_op_wait_count_max() const {
        return data_.ecat.igh_op_wait_count_max;
    }

    // MotionSettings Related
    inline auto get_enable_g01_run_each_servo_cmd() const {
        return data_.motion_settings.enable_g01_run_each_servo_cmd;
    }
    inline auto get_enable_g01_half_closed_loop() const {
        return data_.motion_settings.enable_g01_half_closed_loop;
    }

    inline const auto &get_zynq_settings() const { return data_.zynq_settings; }

    inline const auto &get_zynq_adc_settings() const {
        return data_.zynq_adc_settings;
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    inline const auto &get_drill_settings() const {
        return data_.drill_settings;
    }
#endif

public:
    // you should save to local file manually
    inline void set_fastmove_max_acc_um_s2(double v) {
        data_.fast_move_param.max_acc_um_s2 = v;
    }
    inline void set_fastmove_nacc_ms(uint32_t v) {
        data_.fast_move_param.nacc_ms = v;
    }
    inline void set_fastmove_speed_0_um_s(double v) {
        data_.fast_move_param.speed_0_um_s = v;
    }
    inline void set_fastmove_speed_1_um_s(double v) {
        data_.fast_move_param.speed_1_um_s = v;
    }
    inline void set_fastmove_speed_2_um_s(double v) {
        data_.fast_move_param.speed_2_um_s = v;
    }
    inline void set_fastmove_speed_3_um_s(double v) {
        data_.fast_move_param.speed_3_um_s = v;
    }

    inline void set_jumpparam_max_acc_um_s2(double v) {
        data_.jump_param.max_acc_um_s2 = v;
    }
    inline void set_jumpparam_nacc_ms(double v) {
        data_.jump_param.nacc_ms = v;
    }
    inline void set_jumpparam_buffer_um(double v) {
        data_.jump_param.buffer_um = v;
    }

    // MotionSettings Related
    inline void set_enable_g01_run_each_servo_cmd(bool v) {
        data_.motion_settings.enable_g01_run_each_servo_cmd = v;
    }
    inline void set_enable_g01_half_closed_loop(bool v) {
        data_.motion_settings.enable_g01_half_closed_loop = v;
    }

    // zynq adc settings change
    inline void
    set_zynq_adc_settings(const _sys::_zynq_adc_settings &zynq_adc_settings) {
        data_.zynq_adc_settings = zynq_adc_settings;
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    inline void
    set_drill_settings(const _sys::_drill_settings &drill_settings) {
        data_.drill_settings = drill_settings;
    }
#endif

public:
    bool save_to_file() const {
        return _save_to_file(SystemSettings::GetSysConfigPath());
    }

    void reload_from_file() { _reload_from_file(); }

private:
    bool _save_to_file(const std::string &filename) const;
    void _reload_from_file() {
        std::ifstream ifs(SystemSettings::GetSysConfigPath());
        if (!ifs.is_open()) {
            throw exception(EDM_FMT::format(
                "system settings init failed: config file open failed: {}",
                GetSysConfigPath()));
        }

        auto parse_ret = json::parse(ifs, false);
        if (!parse_ret) {
            throw exception(
                EDM_FMT::format("system settings init failed: parse failed: {}",
                                GetSysConfigPath()));
        }

        auto ret = std::move(*parse_ret);

        try {
            data_ = (_sys::_SystemSettingsData)ret;
        } catch (const std::exception &e) {
            throw exception(EDM_FMT::format(
                "system settings init failed: convert ex: {}: {}", e.what(),
                GetSysConfigPath()));
        }

        ifs.close();
    }

private:
    SystemSettings() { _reload_from_file(); }

private:
    _sys::_SystemSettingsData data_;
};

} // namespace edm
