#pragma once

#include "CoordinateManager.h"

#include <memory>

namespace edm {

namespace coord {

// 该类用于管理与实时坐标相关的所有坐标系事务, 包括
// 1. 指定一个当前使用的坐标系, 维护机床坐标系、当前使用的坐标系的缓存,
// 用于直接提供给外层显示
// 2. 接受外部传入的最新motor坐标值, 并刷新当前机床坐标系值,
// 和正在使用的坐标系的坐标值 (只缓存在使用的)
// 3. 维护软限位, 接受外部指定的各轴软限位(机床坐标系下的限位坐标值), 保存,
// 并可以提供对应的motor坐标限位值
// 4. 机床坐标系、以及各G坐标系的偏置设置,
// 并在此类中触发CoordinateManager的保存功能, 维护一个文件名用于保存
// 5. 坐标系index, 坐标轴数目保护(并log报警), 防止外部输入错误的值
class CoordinateSystem final {
public:
    using ptr = std::shared_ptr<CoordinateSystem>;
    CoordinateSystem(const std::string &filename);
    ~CoordinateSystem() noexcept = default;

    void update_motor_pos(const move::axis_t &new_motor_pos,
                          const move::axis_t &new_motor_pos_act);

    auto get_avaiable_coord_indexes() const {
        return cm_.get_avaiable_coord_indexes();
    }
    bool set_coord_index(uint32_t new_coord_index);
    auto get_current_coord_index() const { return curr_coord_index_; }

    const auto &get_current_coord_axis() const {
        return curr_coord_axis_cache_;
    }
    const auto &get_current_coord_axis_act() const {
        return curr_coord_axis_cache_act_;
    }
    const auto &get_current_machine_axis() const {
        return curr_machine_axis_cache_;
    }
    const auto &get_current_machine_axis_act() const {
        return curr_machine_axis_cache_act_;
    }
    const auto &get_current_motor_axis() const { return curr_motor_axis_; }

    auto get_current_coord_offset() const {
        return cm_.get_coord_offset(curr_coord_index_);
    }
    const auto &get_current_global_offset() const {
        return cm_.get_global_offset();
    }

    bool get_coord_axis(uint32_t coord_index, move::axis_t &output) const;

    const auto &get_cm() const { return cm_; }

    // TODO 修改坐标系偏置, 并保存

    // 设置当前坐标系 偏置
    bool set_current_coord_offset(const move::axis_t& offset);
    bool set_coord_offset(uint32_t index, const move::axis_t& offset);

    void set_global_offset(const move::axis_t& offset);

private:
    void _update_machine_axis_cache();
    void _update_coord_axis_cache();

private:
    bool _init_coordinate_manager(const std::string &filename);
    void _init_curr_index();

private:
    CoordinateManager cm_;
    std::string filename_;

private:
    move::axis_t curr_motor_axis_{0.0};
    move::axis_t curr_motor_axis_act_{0.0}; // 实际坐标

    move::axis_t curr_machine_axis_cache_{0.0};
    move::axis_t curr_machine_axis_cache_act_{0.0}; // 实际坐标

    uint32_t curr_coord_index_;
    move::axis_t curr_coord_axis_cache_{0.0};
    move::axis_t curr_coord_axis_cache_act_{0.0}; // 实际坐标
};

} // namespace coord

} // namespace edm
