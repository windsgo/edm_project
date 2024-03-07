#include "MotionSignalQueue.h"

namespace edm {

namespace move {

const char *
MotionSignalQueue::GetMotionSignalTypeStr(MotionSignalType signal_type) {
    switch (signal_type) {
#define XX_(t__)             \
    case MotionSignal_##t__: \
        return #t__;
        XX_(ManualPointMoveStarted)
        XX_(ManualPointMoveStopped)
        XX_(AutoStarted)
        XX_(AutoPaused)
        XX_(AutoResumed)
        XX_(AutoStopped)

#undef XX_

    default:
        return "UnknowSignal";
    }
}

} // namespace move

} // namespace edm
