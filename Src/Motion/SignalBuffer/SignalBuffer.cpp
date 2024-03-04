#include "SignalBuffer.h"

namespace edm {

namespace move {
void SignalBuffer::reset_all() { signals_arr_.fill(false); }

void SignalBuffer::set_signal(MotionSignalType signal) {
    if (!has_signal_)
        has_signal_ = true;
    signals_arr_[static_cast<int>(signal)] = true;
}

// void SignalBuffer::reset_signal(MotionSignalType signal) {
//     signals_arr_[static_cast<int>(signal)] = false;}

} // namespace move

} // namespace edm
