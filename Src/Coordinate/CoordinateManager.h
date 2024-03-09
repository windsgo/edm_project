#pragma once

#include "Coordinate.h"

#include <memory>
#include <optional>
#include <unordered_map> // coord_index -> Coordinate

#include <json.hpp>

namespace edm {

namespace coord {

// 负责维护坐标系列表及其偏置
// 每次对坐标系有增、删、改操作, 都会触发文件操作,
// 将当前的坐标系列表整个保存入json文件
class CoordinateManager final {
public:
    using map_t = std::unordered_map<uint32_t, Coordinate>;

public:
    static std::optional<CoordinateManager>
    LoadFromJsonFile(const std::string &filename);

public:
    // 默认构造函数, 不包含任何坐标系集合
    CoordinateManager() noexcept = default;

    // 禁用拷贝构造
    CoordinateManager(const CoordinateManager &rhs) = delete;
    CoordinateManager &operator=(const CoordinateManager &rhs) = delete;

    // 支持移动构造
    CoordinateManager(CoordinateManager &&rhs) noexcept = default;
    CoordinateManager &operator=(CoordinateManager &&rhs) noexcept = default;

public:
    void clear_global_offset() { global_offset_.fill(0.0); }
    void set_global_offset(const coord_offset_t &offset) {
        global_offset_ = offset;
    }
    const auto &get_global_offset() const { return global_offset_; }

    bool empty() const { return coordinates_map_.empty(); }
    auto size() const { return coordinates_map_.size(); }

    void clear_coordinates_map() { coordinates_map_.clear(); }
    const auto &get_coordinates_map() const { return coordinates_map_; }
    void set_coordinates_map(map_t &&coordinates_map) {
        coordinates_map_ = std::move(coordinates_map);
    }

    // 是否存在某一个坐标系
    bool exist_coordinate_index(uint32_t index) const {
        return coordinates_map_.find(index) != coordinates_map_.end();
    }

    // 获取存在的坐标系编号数组
    std::vector<uint32_t> get_avaiable_coord_indexes() const;

    // 获取某一个坐标系的偏置, 若不存在, 返回std::nullopt
    std::optional<coord_offset_t> get_coord_offset(uint32_t index) const;

    // 对`set_coord_offset` 输入当前该坐标系下的坐标,
    // 即可将当前点置为当前坐标系的零点
    bool set_coord_offset(uint32_t coord_index, const coord_offset_t &offset);

    //! G54(例)坐标系值 -> 机床坐标系值
    // 根据输入的坐标系编号, 以及坐标系下点的坐标值, 计算该点机床坐标值
    // 如果坐标系编号不存在, 返回false, 否则进行计算, 输出到output并返回true
    bool coord_to_machine(uint32_t coord_index,
                          const move::axis_t &curr_coord_axis_value,
                          move::axis_t &output) const;

    //! G54(例)坐标系值 -> 电机坐标值(实际点)
    // 根据输入的坐标系编号, 以及坐标系下点的坐标值,
    // 计算该点驱动器坐标值(计算了回零全局偏置) 如果坐标系编号不存在, 返回false,
    // 否则进行计算, 输出到output并返回true
    bool coord_to_motor(uint32_t coord_index,
                        const move::axis_t &curr_coord_axis_value,
                        move::axis_t &output) const;

    //! 电机坐标值(实际点) -> 机床坐标系值
    bool motor_to_machine(const move::axis_t &motor_axis,
                          move::axis_t &output) const;

    //! 电机坐标值(实际点) -> G54(例)坐标系值
    bool motor_to_coord(uint32_t coord_index, const move::axis_t &motor_axis,
                        move::axis_t &output) const;

    // TODO

public:
    // 保存到文件
    bool save_as(const std::string &filename) const;

private:
    bool _save_to_file(const std::string &filename) const;

private:
    coord_offset_t global_offset_{0.0};
    map_t coordinates_map_;
};

} // namespace coord

} // namespace edm
