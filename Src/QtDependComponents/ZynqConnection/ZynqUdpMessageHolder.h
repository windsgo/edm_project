#pragma once

#include "ZynqConnectController.h"
#include "UdpMessageDefine.h"



namespace edm {
namespace zynq {

class ZynqUdpMessageHolder {
public:
    using ptr = std::shared_ptr<ZynqUdpMessageHolder>;

public:
    ZynqUdpMessageHolder(ZynqConnectController::ptr zynq_ctrler);

    void get_udp_message(servo_return_converted_data_t& output) const;

private:
    void _init_udp_listener();
    
    void _udp_listener_cb(const QByteArray& ba);

private:
    ZynqConnectController::ptr zynq_ctrler_;

    std::atomic<servo_return_converted_data_t> at_udp_message_converted_cached_;
};

}
}
