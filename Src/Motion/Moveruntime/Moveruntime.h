#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string.h>

#include "Motion/MoveDefines.h"
#include "config.h"

namespace edm {

namespace move {

class MoveRuntimePlanSpeedInput {
public:
    using ptr = std::shared_ptr<MoveRuntimePlanSpeedInput>;

    unit_t acc0{500000};
    unit_t dec0{-acc0};
    unit_t cruise_v{1000};
    unit_t entry_v{0};
    unit_t exit_v{0};

    uint32_t nacc{30}; // 加加速度周期数(1约等于梯形, 30~100为推荐的S形)
};

//! S型离线速度规划器
class Moveruntime final {
private:
    enum class State { NotRunning, Running };

public:
    using ptr = std::shared_ptr<Moveruntime>;
    Moveruntime() noexcept;
    ~Moveruntime() noexcept = default;

    bool plan(const MoveRuntimePlanSpeedInput& speed_param, unit_t target_length);

    void clear();

    // run once, and return the inc
    unit_t run_once();

    //! current_length在运行完毕后也不一定严格等于 total_length_, 会有累积舍入误差
    inline unit_t get_current_length() const { return current_length_; }

    inline unit_t get_target_length() const { return target_length_; }

    inline bool is_running() const { return state_ == State::Running; }
    inline bool is_over() const { return !is_running(); }

    unit_t get_current_speed() const;

private:
    struct impl {
        double acc0; // blu / s^2
        double dec0; // blu / s^2
        double a13;
        double a57;
        double intermediate_e;
        double intermediate_p;
        double recordintermediate_p;
        double recordintermediate_e;
        int32_t N1, N2, N3, N4, N5, N6, N7;
        int32_t total_N;
        int32_t current_N;
        double T; /*cylce second*/
        double j1, j3, j5, j7;
        double entry_acc; // blu /s
        double exit_acc;  // blu /s

        double entry_velocity; // blu /s
        double cruise_velocity;
        double exit_velocity;

        double length; // length of line in blu

        double segment_velocity; // computed velocity for aline segment // blu/s
        double segment_velocity_prev; // blu/s
        double intermediate;          // blujin
        double recordintermediate;

        void clear() { memset(this, 0, sizeof(impl)); }
    };

private:
    unit_t target_length_{0.0};
    unit_t current_length_{0.0};

    State state_{State::NotRunning};

    impl impl_;
};

} // namespace move

} // namespace edm
