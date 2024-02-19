#pragma once

#include <array>
#include <mutex>

#include <QByteArray>

#include "EleparamDefine.h"

namespace edm {

namespace power {

class EleSettingsBuffer {
    friend class PowerController;

public:
    EleSettingsBuffer()
        : canframe_buffer_{QByteArray{8, 0x00}, QByteArray{8, 0x00}}, io_1_{0},
          io_2_{0} {
        canframe_buffer_[0][0] = 0xEB;
        canframe_buffer_[0][1] = 0x90;
        canframe_buffer_[0][2] = 0xE9;
        // canframe_buffer_[0][3] = 0x00;
        // canframe_buffer_[0][4] = 0x00;
        canframe_buffer_[0][5] = 0x09;
    }

    inline void init(uint32_t ori_io_1, uint32_t ori_io_2) {
        assert(canframe_buffer_[0].size() == 8);
        // canframe_buffer_[0].fill(0x00, 8);
        // canframe_buffer_[1].fill(0x00, 8);

        io_1_ = ori_io_1;
        io_2_ = ori_io_2;
    }

    inline void set_pulse_value(uint8_t pulse_value) {
        // 心跳
        canframe_buffer_[0][3] = pulse_value;
    }

    inline void calc_buffer_crc() {
        uint16_t end_check = 0x00;
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 8 - i; ++j) {
                end_check += static_cast<uint8_t>(canframe_buffer_[i][j]);
            }
        }

        canframe_buffer_[1][7] = static_cast<uint8_t>(end_check);
    }

    void set_contactors_io(uint32_t contactor_index, uint8_t status) {
        if (status) {
            if (contactor_index < 33) {
                io_1_ |= 1 << (contactor_index - 1);
            } else {
                io_2_ |= 1 << (contactor_index - 33);
            }
        } else {
            if (contactor_index < 33) {
                io_1_ &= ~(1 << (contactor_index - 1));
            } else {
                io_2_ &= ~(1 << (contactor_index - 33));
            }
        }
    }

private:
    std::array<QByteArray, 2> canframe_buffer_;
    uint32_t io_1_{0};
    uint32_t io_2_{0};
};

class PowerController final {
public:
    static PowerController *instance();

public:
    void init(int can_device_index);

    // 更新电参数缓冲区, 设置标志位之后需要重新调用
    // 根据电参数结构体、高频等标志位，设定缓冲区报文、缓冲区io(该io基于当前系统原先io)
    //! 一旦更新电参数, 必须尽快调用 trigger_send_contactors_io
    //! 以将缓存区io发送出去, 防止缓冲区io将系统其他操作覆盖
    void update_eleparam(const EleParam_dkd_t &new_eleparam);

    // 将缓冲区can报文进行心跳与校验设置，并发送出去
    void trigger_send_eleparam();

    // 将缓冲区io再发送一遍
    //! 要注意这个io如果不刷新就发送, 可能会覆盖调期间系统其他io的设定
    void trigger_send_contactors_io();

    // 高频开关接口
    //! 注意, 外部设定高频开关等之后, 只是修改了标志位
    //! 还需要再调用update_eleparam()来刷新电参数, 并调用trigger函数发送
    void set_highpower_on(bool on);
    bool is_highpower_on() const;

private:
    PowerController();
    ~PowerController() noexcept = default;

private:
    // 分别处理电参数对应can frame (不包括继电器IO, 继电器IO有单独函数统一处理)
    // 不包括心跳和校验计算、填充, 留到发送时进行填充
    void _fill_buffer_canframe(const EleParam_dkd_t &new_eleparam);

    void _handle_on(uint8_t param_on);
    void _handle_off(uint8_t param_off);
    void _handle_up_and_dn(uint8_t param_up, uint8_t param_dn);
    void _handle_ip(uint16_t param_ip);
    void _handle_hp(uint8_t param_hp);
    void _handle_ma(uint8_t param_ma);
    void _handle_sv(uint8_t param_sv);
    void _handle_al(uint8_t param_al);
    void _handle_oc(uint8_t param_oc);
    void _handle_pp(uint8_t param_pp);
    // lv 无 can frame操作
    // pl 无 can frame操作 (极性)
    void _handle_machbit();

private:
    // 继电器IO设定处理
    void _fill_buffer_io_value(const EleParam_dkd_t &new_eleparam);

    // NOW 与高频, ip, hp, pp 有关
    void _handle_contactors_NOW(const EleParam_dkd_t &new_eleparam);

    // MON, HON 与高频, hp, pp 有关
    void _handle_contactors_MON_HON(const EleParam_dkd_t &new_eleparam);

    // IP0 与高频, ip有关
    void _handle_contactors_IP0(uint16_t ip);

    // LVx 与高频, lv有关
    void _handle_contactors_LVx(uint8_t lv);

    // PL POSITIVE 正极性继电器, 与pl有关
    void _handle_contactors_PL(uint8_t pl);

    // MACH 与高频有关
    void _handle_contactors_MACH();

    // CAP 电容继电器, 与c有关
    void _handle_contactors_CAP(uint8_t c);

private:
    // 向伺服IO板发送 伺服参数
    void _handle_servo_settings(const EleParam_dkd_t &new_eleparam);

private:
    // 特殊电参数覆盖处理 (IO覆盖, 由IO设定部分调用)
    void _handle_special_ele_index(uint16_t up_index);

private:
    // 触发发送电源can协议时, 处理缓冲区心跳和校验
    void _calc_buffer_pulse_and_crc();

private:
    // 电参数设定缓存(用于从参数和高频开关位转换为继电器io与电源can帧)
    EleSettingsBuffer curr_elesettings_buffer_;

    bool inited_ = false; // if not inited, no io can frame will be sent

    int can_device_index_ = -1; // used to send can frames by can::CanController

    // 高频开关标志位(影响继电器, 切换加工时用)
    uint8_t highpower_on_flag_ = 0;

    // 允许高频标志位(影响电源can帧标志位, 抬刀时切换电压用)
    uint8_t machpower_flag_ = 1;

    // 电参数can帧心跳值
    uint8_t canframe_pulse_value_ = 0;

    mutable std::mutex mutex_;

private:
    constexpr static const int POWERCAN_TXID = 0x0010; // 电源参数CAN帧TxID
    constexpr static const int IOBOARD_SERVOSETTINGS_TXID =
        0x0010; // IO板: 伺服参数设定 CAN帧TxID
};

} // namespace power

} // namespace edm
