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

    static inline unit_t CalcAxisLength(const axis_t &axis) {
        unit_t sum = 0.0;
        for (const auto &axis_i : axis) {
            sum += axis_i * axis_i;
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
};

} // namespace move

} // namespace edm