#include "G01AutoTask.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {
bool G01AutoTask::pause() { return false; }

bool G01AutoTask::resume() { return false; }

bool G01AutoTask::stop(bool immediate) { return false; }

bool G01AutoTask::is_normal_running() const {
    return state_ == State::NormalRunning;
}

bool G01AutoTask::is_pausing() const { return state_ == State::Pausing; }

bool G01AutoTask::is_paused() const { return state_ == State::Paused; }

bool G01AutoTask::is_resuming() const { return state_ == State::Resuming; }

bool G01AutoTask::is_stopping() const { return state_ == State::Stopping; }

bool G01AutoTask::is_stopped() const { return state_ == State::Stopped; }

bool G01AutoTask::is_over() const { return is_stopped(); }

void G01AutoTask::run_once() {
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
    default:
        break;
    }
}

const char *G01AutoTask::GetStateStr(State s) {
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

const char *G01AutoTask::GetServoSubStateStr(ServoSubState s) {
    switch (s) {
#define XX_(s__)             \
    case ServoSubState::s__: \
        return #s__;
        XX_(Servoing)
        XX_(JumpUping)
        XX_(JumpDowning)
        XX_(JumpDowningBuffer)
#undef XX_
    default:
        return "Unknow";
    }
}

const char *G01AutoTask::GetPauseOrStopSubStateStr(PauseOrStopSubState s) {
    switch (s) {
#define XX_(s__)                   \
    case PauseOrStopSubState::s__: \
        return #s__;
        XX_(WaitingForJumpEnd)
        XX_(BackingToBegin)
#undef XX_
    default:
        return "Unknow";
    }
}

const char *G01AutoTask::GetResumeSubStateStr(ResumeSubState s) {
    switch (s) {
#define XX_(s__)              \
    case ResumeSubState::s__: \
        return #s__;
        XX_(RecoveringToLastMachingPos)
#undef XX_
    default:
        return "Unknow";
    }
}

void G01AutoTask::_state_changeto(State new_s) {
    s_logger->trace("G01 State: {} -> {}", GetStateStr(state_),
                    GetStateStr(new_s));
    state_ = new_s;
}

void G01AutoTask::_servo_substate_changeto(ServoSubState new_s) {
    s_logger->trace("G01 State: {} -> {}",
                    GetServoSubStateStr(servo_sub_state_),
                    GetServoSubStateStr(new_s));
    servo_sub_state_ = new_s;
}

void G01AutoTask::_pauseorstop_substate_changeto(PauseOrStopSubState new_s) {
    s_logger->trace("G01 State: {} -> {}",
                    GetPauseOrStopSubStateStr(pause_or_stop_sub_state_),
                    GetPauseOrStopSubStateStr(new_s));
    pause_or_stop_sub_state_ = new_s;
}

void G01AutoTask::_resume_substate_changeto(ResumeSubState new_s) {
    s_logger->trace("G01 State: {} -> {}",
                    GetResumeSubStateStr(resume_sub_state_),
                    GetResumeSubStateStr(new_s));
    resume_sub_state_ = new_s;
}

} // namespace move

} // namespace edm