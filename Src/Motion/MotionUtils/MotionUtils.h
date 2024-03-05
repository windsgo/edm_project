#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "Motion/MoveDefines.h"

namespace edm {

namespace move {

class MotionUtils final {
private:
    MotionUtils() noexcept = default;
    ~MotionUtils() noexcept = default;

public:
    static inline void ClearAxis(axis_t &axis) { axis.fill(0.0); }

    // 计算一个向量的长度
    static inline unit_t CalcAxisLength(const axis_t &axis) {
        unit_t sum = 0.0;
        for (const auto &axis_i : axis) {
            sum += axis_i * axis_i;
        }

        return std::sqrt(sum);
    }

    // 计算长度: 起点到终点
    static inline unit_t CalcAxisLength(const axis_t &begin_axis,
                                        const axis_t &end_axis) {
        unit_t sum = 0.0;
        for (int i = 0; i < begin_axis.size(); ++i) {
            auto temp = (end_axis[i] - begin_axis[i]);
            sum += temp * temp;
        }

        return std::sqrt(sum);
    }

    static inline void CalcAxisVector(const axis_t &begin_axis,
                                      const axis_t &end_axis, axis_t &vector) {
        for (int i = 0; i < begin_axis.size(); ++i) {
            vector[i] = end_axis[i] - begin_axis[i];
        }
    }

    static inline axis_t CalcAxisVector(const axis_t &begin_axis,
                                        const axis_t &end_axis) {
        axis_t ret;
        MotionUtils::CalcAxisVector(begin_axis, end_axis, ret);
        return ret;
    }

    static inline void CalcAxisUnitVector(const axis_t &begin_axis,
                                          const axis_t &end_axis,
                                          axis_t &unit_vec) {
        for (int i = 0; i < begin_axis.size(); ++i) {
            unit_vec[i] = end_axis[i] - begin_axis[i];
        }

        // dont use reference function type
        // incase the compiler optimize sth ...
        unit_vec = ScaleAxis(unit_vec, (double)1 / CalcAxisLength(unit_vec));
    }

    static inline axis_t CalcAxisUnitVector(const axis_t &begin_axis,
                                            const axis_t &end_axis) {
        axis_t ret;
        CalcAxisUnitVector(begin_axis, end_axis, ret);
        return ret;
    }

    // 坐标数乘
    static inline void ScaleAxis(const axis_t &axis, unit_t scale,
                                 axis_t &output) {
        for (int i = 0; i < axis.size(); ++i) {
            output[i] = axis[i] * scale;
        }
    }

    static inline axis_t ScaleAxis(const axis_t &axis, unit_t scale) {
        axis_t ret;
        MotionUtils::ScaleAxis(axis, scale, ret);
        return ret;
    }

    static inline bool IsAxisTheSame(const axis_t& axis1, const axis_t& axis2) {
        return axis1 == axis2;
    }
};

} // namespace move

} // namespace edm