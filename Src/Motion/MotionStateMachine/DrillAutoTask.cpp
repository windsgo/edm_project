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

bool DrillAutoTask::pause() {
    if (drill_state_ == DrillState::Idle) {
        return false;
    }

    pause_flag_ = true;
    return true;
}

bool DrillAutoTask::resume() {
    if (pause_flag_ == false) {
        return false;
    }

    pause_flag_ = false;

    switch (drill_state_) {
    // TODO
    // 根据不同的状态, 打开高频等
    }

    return false;
}

bool DrillAutoTask::stop(bool immediate) {
    (void)immediate;
    _clear_all_status();

    _all_pump_and_spindle_on(false);
    _mach_on(false);
    return true;
}

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

const char *DrillAutoTask::GetDrillStateStr(DrillAutoTask::DrillState s) {
    switch (s) {
#define XX_(__s)                         \
    case DrillAutoTask::DrillState::__s: \
        return #__s;
        XX_(Idle)

        XX_(PrepareTouch)
        XX_(PrepareTouchDelaying)
        XX_(Touching)
        XX_(TouchedBacking)

        XX_(PrepareDrill)
        XX_(PrepareDrillDelaying)
        XX_(Drilling)

        XX_(PrepareBack)
        XX_(PrepareBackDelaying)
        XX_(ReturnBacking)

#undef XX_
    default:
        return "Unknow";
    }
}

void DrillAutoTask::_drillstate_changeto(DrillAutoTask::DrillState new_state) {
    s_logger->trace("Drill State: {} -> {}",
                    GetDrillStateStr(drill_state_),
                    GetDrillStateStr(new_state));

    drill_state_ = new_state;
}

void DrillAutoTask::_clear_all_status() {
    _drillstate_changeto(DrillState::Idle);
    runtime_.clear();
    pause_flag_ = false;
}

} // namespace move

} // namespace edm
