#pragma once

#include "Motion/JumpDefines.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "MotionAutoTask.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include <chrono>

namespace edm {

namespace move {

class G04AutoTask final : public AutoTask {
public:
    G04AutoTask(double delay_s)
        : AutoTask(AutoTaskType::G04Delay) {
        target_delay_peroids_ =
            util::UnitConverter::ms2p(std::round(delay_s * 1000.0));
        if (target_delay_peroids_ < 1) {
            target_delay_peroids_ = 1;
        }
    }

    bool pause() override {
        switch (state_) {
        case State::Stopped:
        default:
            return false;
        case State::Paused:
            return true;
        case State::NormalDelaying:
            state_ = State::Paused;
            return true;
        }
    }
    bool resume() override {
        switch (state_) {
        case State::Stopped:
        case State::NormalDelaying:
        default:
            return false;
        case State::Paused:
            state_ = State::NormalDelaying;
            return true;
        }
    }
    bool stop(bool immediate [[maybe_unused]] = false) override {
        state_ = State::Stopped;
        return true;
    }

    bool is_normal_running() const override {
        return state_ == State::NormalDelaying;
    }
    bool is_pausing() const override { return false; }
    bool is_paused() const override { return state_ == State::Paused; }
    bool is_resuming() const override { return false; }
    bool is_stopping() const override { return false; }
    bool is_stopped() const override { return state_ == State::Stopped; }
    bool is_over() const override { return is_stopped(); }

    void run_once() override {
        switch (state_) {
        case State::NormalDelaying: {
            ++curr_delayed_peroids_;
            if (curr_delayed_peroids_ >= target_delay_peroids_) {
                state_ = State::Stopped;
            }
            break;
        }
        case State::Paused:
        case State::Stopped:
        default:
            break;
        }
    }

private:
    enum class State {
        NormalDelaying,
        Paused,
        Stopped,
    };

    State state_{State::NormalDelaying};

    uint64_t curr_delayed_peroids_{0};

    uint64_t target_delay_peroids_;
};

} // namespace move

} // namespace edm
