#include "ZynqUdpMessageHolder.h"
#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "QtDependComponents/ZynqConnection/UdpMessageDefine.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
#include "Utils/Format/edm_format.h"
#include <functional>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace zynq {

ZynqUdpMessageHolder::ZynqUdpMessageHolder(
    ZynqConnectController::ptr zynq_ctrler)
    : zynq_ctrler_(zynq_ctrler) {
    _init_udp_listener();
}

void ZynqUdpMessageHolder::_init_udp_listener() {
    zynq_ctrler_->add_udp_listener(
        std::bind_front(&ZynqUdpMessageHolder::_udp_listener_cb, this));
}

void ZynqUdpMessageHolder::_udp_listener_cb(const QByteArray &ba) {
    //! this call-back cb is called in the Zynq Controller Thread

    auto udp_message_ptr =
        reinterpret_cast<const servo_return_data_t *>(ba.data() + 4);

    // atomic operation
    at_udp_message_cached_.store(*udp_message_ptr);
}

void ZynqUdpMessageHolder::get_udp_message(servo_return_data_t &output) const {
    // atomic operation
    output = at_udp_message_cached_.load();
}

} // namespace zynq
} // namespace edm