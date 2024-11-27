#include "UnitConverter.h"

namespace edm {

namespace util {

const uint32_t UnitConverter::MOTION_CYCLE_US{
    SystemSettings::instance().get_motion_cycle_us()};

const double UnitConverter::p2ms_{UnitConverter::MOTION_CYCLE_US /
                                  1000.0};                     // 周期 -> ms
const double UnitConverter::ms2p_{1.0 / UnitConverter::p2ms_}; // ms -> 周期

const double UnitConverter::mm_min2blu_p_{
    1000.0 / 60.0 * UnitConverter::um2blu_ / UnitConverter::s2ms_ /
    UnitConverter::ms2p_};

const double UnitConverter::blu_p2mm_min_{1.0/UnitConverter::mm_min2blu_p_};

} // namespace util

} // namespace edm
