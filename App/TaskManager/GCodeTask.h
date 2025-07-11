#pragma once

#include "GCodeTaskBase.h"

#include "Motion/MoveDefines.h"

// 定义不同GCode命令

#include <cstdint>
#include <memory>

#include <optional>

namespace edm {

namespace task {

class GCodeTaskG00Motion final : public GCodeTaskBase {
public:
    GCodeTaskG00Motion(bool touch_detect_enable, bool is_touch_motion,
                       int feed_speed, int coord_index,
                       GCodeCoordinateMode coord_mode,
                       const std::vector<std::optional<double>> cmd_values,
                       int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::G00MotionCommand, line_number,
                        node_index),
          touch_detect_enable_(touch_detect_enable),
          is_touch_motion_(is_touch_motion), feed_speed_(feed_speed),
          coord_index_(coord_index), coord_mode_(coord_mode),
          cmd_values_(cmd_values) {}
    ~GCodeTaskG00Motion() noexcept override = default;

    auto touch_detect_enable() const { return touch_detect_enable_; }
    auto is_touch_motion() const { return is_touch_motion_; }

    auto feed_speed() const { return feed_speed_; }
    auto coord_index() const { return coord_index_; }

    auto coord_mode() const { return coord_mode_; }

    const auto &cmd_values() const { return cmd_values_; }

    bool is_motion_task() const override { return true; }

private:
    bool touch_detect_enable_; // 接触感知使能 (m05忽略接触感知)
    bool is_touch_motion_;     // 是否是碰边动作 (touch=?)
    int feed_speed_;           // 在此之前设定的feed speed
    int coord_index_;          // 使用的坐标系序号
    GCodeCoordinateMode coord_mode_; // g90, g91
    std::vector<std::optional<double>>
        cmd_values_; // g00()后面带的坐标值, 没有的以std::nullopt记录 (单位mm,
                     // 无需转换)
};

class GCodeTaskG01Motion final : public GCodeTaskBase {
public:
    GCodeTaskG01Motion(int coord_index, GCodeCoordinateMode coord_mode,
                       const std::vector<std::optional<double>> cmd_values,
                       int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::G01MotionCommand, line_number,
                        node_index),
          coord_index_(coord_index), coord_mode_(coord_mode),
          cmd_values_(cmd_values) {}
    ~GCodeTaskG01Motion() noexcept override = default;

    auto coord_index() const { return coord_index_; }

    auto coord_mode() const { return coord_mode_; }

    const auto &cmd_values() const { return cmd_values_; }

    bool is_motion_task() const override { return true; }

private:
    int coord_index_;                // 使用的坐标系序号
    GCodeCoordinateMode coord_mode_; // g90, g91
    std::vector<std::optional<double>>
        cmd_values_; // g00()后面带的坐标值, 没有的以std::nullopt记录
};

class GCodeTaskCoordinateMode final : public GCodeTaskBase {
public:
    GCodeTaskCoordinateMode(int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::CoordinateModeCommand, line_number,
                        node_index) {}
    ~GCodeTaskCoordinateMode() noexcept override = default;

    bool is_motion_task() const override { return false; }
};

class GCodeTaskCoordinateIndex final : public GCodeTaskBase {
public:
    GCodeTaskCoordinateIndex(int coord_index, int line_number,
                             int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::CoordinateIndexCommand, line_number,
                        node_index),
          coord_index_(coord_index) {}
    ~GCodeTaskCoordinateIndex() noexcept override = default;

    auto coord_index() const { return coord_index_; }

    bool is_motion_task() const override { return false; }

private:
    int coord_index_; // 切换坐标系序号
};

class GCodeTaskEleparamSet final : public GCodeTaskBase {
public:
    GCodeTaskEleparamSet(int eleparam_index, int line_number,
                         int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::EleparamSetCommand, line_number,
                        node_index),
          eleparam_index_(eleparam_index) {}
    ~GCodeTaskEleparamSet() noexcept override = default;

    auto eleparam_index() const { return eleparam_index_; }

    bool is_motion_task() const override { return false; }

private:
    int eleparam_index_; // 要设定的电参数序号
};

class GCodeTaskFeedSpeedSet final : public GCodeTaskBase {
public:
    GCodeTaskFeedSpeedSet(int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::FeedSpeedSetCommand, line_number,
                        node_index) {}
    ~GCodeTaskFeedSpeedSet() noexcept override = default;

    bool is_motion_task() const override { return false; }
};

class GCodeTaskDeley final : public GCodeTaskBase {
public:
    GCodeTaskDeley(double delay_s, int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::DelayCommand, line_number, node_index),
          delay_s_(delay_s) {}
    ~GCodeTaskDeley() noexcept override = default;

    auto delay_s() const { return delay_s_; }

    bool is_motion_task() const override { return true; }

private:
    double delay_s_; // 要延时的秒数 (支持小数)
};

class GCodeTaskProgramEnd final : public GCodeTaskBase {
public:
    GCodeTaskProgramEnd(int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::ProgramEndCommand, line_number,
                        node_index) {}
    ~GCodeTaskProgramEnd() noexcept override = default;

    bool is_motion_task() const override { return false; }
};

class GCodeTaskPauseCommand final : public GCodeTaskBase {
public:
    GCodeTaskPauseCommand(int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::PauseCommand, line_number, node_index) {}
    ~GCodeTaskPauseCommand() noexcept override = default;

    bool is_motion_task() const override { return true; }
};

class GCodeTaskCoordSetZeroCommand final : public GCodeTaskBase {
public:
    GCodeTaskCoordSetZeroCommand(const std::vector<bool> &set_zero_axis_list,
                                 int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::CoordSetZeroCommand, line_number,
                        node_index),
          set_zero_axis_list_(set_zero_axis_list) {}
    ~GCodeTaskCoordSetZeroCommand() noexcept override = default;

    const auto &set_zero_axis_list() const { return set_zero_axis_list_; }

    bool is_motion_task() const override { return false; }

private:
    std::vector<bool> set_zero_axis_list_;
};

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
class GCodeTaskDrillMotion final : public GCodeTaskBase {
public:
    GCodeTaskDrillMotion(const move::DrillStartParams &start_params,
                         int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::DrillMotionCommand, line_number,
                        node_index),
          start_params_(start_params) {}
    ~GCodeTaskDrillMotion() noexcept override = default;

    const auto &start_params() const { return start_params_; }

    bool is_motion_task() const override { return true; }

private:
    move::DrillStartParams start_params_;
};
#endif

class GCodeTaskG01GroupMotion final : public GCodeTaskBase {
public:
    struct G01GroupPoint {
        std::vector<std::optional<double>> cmd_values;
        GCodeCoordinateMode coord_mode{GCodeCoordinateMode::Undefined};
        int line_number{-1};
    };

public:
    GCodeTaskG01GroupMotion(int coord_index,
                            const std::vector<G01GroupPoint> &points,
                            int line_number, int node_index = -1)
        : GCodeTaskBase(GCodeTaskType::G01GroupMotionCommand, line_number,
                        node_index),
          coord_index_(coord_index), points_(points) {}
    ~GCodeTaskG01GroupMotion() noexcept override = default;

    bool is_motion_task() const override { return true; }

    const auto &points() const { return points_; }

    auto coord_index() const { return coord_index_; }

private:
    int coord_index_;
    std::vector<G01GroupPoint> points_;
};

} // namespace task

} // namespace edm
