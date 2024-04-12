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

#include <json.hpp>

namespace edm {

namespace _sys {

struct _ecat_setting {
    uint32_t ecat_iomap_size{4096};
    std::string ecat_netif_name; // need to specify, based on different netif

    MEO_JSONIZATION(MEO_OPT ecat_iomap_size, ecat_netif_name);
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

class _SystemSettingsData final {
public:
    _SystemSettingsData() noexcept = default;

public: // settings
    std::string coord_config_file{"coord.json"};
    std::string log_config_file{"logdefine.json"};
    std::string qss_file{"gui.qss"};
    std::string can_device_name{"can0"};
    uint32_t can_device_bitrate{500000};
    _ecat_setting ecat;
    _fast_move_param fast_move_param;
    std::string power_database_file{"power.db"};
    std::string helper_can_device_name{"can1"};
    std::string interp_module_path_relative_to_root{
        "Src/Interpreter/rs274pyInterpreter/pymodule/"};
    uint32_t info_dispatcher_peroid_ms{20};
    _jump_param jump_param;
    std::string datasave_dir;
    uint32_t motion_cycle_us {1000};
    uint32_t monitor_peroid_ms{50};

    MEO_JSONIZATION(MEO_OPT coord_config_file, MEO_OPT log_config_file,
                    MEO_OPT qss_file, MEO_OPT can_device_name,
                    MEO_OPT can_device_bitrate, ecat, MEO_OPT fast_move_param,
                    MEO_OPT power_database_file, MEO_OPT helper_can_device_name,
                    MEO_OPT interp_module_path_relative_to_root,
                    MEO_OPT info_dispatcher_peroid_ms, MEO_OPT jump_param,
                    MEO_OPT datasave_dir, MEO_OPT motion_cycle_us,
                    MEO_OPT monitor_peroid_ms);
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
        return data_.coord_config_file;
    }
    const std::string &get_log_config_file() const {
        return data_.log_config_file;
    }
    const std::string &get_qss_file() const { return data_.qss_file; }

    // can
    const std::string &get_can_device_name() const {
        return data_.can_device_name;
    }
    uint32_t get_can_device_bitrate() const { return data_.can_device_bitrate; }

    const std::string &get_helper_can_device_name() const {
        return data_.helper_can_device_name;
    }

    // ecat
    uint32_t get_ecat_iomap_size() const { return data_.ecat.ecat_iomap_size; }
    const std::string &get_ecat_netif_name() const {
        return data_.ecat.ecat_netif_name;
    }

    // speed
    double get_fmparam_max_acc_um_s2() const {
        return data_.fast_move_param.max_acc_um_s2;
    }
    uint32_t get_fmparam_nacc_ms() const {
        return data_.fast_move_param.nacc_ms;
    }
    double get_fmparam_speed_0_um_s() const {
        return data_.fast_move_param.speed_0_um_s;
    }
    double get_fmparam_speed_1_um_s() const {
        return data_.fast_move_param.speed_1_um_s;
    }
    double get_fmparam_speed_2_um_s() const {
        return data_.fast_move_param.speed_2_um_s;
    }
    double get_fmparam_speed_3_um_s() const {
        return data_.fast_move_param.speed_3_um_s;
    }

    // power db
    const std::string &get_power_database_file() const {
        return data_.power_database_file;
    }

    // interp module
    const std::string &get_interp_module_path_relative_to_root() const {
        return data_.interp_module_path_relative_to_root;
    }

    uint32_t get_info_dispatcher_peroid_ms() const {
        return data_.info_dispatcher_peroid_ms;
    }

    const auto &get_jump_param() const { return data_.jump_param; }

    const auto &get_datasave_dir() const { return data_.datasave_dir; }

    uint32_t get_motion_cycle_us() const { return data_.motion_cycle_us; }

    uint32_t get_monitor_peroid_ms() const { return data_.monitor_peroid_ms; }

public:
    // TODO change settings and save to local file
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
