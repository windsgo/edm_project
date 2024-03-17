#include "GCodeRunner.h"

namespace edm
{

namespace task
{

bool GCodeRunner::pause() {
    switch (state_) {
    case State::Running: {
        switch (gcode_list_[curr_gcode_num_]->type())
        {
        case GCodeTaskType::CoordinateIndexCommand:
        case GCodeTaskType::CoordinateModeCommand:
        case GCodeTaskType::EleparamSetCommand:
        case GCodeTaskType::FeedSpeedSetCommand:
        case GCodeTaskType::ProgramEndCommand:
            // 本地运行的task, 直接暂停在本地
            state_ = State::Paused;
            return true;
        default: {
            bool ret = _cmd_auto_pause();
            if (!ret) {
                return false;
            }   

            state_ = State::WaitingForPaused;

            return true;
        }
        }
    }
    case State::WaitingForPaused:
    case State::Paused:
        return true;
    case State::WaitingForStopped:
    case State::Stopped:
        return false;
    }
}

bool GCodeRunner::_cmd_auto_pause() { return false; }

void GCodeRunner::_run_once() {
    switch (state_) {
    case State::Running: {
        break;
    }
    case State::WaitingForPaused: {
        break;
    }
    case State::Paused:
    case State::WaitingForStopped:
    case State::Stopped:
        break;
    }
}

} // namespace task

} // namespace edm
