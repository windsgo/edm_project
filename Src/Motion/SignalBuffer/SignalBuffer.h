#pragma once

#include <memory>
#include <array>

#include "Motion/MoveDefines.h"

namespace edm {

namespace move {

// 用于记录当前周期产生的信号
class SignalBuffer final {
public:
    using ptr = std::shared_ptr<SignalBuffer>;
    SignalBuffer() noexcept = default;
    ~SignalBuffer() noexcept = default;

    void reset_all();

    void set_signal(MotionSignalType signal);
    // void reset_signal(MotionSignalType signal);

    inline bool has_signal() const { return has_signal_; }

    const auto& get_signals_arr() const { return signals_arr_; }

private:
    bool has_signal_ {false};
    std::array<bool, static_cast<int>(MotionSignal_MAX)> signals_arr_ {false};
};

} // namespace move

} // namespace edm

