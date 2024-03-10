#pragma once

#include "CoordinateManager.h"

#include <memory>

namespace edm
{

namespace coord
{

// 该类用于管理与实时坐标相关的所有坐标系事务, 包括
// 1. 指定一个当前使用的坐标系, 维护机床坐标系、当前使用的坐标系的缓存, 用于直接提供给外层显示
// 2. 接受外部传入的最新motor坐标值, 并刷新当前机床坐标系值, 和正在使用的坐标系的坐标值 (只缓存在使用的)
// 3. 维护软限位, 接受外部指定的各轴软限位(机床坐标系下的限位坐标值), 保存, 并可以提供对应的motor坐标限位值
// 4. 机床坐标系、以及各G坐标系的偏置设置, 并在此类中触发CoordinateManager的保存功能, 维护一个文件名用于保存
// 5. 坐标系index, 坐标轴数目保护(并log报警), 防止外部输入错误的值
class CoordinateSystem final {
public:
    using ptr = std::shared_ptr<CoordinateSystem>;
    CoordinateSystem(const std::string& filename);
    ~CoordinateSystem() noexcept = default;

    void update_motor_pos(const move::axis_t& new_motor_pos);

    auto get_avaiable_coord_indexes() const { return cm_.get_avaiable_coord_indexes(); }
    bool set_coord_index(uint32_t new_coord_index);
    auto get_current_coord_index() const { return curr_coord_index_; }

    const auto& get_current_coord_axis() const { return curr_coord_axis_cache_; }
    const auto& get_current_machine_axis() const { return curr_machine_axis_cache_; }
    const auto& get_current_motor_axis() const { return curr_motor_axis_; }

    bool get_coord_axis(uint32_t coord_index, move::axis_t& output) const;

    const auto& get_cm() const { return cm_; }

private:
    void _update_machine_axis_cache();
    void _update_coord_axis_cache();

private:
    bool _init_coordinate_manager(const std::string& filename);
    void _init_curr_index();

private:
    CoordinateManager cm_;
    std::string filename_;

private:
    move::axis_t curr_motor_axis_ {0.0};
    move::axis_t curr_machine_axis_cache_ {0.0};
    
    uint32_t curr_coord_index_;
    move::axis_t curr_coord_axis_cache_ {0.0};
};

} // namespace coord
    
} // namespace edm
