#include "UnitConverter.h"

namespace edm {

namespace util {

const uint32_t UnitConverter::MOTION_CYCLE_US{
    SystemSettings::instance().get_motion_cycle_us()};

const double UnitConverter::p2ms_{UnitConverter::MOTION_CYCLE_US / 1000.0}; // 周期 -> ms
const double UnitConverter::ms2p_{1.0 / UnitConverter::p2ms_};              // ms -> 周期

} // namespace util

} // namespace edm
