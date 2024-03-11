#pragma once

#include <functional>
#include <memory>

#include "Motion/MoveDefines.h"
#include "QtDependComponents/CanController/CanController.h"

namespace edm {

//! 仅适用于 HCENT 3轴成型机
//! 如果是6轴机床还需重新适配
class HandboxConverter final {
public:
    using ptr = std::shared_ptr<HandboxConverter>;

    //! 传入的回调函数需要是线程安全的, 且开销极小的, 这些函数会在CAN线程中调用
    HandboxConverter(can::CanController::ptr can_ctrler,
                     uint32_t can_device_index,
                     const std::function<void(const move::axis_t &, uint32_t,
                                              bool)> &cb_start_manual_pointmove,
                     const std::function<void(void)> &cb_stop_manual_pointmove,
                     const std::function<void(void)> &cb_pause_auto,
                     const std::function<void(void)> &cb_ent_auto,
                     const std::function<void(void)> &cb_stop_auto,
                     const std::function<void(void)> &cb_ack,
                     const std::function<void(bool)> &cb_pump_on);

private:
    enum id {
        start_pointmove = 6,
        stop_pointmove = 4,
        pump = 10,
        ent_auto = 11,
        pause_auto = 12,
        stop_auto = 13,
        ack = 20
    };

private:
    void _listen_cb(const QCanBusFrame &frame);

    void _frame_start_pointmove(const QCanBusFrame &frame);
    void _frame_stop_pointmove();
    void _frame_pump(const QCanBusFrame &frame);
    void _frame_ent_auto();
    void _frame_pause_auto();
    void _frame_stop_auto();
    void _frame_ack();

private:
    std::function<void(const move::axis_t &, uint32_t, bool)>
        cb_start_manual_pointmove_; // 方向; 速度档位
    std::function<void(void)> cb_stop_manual_pointmove_;

    std::function<void(void)> cb_pause_auto_; // HALT
    std::function<void(void)>
        cb_ent_auto_; // ENT, ENT只用于继续(resume), 现不用于开始
    std::function<void(void)> cb_stop_auto_; // OFF

    std::function<void(void)> cb_ack_; // cb_ack_

    std::function<void(bool)> cb_pump_on_; // 油泵开关
};

} // namespace edm
