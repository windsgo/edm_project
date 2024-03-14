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
    double max_acc_um_s2{500000}; // um / s
    uint32_t nacc_ms{60};         // nacc ms

    double speed_0_um_s{33000}; // 0档速度
    double speed_1_um_s{6600};  // 1档速度
    double speed_2_um_s{330};   // 2档速度
    double speed_3_um_s{16};
    // 3 档速度是um移动, 但是也给出, 用于非按钮点动时

    MEO_JSONIZATION(MEO_OPT max_acc_um_s2, MEO_OPT nacc_ms,
                    MEO_OPT speed_0_um_s, MEO_OPT speed_1_um_s,
                    MEO_OPT speed_2_um_s, MEO_OPT speed_3_um_s);
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

    MEO_JSONIZATION(MEO_OPT coord_config_file, MEO_OPT log_config_file,
                    MEO_OPT qss_file, MEO_OPT can_device_name,
                    MEO_OPT can_device_bitrate, ecat, MEO_OPT fast_move_param,
                    MEO_OPT power_database_file);
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

    // ecat
    uint32_t get_ecat_iomap_size() const { return data_.ecat.ecat_iomap_size; }
    const std::string &get_ecat_netif_name() const {
        return data_.ecat.ecat_netif_name;
    }

    // speed
    double get_fmparam_max_acc_um_s2() const {
        return data_.fast_move_param.max_acc_um_s2;
    }
    double get_fmparam_nacc_ms() const { return data_.fast_move_param.nacc_ms; }
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

public:
    // TODO change settings and save to local file

private:
    bool _save_to_file(const std::string &filename) const;

private:
    SystemSettings() {
        std::ifstream ifs(GetSysConfigPath());
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
    _sys::_SystemSettingsData data_;
};

} // namespace edm
