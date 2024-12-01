#include "DrillAutoTask.h"

#include "Logger/LogMacro.h"
#include "Motion/MotionSharedData/MotionSharedData.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include <chrono>

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

static auto s_motion_shared = edm::move::MotionSharedData::instance();

namespace edm {

namespace move {

DrillAutoTask::DrillAutoTask(const StartParams &start_params,
                             const MotionCallbacks &cbs)
    : cbs_(cbs), AutoTask(AutoTaskType::Drill) {}

void DrillAutoTask::_all_pump_and_spindle_on(bool on) {
    cbs_.cb_opump_on(on);
    cbs_.cb_ipump_on(on);
    if (on) {
        s_motion_shared->get_spindle_controller()->start_spindle();
    } else {
        s_motion_shared->get_spindle_controller()->stop_spindle();
    }
}

void DrillAutoTask::_mach_on(bool on) { cbs_.cb_mach_on(on); }

} // namespace move

} // namespace edm
