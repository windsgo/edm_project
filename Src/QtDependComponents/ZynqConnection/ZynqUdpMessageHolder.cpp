#include "ZynqUdpMessageHolder.h"
#include "QtDependComponents/ZynqConnection/UdpMessageDefine.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
#include <functional>
#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "Utils/Format/edm_format.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace zynq {

ZynqUdpMessageHolder::ZynqUdpMessageHolder(ZynqConnectController::ptr zynq_ctrler)
    : zynq_ctrler_(zynq_ctrler)
{
    _init_udp_listener();
}

void ZynqUdpMessageHolder::_init_udp_listener()
{
    zynq_ctrler_->add_udp_listener(std::bind_front(&ZynqUdpMessageHolder::_udp_listener_cb, this));
}

void ZynqUdpMessageHolder::_udp_listener_cb(const QByteArray& ba)
{
    //! this call-back cb is called in the Zynq Controller Thread

    auto udp_message_ptr = reinterpret_cast<const struct udp_message*>(ba.data());

    s_logger->debug("udp received: {}", udp_message_ptr->input_io);

    // atomic operation
    at_udp_message_cached_.store(*udp_message_ptr);
}

void ZynqUdpMessageHolder::get_udp_message(udp_message& output) const
{
    // atomic operation
    output = at_udp_message_cached_.load();
}

}
}