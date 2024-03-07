#pragma once

#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"

#include <memory>

// 用于放电加工, 抬刀, 对路径段的抽象描述

namespace edm {

namespace move {

enum class TrajectorySegementType { Unknow, Linear, Nurbs };

// 路径段基类, 维护路径段类型, 路径段的起点和终点坐标
class TrajectorySegementBase {
public:
    using ptr = std::shared_ptr<TrajectorySegementBase>;
    // new segement ctor, set curr_pos to start_pos
    TrajectorySegementBase(TrajectorySegementType type, const axis_t &start_pos,
                           const axis_t &end_pos)
        : type_(type), start_pos_(start_pos), end_pos_(end_pos),
          curr_pos_{start_pos} {}

    virtual ~TrajectorySegementBase() noexcept = default;

    inline auto type() const { return type_; }
    inline const auto &start_pos() const { return start_pos_; }
    inline const auto &end_pos() const { return end_pos_; }
    inline const auto &curr_pos() const { return curr_pos_; }

    inline virtual void set_at_start() = 0;
    inline virtual void set_at_end() = 0;

    inline virtual bool at_start() const = 0;
    inline virtual bool at_end() const = 0;

    inline virtual void run_once(unit_t inc) = 0;

private:
    TrajectorySegementType type_;

protected:
    axis_t start_pos_;
    axis_t end_pos_;

    axis_t curr_pos_;
};

//! 先只实现线性路径段, 圆弧路径段感觉用不到, Nurbs不存在路径段, 是一整个
//! Trajectory
// 线性路径段
class TrajectoryLinearSegement : public TrajectorySegementBase {
public:
    // new segement ctor, set curr_pos to start_pos
    TrajectoryLinearSegement(const axis_t &start_pos, const axis_t &end_pos)
        : TrajectorySegementBase(TrajectorySegementType::Linear, start_pos,
                                 end_pos),
          total_length_(MotionUtils::CalcAxisLength(start_pos, end_pos)),
          curr_length_(0.0) {}

    // custom set curr_pos to user-set value (using curr_length)
    TrajectoryLinearSegement(const axis_t &start_pos, const axis_t &end_pos,
                             unit_t curr_length)
        : TrajectorySegementBase(TrajectorySegementType::Linear, start_pos,
                                 end_pos),
          total_length_(MotionUtils::CalcAxisLength(start_pos, end_pos)),
          curr_length_(curr_length) {
        _validate_curr_length();
        curr_pos_ = CalcCurrPos(start_pos, end_pos, curr_length_);
    }

    ~TrajectoryLinearSegement() noexcept override = default;

    void set_at_start() override;
    void set_at_end() override;

    bool at_start() const override;
    bool at_end() const override;
    void run_once(unit_t inc) override;

public:
    inline const unit_t curr_length() const { return curr_length_; }
    inline const unit_t total_length() const { return total_length_; }

private:
    void _add_inc(unit_t inc);
    void _validate_curr_length();

public:
    static axis_t CalcCurrPos(const axis_t &start_pos, const axis_t &end_pos,
                              unit_t curr_length);

private:
    unit_t total_length_;
    unit_t curr_length_;
};

//
class TrajectoryNurbsSegement {};

} // namespace move

} // namespace edm
