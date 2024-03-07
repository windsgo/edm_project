#pragma once

#include <array>
#include <memory>

#include "config.h"

#include <type_traits>

#include "Motion/MoveDefines.h"

namespace edm {

namespace coord {

using coord_offset_t = move::axis_t;

/**
 * 坐标系偏置 := 坐标系零点，在机床坐标系下的位置
 * 回零偏执(全局偏置) := 机床零点，在驱动器坐标系下的位置
 * 
 * motor_pos(驱动器坐标) = coordn_pos(坐标系n下的坐标) + coordn_offset(坐标系n的偏置) + global_offset(回零偏置)
 *                     = machine_pos + global_offset
 * machine_pos(机床坐标) = coordn_pos + coordn_offset
 */

class Coordinate {
public:
    Coordinate() noexcept = default;
    ~Coordinate() noexcept = default;

    inline auto index() const { return index_; }
    inline const auto &offset() const { return offset_; }

    // 对`set_offset` 输入当前该坐标系下的坐标, 即可将当前点置为当前坐标系的零点 
    inline void set_offset(const coord_offset_t &offset) { offset_ = offset; }
    template <typename... Args> inline void set_offset(Args &&...args) {
        offset_ = std::move(coord_offset_t(std::forward<Args>(args)...));
    }

    // `clear_offset` 将当前坐标系的偏置清零
    // 效果: 当前坐标系与机床坐标系零点重合, 坐标系坐标 = 机床坐标
    inline void clear_offset() { offset_.fill(0.0); }

    // `get_machine_pos` 根据输入的当前坐标系下点的坐标值, 计算该点机床坐标值
    inline move::axis_t get_machine_pos(const move::axis_t& curr_coord_axis_value) {
        move::axis_t ret;
        for (int i = 0; i < curr_coord_axis_value.size(); ++i) {
            // 机床坐标 = 坐标系下坐标值 + 坐标系零点相对机床零点的位置(坐标系偏置)
            ret[i] = curr_coord_axis_value[i] + offset_[i];
        }

        return ret; // NRVO
    }

private:
    uint32_t index_;
    coord_offset_t offset_;
};

} // namespace coord

} // namespace edm
