#pragma once

#include "Motion/JumpDefines.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "MotionAutoTask.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include <chrono>

namespace edm {

namespace move {

class M00FakeAutoTask final : public AutoTask {
public:
    M00FakeAutoTask(const axis_t &init_axis)
        : AutoTask(AutoTaskType::M00Fake, init_axis) {}

    bool pause() override {
        switch (state_) {
        case State::Stopped:
        default:
            return false;
        case State::Paused:
            return true;
        }
    }
    bool resume() override {
        switch (state_) {
        case State::Stopped:
        default:
            return false;
        case State::Paused:
            state_ = State::Stopped;
            return true;
        }
    }
    bool stop(bool immediate [[maybe_unused]] = false) override {
        state_ = State::Stopped;
        return true;
    }

    bool is_normal_running() const override { return false; }
    bool is_pausing() const override { return false; }
    bool is_paused() const override { return state_ == State::Paused; }
    bool is_resuming() const override { return false; }
    bool is_stopping() const override { return false; }
    bool is_stopped() const override { return state_ == State::Stopped; }
    bool is_over() const override { return is_stopped(); }

    void run_once() override { }

private:
    enum class State {
        Paused,
        Stopped,
    };

    State state_{State::Paused};
};

} // namespace move

} // namespace edm
