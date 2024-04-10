#pragma once

#include "Motion/MoveDefines.h"

#include "SystemSettings/SystemSettings.h"

#include "config.h"

namespace edm {

namespace util {

class UnitConverter final {
private:
    // length unit factors
    constexpr static const double um2blu_{EDM_BLU_PER_UM};
    constexpr static const double blu2um_{1.0 / (double)EDM_BLU_PER_UM};
    constexpr static const double um2mm_{1.0 / 1000.0};
    constexpr static const double mm2um_{1000.0};
    constexpr static const double mm2blu_{mm2um_ * um2blu_};
    constexpr static const double blu2mm_{blu2um_ * um2mm_};

    // time unit factors
    constexpr static const double s2min_{1.0 / 60.0};
    constexpr static const double min2s_{60.0};
    constexpr static const double s2ms_{1000.0};
    constexpr static const double ms2us_{1000.0};
    static const uint32_t MOTION_CYCLE_US;
    static const double p2ms_; // {MOTION_CYCLE_US / 1000.0}; // 周期 -> ms
    static const double ms2p_; //{1.0 / p2ms_};              // ms -> 周期

    // speed unit factors
    constexpr static const double mm_min2um_s_{1000.0 / 60.0}; // mm/min -> um/s
    constexpr static const double mm_min2blu_s_{1000.0 / 60.0 *
                                                um2blu_}; // mm/min -> blu/s

public:
    static inline move::unit_t um2blu(move::unit_t um) { return um * um2blu_; }
    static inline move::unit_t mm2blu(move::unit_t mm) { return mm * mm2blu_; }
    static inline move::unit_t blu2um(move::unit_t blu) {
        return blu * blu2um_;
    }
    static inline move::unit_t blu2mm(move::unit_t blu) {
        return blu * blu2mm_;
    }

    static inline move::unit_t ms2p(move::unit_t ms) { return ms * ms2p_; }

    static inline move::unit_t mm_min2blu_s(move::unit_t mm_min) {
        return mm_min * mm_min2blu_s_;
    }

    static inline move::unit_t um_ms2blu_p(move::unit_t um_ms) {
        return um_ms * um2blu_ / ms2p_;
    }

    // TODO
};

} // namespace util

} // namespace edm
