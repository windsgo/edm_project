#include "CoordinateSystem.h"

#include <filesystem>
#include <optional>
#include <string>

#include "Exception/exception.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Utils/Format/edm_format.h"

#include "Logger/LogMacro.h"
#include <json.hpp> // meo json

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

static std::optional<std::string>
_create_backup_file_for(const std::string ori_file) {
    auto bak_file = ori_file + ".bak";
    if (std::filesystem::exists(bak_file)) {
        int i = 1;
        while (true || i > 20) {
            auto bak_i_file = bak_file + "." + std::to_string(i);
            if (!std::filesystem::exists(bak_i_file)) {
                bak_file = std::move(bak_i_file);
                break;
            }
            ++i;
        }
    }

    auto ret = std::filesystem::copy_file(
        ori_file, bak_file, std::filesystem::copy_options::skip_existing);
    if (ret) {
        return std::move(bak_file);
    } else {
        return std::nullopt;
    }
}

namespace edm {

namespace coord {

CoordinateSystem::CoordinateSystem(const std::string &filename)
    : filename_(filename) {
    if (!std::filesystem::path{filename}.has_filename()) {
        throw exception(EDM_FMT::format(
            "invalid coord config filename: {}, not a file path", filename));
    }

    // 创建 cm
    _init_coordinate_manager(filename);

    // 初始化默认的坐标系编号
    _init_curr_index();

    // 初始化坐标系缓存
    _update_machine_axis_cache();
    _update_coord_axis_cache();

    move::MotionUtils::ClearAxis(curr_using_v_offsets_);
}

void CoordinateSystem::update_motor_pos(const move::axis_t &new_motor_pos,
                                        const move::axis_t &new_motor_pos_act) {
    curr_motor_axis_ = new_motor_pos;
    curr_motor_axis_act_ = new_motor_pos_act;

    _update_machine_axis_cache();
    _update_coord_axis_cache();
}

bool CoordinateSystem::set_coord_index(uint32_t new_coord_index) {
    const auto &cm_map = cm_.get_coordinates_map();

    auto ret = cm_map.find(new_coord_index);
    if (ret == cm_map.end()) {
        return false;
    }

    curr_coord_index_ = ret->first;
    _update_coord_axis_cache();

    return true;
}

bool CoordinateSystem::get_coord_axis(uint32_t coord_index,
                                      move::axis_t &output) const {
    if (coord_index == curr_coord_index_) {
        output = curr_coord_axis_cache_;
        return true;
    }

    return cm_.motor_to_coord(coord_index, curr_motor_axis_, output);
}

bool CoordinateSystem::set_current_coord_offset(const move::axis_t &offset) {
    bool ret = cm_.set_coord_offset(curr_coord_index_, offset);
    if (ret) {
        cm_.save_as(filename_);
    }

    return ret;
}

bool CoordinateSystem::set_coord_offset(uint32_t index,
                                        const move::axis_t &offset) {
    bool ret = cm_.set_coord_offset(index, offset);
    if (ret) {
        cm_.save_as(filename_);
    }

    return ret;
}

void CoordinateSystem::set_global_offset(const move::axis_t &offset) {
    cm_.set_global_offset(offset);
    cm_.save_as(filename_);

    _update_machine_axis_cache();
}

bool CoordinateSystem::set_soft_limits(const CoordSoftLimit &csl) {
    if (!cm_.set_soft_limits(csl)) {
        return false;
    }

    cm_.save_as(filename_);
    return true;
}

void CoordinateSystem::_update_machine_axis_cache() {
    cm_.motor_to_machine(curr_motor_axis_, curr_machine_axis_cache_);
    cm_.motor_to_machine(curr_motor_axis_act_, curr_machine_axis_cache_act_);
}

void CoordinateSystem::_update_coord_axis_cache() {
    cm_.motor_to_coord(curr_coord_index_, curr_motor_axis_,
                       curr_coord_axis_cache_);
    cm_.motor_to_coord(curr_coord_index_, curr_motor_axis_act_,
                       curr_coord_axis_cache_act_);
}

bool CoordinateSystem::_init_coordinate_manager(const std::string &filename) {
    auto ret = CoordinateManager::LoadFromJsonFile(filename);
    if (ret) {
        cm_ = std::move(*ret);
        return true;
    }

    s_logger->error("init coord from file [{}] failed.", filename);

#ifdef EDM_INVALID_COORD_CONFIG_THROW
    throw exception(EDM_FMT::format("init coord from file [{}] failed.", filename));
    return false;
#else // EDM_INVALID_COORD_CONFIG_THROW

    s_logger->info("creating default coord G54 ~ G59 ...");

    cm_ = CoordinateManager();

    // 手动插入默认坐标系
    CoordinateManager::map_t coord_map;
    for (uint32_t i = 54; i <= 59; ++i) {
        coord_map.emplace(i, Coordinate{i, coord_offset_t{0.0}});
    }

    cm_.set_coordinates_map(std::move(coord_map));

    // 如果输入的filename存在, 为其创建备份文件
    if (std::filesystem::exists(filename) &&
        !std::filesystem::is_directory(filename)) {
        auto backup_ret = _create_backup_file_for(filename);
        if (backup_ret) {
            s_logger->info("backup wrong coord config to: {}", *backup_ret);
        } else {
            s_logger->error("backup wrong coord config failed");
        }
    }

    // 覆盖filename的文件为当前坐标系文件
    bool save_ret = cm_.save_as(filename);
    if (save_ret) {
        s_logger->info("default coord config saved successfully to: {}",
                       filename);
    } else {
        s_logger->error("default coord config saved failed to: {}", filename);
    }

    return true;
#endif // EDM_INVALID_COORD_CONFIG_THROW
}

void CoordinateSystem::_init_curr_index() {
    auto avaiable_coord_indexes = cm_.get_avaiable_coord_indexes();
    if (avaiable_coord_indexes.empty()) {
        throw exception("no avaiable_coord_indexes");
    }

    curr_coord_index_ = avaiable_coord_indexes[0];
}

} // namespace coord

} // namespace edm