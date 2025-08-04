#include "HandboxConverter.h"

#include "Logger/LogMacro.h"
#include <cstddef>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

HandboxConverter::HandboxConverter(
    can::CanController::ptr can_ctrler, uint32_t can_device_index,
    const std::function<void(const move::axis_t &, uint32_t, bool)>
        &cb_start_manual_pointmove,
    const std::function<void(void)> &cb_stop_manual_pointmove,
    const std::function<void(void)> &cb_pause_auto,
    const std::function<void(void)> &cb_ent_auto,
    const std::function<void(void)> &cb_stop_auto,
    const std::function<void(void)> &cb_ack,
    const std::function<void(uint8_t)> &cb_pump)
    : cb_start_manual_pointmove_(cb_start_manual_pointmove),
      cb_stop_manual_pointmove_(cb_stop_manual_pointmove),
      cb_pause_auto_(cb_pause_auto), cb_ent_auto_(cb_ent_auto),
      cb_stop_auto_(cb_stop_auto), cb_ack_(cb_ack), cb_pump_(cb_pump) {
    auto cb = std::bind_front(&HandboxConverter::_listen_cb, this);

    can_ctrler->add_frame_received_listener(can_device_index, cb);
}

void HandboxConverter::_listen_cb(const QCanBusFrame &frame) {
    if (!frame.hasExtendedFrameFormat()) {
        return;
    }

    uint32_t func_id = (frame.frameId() >> 4) & 0xFF;

    if (func_id == 0 && func_id == 1) {
        return;
    }

    s_logger->trace("drillbox funcid: {}", func_id);

    switch (func_id) {
    case id::start_pointmove:
        _frame_start_pointmove(frame);
        break;
    case id::stop_pointmove:
        _frame_stop_pointmove();
        break;
    case id::pump:
        _frame_pump(frame);
        break;
    case id::ent_auto:
        _frame_ent_auto();
        break;
    case id::pause_auto:
        _frame_pause_auto();
        break;
    case id::stop_auto:
        _frame_stop_auto();
        break;
    case id::ack:
        _frame_ack();
        break;

    default:
        s_logger->warn("unknow drillbox funcid: {}", func_id);
        break;
    }
}

void HandboxConverter::_frame_start_pointmove(const QCanBusFrame &frame) {
    const uint8_t *p_uchar =
        reinterpret_cast<uint8_t *>(frame.payload().data());

    uint8_t axis_bits = p_uchar[0];
    uint8_t dir_bits = p_uchar[1];
    uint8_t speed_level = p_uchar[5]; // 速度档位 0-3
    bool touch_detect_enable = !p_uchar[6]; // 1-按住st, 0-不按st

    move::axis_t dir {0.0};
    for (uint8_t i = 0; i < dir.size(); ++i) {
        if (axis_bits & (1 << i)) {
            if (dir_bits & (1 << i)) {
                // Pos
                dir[i] = 1.0;
            } else {
                // Neg
                dir[i] = -1.0;
            }
        } else {
            dir[i] = 0.0;
        }
    }

    if (speed_level > 3) {
        speed_level = 3;
    }

    cb_start_manual_pointmove_(dir, speed_level, touch_detect_enable);
}

void HandboxConverter::_frame_stop_pointmove() { cb_stop_manual_pointmove_(); }

void HandboxConverter::_frame_pump(const QCanBusFrame &frame) {
    const uint32_t *p_uint32 =
        reinterpret_cast<uint32_t *>(frame.payload().data());

    uint32_t machineio = *p_uint32;

#if 1
    cb_pump_(machineio);
#else
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL) \
    || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) \
    || defined(EDM_POWER_DIMEN_WITH_EXTRA_ZHONGGU_IO)
    if (machineio == 5) {
        // 开油泵
        cb_pump_on_(true);
    } else if (machineio == 6) {
        // 关油泵
        cb_pump_on_(false);
    }
#else
    // TODO
    if (machineio == 3) {
        // 开油泵
        cb_pump_on_(true);
    } else if (machineio == 4) {
        // 关油泵
        cb_pump_on_(false);
    }
#endif
#endif
}

void HandboxConverter::_frame_ent_auto() { cb_ent_auto_(); }

void HandboxConverter::_frame_pause_auto() { cb_pause_auto_(); }

void HandboxConverter::_frame_stop_auto() { cb_stop_auto_(); }

void HandboxConverter::_frame_ack() { cb_ack_(); }

} // namespace edm
