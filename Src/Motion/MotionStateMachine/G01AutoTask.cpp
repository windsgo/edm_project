#include "G01AutoTask.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {
bool G01AutoTask::pause() {
    switch (state_) {
    case State::Stopping:
    case State::Stopped:
    case State::Resuming:
    default:
        return false;
    case State::Pausing:
    case State::Paused:
        return true;
    case State::NormalRunning: {
        _state_changeto(State::Pausing);
        _pauseorstop_substate_changeto(PauseOrStopSubState::WaitingForJumpEnd);
        return true;
    }
    }
}

bool G01AutoTask::resume() {
    switch (state_) {
    case State::Stopping:
    case State::Stopped:
    case State::Pausing:
    case State::NormalRunning:
    default:
        return false;
    case State::Resuming:
        return true;
    case State::Paused: {
        _state_changeto(State::Resuming);
        _resume_substate_changeto(ResumeSubState::RecoveringToLastMachingPos);
        return true;
    }
    }
}

bool G01AutoTask::stop(bool immediate [[maybe_unused]]) {
    // TODO 不处理 immediate

    switch (state_) {
    case State::Pausing: {
        _state_changeto(State::Stopping); // 沿用Pausing时Substate做的事情
        return true;
    }
    case State::Paused: {
        _state_changeto(State::Stopped);
        return true;
    }
    case State::Stopping:
    case State::Stopped:
        return true;
    case State::Resuming: {
        s_logger->warn("G01 Stopped When Resuming");
        // TODO 如果支持了暂停回原点, 这个地方要考虑速度大的时候Stop会不会振动
        _state_changeto(State::Stopped);
        return true;
    }
    case State::NormalRunning: {
        _state_changeto(State::Stopping);
        _pauseorstop_substate_changeto(PauseOrStopSubState::WaitingForJumpEnd);
        return true;
    }
    default:
        return false;
    }
}

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
    s_logger->trace("G01 ServoSubState: {} -> {}",
                    GetServoSubStateStr(servo_sub_state_),
                    GetServoSubStateStr(new_s));
    servo_sub_state_ = new_s;
}

void G01AutoTask::_pauseorstop_substate_changeto(PauseOrStopSubState new_s) {
    s_logger->trace("G01 PauseOrStopSubState: {} -> {}",
                    GetPauseOrStopSubStateStr(pause_or_stop_sub_state_),
                    GetPauseOrStopSubStateStr(new_s));
    pause_or_stop_sub_state_ = new_s;
}

void G01AutoTask::_resume_substate_changeto(ResumeSubState new_s) {
    s_logger->trace("G01 ResumeSubState: {} -> {}",
                    GetResumeSubStateStr(resume_sub_state_),
                    GetResumeSubStateStr(new_s));
    resume_sub_state_ = new_s;
}

void G01AutoTask::_state_normal_running() {
    switch (servo_sub_state_) {
    case ServoSubState::Servoing: {
        _servo_substate_servoing();
        break;
    }
    case ServoSubState::JumpUping:
        _servo_substate_jumpuping();
        break;
    case ServoSubState::JumpDowning:
        _servo_substate_jumpdowning();
        break;
    case ServoSubState::JumpDowningBuffer:
        _servo_substate_jumpdowningbuffer();
        break;
    default:
        break;
    }
}

void G01AutoTask::_state_pausing() {
    _state_pausing_or_stopping(State::Paused);
}

void G01AutoTask::_state_paused() {
    // Do Nothing
}

void G01AutoTask::_state_resuming() {
    switch (resume_sub_state_) {
    case ResumeSubState::RecoveringToLastMachingPos:
        if (!back_to_begin_when_pause_) {
            _state_changeto(State::NormalRunning);
            assert(servo_sub_state_ == ServoSubState::Servoing);
            break;
        }

        // TODO Back To Begin Related Resume
        // Currently direct change to paused, do nothing
        assert(false); // should not be here
        _state_changeto(State::NormalRunning);
        break;
    default:
        break;
    }
}

void G01AutoTask::_state_stopping() {
    _state_pausing_or_stopping(State::Stopped);
}

void G01AutoTask::_state_stopped() {
    // Do Nothing
}

void G01AutoTask::_state_pausing_or_stopping(State target_state) {
    assert(target_state == State::Stopped || target_state == State::Paused);

    switch (pause_or_stop_sub_state_) {
    case PauseOrStopSubState::WaitingForJumpEnd: {
        // Check Servo Sub State, Wait for jump end (servo sub state return to
        // Servoing)
        switch (servo_sub_state_) {
        case ServoSubState::Servoing: {
            // if is servoing, then go to next pause and stop state
            _pauseorstop_substate_changeto(PauseOrStopSubState::BackingToBegin);
            break;
        }
        case ServoSubState::JumpUping:
            _servo_substate_jumpuping();
            break;
        case ServoSubState::JumpDowning:
            _servo_substate_jumpdowning();
            break;
        case ServoSubState::JumpDowningBuffer:
            _servo_substate_jumpdowningbuffer();
            break;
        }

        break;
    }
    case PauseOrStopSubState::BackingToBegin: {
        assert(servo_sub_state_ == ServoSubState::Servoing);
        if (target_state == State::Stopped && !back_to_begin_when_stop_) {
            _state_changeto(State::Stopped);
            break;
        }
        if (target_state == State::Paused && !back_to_begin_when_pause_) {
            _state_changeto(State::Paused);
            break;
        }

        // TODO Back To Begin
        // Currently direct change to paused/stopped, do nothing
        assert(false); // should not be here
        _state_changeto(target_state);
        break;
    }
    default:
        break;
    }
}

void G01AutoTask::_servo_substate_servoing() {}

void G01AutoTask::_servo_substate_jumpuping() {}

void G01AutoTask::_servo_substate_jumpdowning() {}

void G01AutoTask::_servo_substate_jumpdowningbuffer() {}

} // namespace move

} // namespace edm