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

class SystemSettingsData final {
public:
    SystemSettingsData() noexcept = default;

public: // settings
    std::string coord_config_file;
    std::string log_config_file;
    std::string qss_file;
    std::string can_device_name;
    uint32_t can_device_bitrate;
    uint32_t ecat_iomap_size;
    std::string ecat_netif_name;

    MEO_JSONIZATION(coord_config_file, log_config_file, qss_file,
                    can_device_name, can_device_bitrate, ecat_iomap_size,
                    ecat_netif_name);
};

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

    const std::string &get_can_device_name() const {
        return data_.can_device_name;
    }

    uint32_t get_can_device_bitrate() const { return data_.can_device_bitrate; }

    uint32_t get_ecat_iomap_size() const { return data_.ecat_iomap_size; }

    const std::string &get_ecat_netif_name() const {
        return data_.ecat_netif_name;
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
            data_ = (SystemSettingsData)ret;
        } catch (const std::exception &e) {
            throw exception(EDM_FMT::format(
                "system settings init failed: convert ex: {}: {}", e.what(),
                GetSysConfigPath()));
        }

        ifs.close();
    }

private:
    SystemSettingsData data_;
};

} // namespace edm
