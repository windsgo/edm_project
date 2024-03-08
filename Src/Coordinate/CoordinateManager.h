#pragma once

#include "Coordinate.h"

#include <unordered_map> // coord_index -> Coordinate
#include <memory>
#include <optional>

#include <json.hpp>

namespace edm {

namespace coord {

// 负责维护坐标系列表及其偏置
// 每次对坐标系有增、删、改操作, 都会触发文件操作, 将当前的坐标系列表整个保存入json文件
class CoordinateManager final {
public:
    // 另存为, 备份, 不会改变当前默认保存的文件名
    void save_as(const std::string& filename) const;

    // 变更对应文件
    void set_filename(const std::string& new_filename);
    const std::string& get_filename() const { return filename_; }

    // 获取内部数据
    const auto& data() const { return coordinates_map_; }

public: // change
    

public:
    // 默认构造函数, 不包含任何坐标系集合
    CoordinateManager() noexcept = default;

    // 从json文件构造, 如果文件不存在或文件格式错误, 则会构建默认的坐标系集合G54~G59
    CoordinateManager(const std::string& filename);

    // 禁用拷贝构造
    CoordinateManager(const CoordinateManager& rhs) = delete;
    CoordinateManager& operator=(const CoordinateManager& rhs) = delete;

    // 支持移动构造
    CoordinateManager(CoordinateManager&& rhs) noexcept = default;
    CoordinateManager& operator=(CoordinateManager&& rhs) noexcept = default;

private:
    bool _parse_json_to_map(const json::value& value);
    void _reset_and_generate_default_coord();
    void _save_to_file(const std::string& filename) const;

    json::value _to_json() const;

private:
    // all members default movable
    std::string filename_; // 用于在修改时保存到的文件目录
    std::unordered_map<uint32_t, Coordinate> coordinates_map_;
};

} // namespace coord

} // namespace edm
