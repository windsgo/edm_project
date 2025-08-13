#include "G01GroupAutoTask.h"
#include "Motion/MotionSharedData/MotionSharedData.h"
#include "Motion/MotionStateMachine/MotionAutoTask.h"

#include "Logger/LogMacro.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/Trajectory/TrajectoryList.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include <cassert>
EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

G01GroupAutoTask::G01GroupAutoTask(const G01GroupStartParam &start_param,
                                   const MotionCallbacks &cbs)
    : AutoTask(AutoTaskType::G01LineGroup), cbs_(cbs),
      start_param_(start_param) {

    TrajectoryList::container_type segements;
    
    axis_t first_pos = s_motion_shared->get_global_cmd_axis();

    const auto &items = start_param_.items;
    for (int i = 0; i < items.size(); ++i) {
        const auto &item = items[i];

        auto end_pos = first_pos;
        for (int axis = 0; axis < end_pos.size(); ++axis) {
            end_pos[axis] += item.incs[axis];
        }

        segements.push_back(
            std::make_shared<TrajectoryLinearSegement>(first_pos, end_pos));
        
        first_pos = end_pos;
    }

    if (segements.empty()) {
        s_logger->error("G01GroupAutoTask: no segements");
        _state_changeto(State::Stopped);
        return;
    }

    traj_list_ = std::make_shared<TrajectoryList>(std::move(segements));

    _state_changeto(State::NormalRunning);
    _mach_on(true);
}

const char *G01GroupAutoTask::GetStateStr(G01GroupAutoTask::State s) {
    switch (s) {
#define XX_(s__)     \
    case State::s__: \
        return #s__;
        XX_(NormalRunning)
        XX_(Pausing)
        XX_(Paused)
        XX_(Resuming)
        XX_(Stopping)
        XX_(Stopped)
#undef XX_
    default:
        return "Unknow";
    }
}

void G01GroupAutoTask::_state_changeto(G01GroupAutoTask::State new_s) {
    s_logger->trace("G01 State: {} -> {}", GetStateStr(state_),
                    GetStateStr(new_s));
    state_ = new_s;
}

void G01GroupAutoTask::_mach_on(bool enable) {
    return; // test

    cbs_.cb_mach_on(enable);
    cbs_.cb_enable_voltage_gate(enable);

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    const auto &dp = s_motion_shared->get_drill_params();
    if (dp.auto_switch_opump) {
        cbs_.cb_opump_on(enable);
    }
    if (dp.auto_switch_ipump) {
        cbs_.cb_ipump_on(enable);
    }

    if (enable) {
        s_motion_shared->get_spindle_controller()->start_spindle();
    } else {
        s_motion_shared->get_spindle_controller()->stop_spindle();
    }
#endif
}

bool G01GroupAutoTask::pause() {
    switch (state_) {
    case State::Resuming:
    case State::NormalRunning:
        _state_changeto(State::Pausing);
        return true;
    case State::Pausing:
        // Nothing to do
        return true;

    default:
    case State::Paused:
    case State::Stopped:
    case State::Stopping:
        return false;
    }
}
bool G01GroupAutoTask::resume() {
    switch (state_) {
    case State::Paused:
        _state_changeto(State::Resuming);
        return true;
    case State::Resuming:
        // Nothing to do
        return true;

    default:
    case State::NormalRunning:
    case State::Pausing:
    case State::Stopped:
    case State::Stopping:
        return false;
    }
}

bool G01GroupAutoTask::stop(bool immediate) {
    (void)immediate;

    switch (state_) {
    case State::NormalRunning:
    case State::Resuming:
    case State::Pausing:
    case State::Paused:
        _state_changeto(State::Stopping);
        return true;
    case State::Stopping:
        // Nothing to do
        return true;

    default:
    case State::Stopped:
        return false;
    }
}

bool G01GroupAutoTask::is_normal_running() const {
    return state_ == State::NormalRunning;
}
bool G01GroupAutoTask::is_pausing() const { return state_ == State::Pausing; }
bool G01GroupAutoTask::is_paused() const { return state_ == State::Paused; }
bool G01GroupAutoTask::is_resuming() const { return state_ == State::Resuming; }
bool G01GroupAutoTask::is_stopping() const { return state_ == State::Stopping; }
bool G01GroupAutoTask::is_stopped() const { return state_ == State::Stopped; }
bool G01GroupAutoTask::is_over() const { return is_stopped(); }

void G01GroupAutoTask::_state_normal_running() {
    auto index = traj_list_->curr_segement_index();
    assert(index >= 0 && index < start_param_.items.size());

    s_motion_shared->set_sub_line_num(start_param_.items[index].line);

    double _servo_dis = util::UnitConverter::mm_min2blu_p(
        s_motion_shared->cached_udp_message().servo_calced_speed_mm_min);

    // test
    _servo_dis = util::UnitConverter::mm_min2blu_p(start_param_.items.at(index).feedrate);
    _servo_dis *= s_motion_shared->get_g01_speed_ratio();

    traj_list_->run_once(_servo_dis);

    s_motion_shared->set_global_cmd_axis(traj_list_->get_curr_cmd_axis());

    if (traj_list_->at_end()) {
        _state_changeto(State::Stopped);
        _mach_on(false);
        return;
    }
}

void G01GroupAutoTask::_state_pausing() {
    _mach_on(false);
    _state_changeto(State::Paused);
}

void G01GroupAutoTask::_state_paused() {
    // Nothing to do
}

void G01GroupAutoTask::_state_resuming() {
    _mach_on(true);
    _state_changeto(State::NormalRunning);
}

void G01GroupAutoTask::_state_stopping() {
    _mach_on(false);
    _state_changeto(State::Stopped);
}

void G01GroupAutoTask::_state_stopped() {
    // Nothing to do
}

void G01GroupAutoTask::run_once() {
    switch (state_) {
    case State::NormalRunning:
        _state_normal_running();
        break;
    case State::Pausing:
        _state_pausing();
        break;
    case State::Paused:
        _state_paused();
        break;
    case State::Resuming:
        _state_resuming();
        break;
    case State::Stopping:
        _state_stopping();
        break;
    case State::Stopped:
        _state_stopped();
        break;
    }
}

} // namespace move

} // namespace edm