#include "IOController.h"

#include "QtDependComponents/CanController/CanController.h"

#include <QCanBusFrame>

namespace edm {

namespace io {

static auto s_can_ctrler = can::CanController::instance();

IOController *IOController::instance() {
    static IOController instance;
    return &instance;
}

void IOController::init(int can_device_index) {
    inited_ = true;
    can_device_index_ = can_device_index;
}

void IOController::trigger_send_current_io() {
    if (!inited_)
        return;

    uint32_t io1;
    uint32_t io2;
    {
        std::lock_guard guard(mutex_can_io_);
        io1 = can_machineio_1_;
        io2 = can_machineio_2_;
    }

    // make frame and send

    // s_can_ctrler->send_frame(can_device_index_, );
}

uint32_t IOController::get_can_machineio_1_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_1_;
}

uint32_t IOController::get_can_machineio_2_safe() const {
    std::lock_guard guard(mutex_can_io_);
    return can_machineio_2_;
}

} // namespace io

} // namespace edm