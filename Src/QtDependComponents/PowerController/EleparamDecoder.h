#pragma once

#include <array>
#include <memory>
#include <optional>

#include <QByteArray>

#include "EleparamDefine.h"

namespace edm {

namespace power {

class EleparamDecodeResult final {
public:
    using ptr = std::shared_ptr<EleparamDecodeResult>;

public:
    EleparamDecodeResult()
        : can_buffer_{QByteArray{8, 0x00}, QByteArray{8, 0x00}}, io_1_(0x00),
          io_2_(0x00) {}
    ~EleparamDecodeResult() = default;

    auto &can_buffer() { return can_buffer_; }
    const auto &can_buffer() const { return can_buffer_; }

    auto &io_1() { return io_1_; }
    const auto &io_1() const { return io_1_; }

    auto &io_2() { return io_2_; }
    const auto &io_2() const { return io_2_; }

public:
    // 提供单独设定新的心跳值的接口
    void set_pulse_count(uint8_t count) {
        can_buffer_[0][3] = count;

        uint16_t sum = 0x00;
        // 重新计算校验和
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 8 - i; ++j) {
                sum += static_cast<uint8_t>(can_buffer_[i][j]);
            }
        }

        can_buffer_[1][7] = static_cast<uint8_t>(sum);
    }

public:
    static auto get_io_1_mask() { return io_1_mask; }
    static auto get_io_2_mask() { return io_2_mask; }

private:
    std::array<QByteArray, 2> can_buffer_;
    uint32_t io_1_;
    uint32_t io_2_;

private:
    // 定义电参数控制的 io_1 和 io_2 的掩码
    static const uint32_t io_1_mask;
    static const uint32_t io_2_mask;
};

class EleparamDecodeInput final {
public:
    using ptr = std::shared_ptr<EleparamDecodeInput>;

public:
    EleparamDecodeInput(const EleParam_dkd_t& ele_param, uint8_t highpower_flag,
                        uint8_t machpower_flag, uint16_t counter)
        : ele_param_(ele_param), highpower_flag_(highpower_flag),
          machpower_flag_(machpower_flag), counter_(counter) {}
    ~EleparamDecodeInput() = default;

    const auto& ele_param() const { return ele_param_; }
    auto highpower_flag() const { return highpower_flag_; }
    auto machpower_flag() const { return machpower_flag_; }
    auto counter() const { return counter_; }

private:
    EleParam_dkd_t ele_param_;
    uint8_t highpower_flag_;
    uint8_t machpower_flag_; // can 帧 mach 高频使能位
    uint16_t counter_;       // 计数器(用于填充心跳)
};

class EleparamDecoder final {
public:
    static EleparamDecodeResult::ptr decode(EleparamDecodeInput::ptr input);

private:
    void _decode(EleparamDecodeInput::ptr input);
    auto _get_result() const { return result_; }

private:
    // decode 总步骤
    void _fill_can_buffer();  // 填充can frame的buffer
    void _fill_io_settings(); // 填充(设置)IO值

private:
    // can frame 填充步骤
    void _canframe_fill_fixedbytes();

    void _canframe_handle_on();
    void _canframe_handle_off();
    void _canframe_handle_up_and_on();
    void _canframe_handle_ip();
    void _canframe_handle_hp();
    void _canframe_handle_ma();
    void _canframe_handle_sv();
    void _canframe_handle_al();
    void _canframe_handle_ld();
    void _canframe_handle_oc();
    void _canframe_handle_pp();
    void _canframe_handle_lv() {} // do nothing (只影响继电器)
    void _canframe_handle_pl() {} // do nothing (只影响继电器)
    void _canframe_handle_machbit();

    void _canframe_handle_pulsecounter(); // 心跳
    void _canframe_calc_crc(); // 校验

private:
    // io 设置辅助函数
    //! 要注意 NOW 和 PK 继电器相反的硬件逻辑
    void _set_contactor_io(uint32_t contactor_index, bool enable);

    //! NOW, PK 单独设置函数(反逻辑)
    void _set_contactor_io_NOW(bool enable);
    void _set_contactor_io_PK(bool enable);

    // 其他继电器单独设置函数
    void _set_contactor_io_MON(bool enable);
    void _set_contactor_io_HON(bool enable);
    
    void _set_contactor_io_IP0(bool enable); //! IP0 反逻辑
    void _set_contactor_io_IP7(bool enable);
    void _set_contactor_io_IP15(bool enable);

    void _set_contactor_io_LV1(bool enable);
    void _set_contactor_io_LV2(bool enable);

    void _set_contactor_io_RVNM(bool enable);

    void _set_contactor_io_MACH(bool enable);

    // 电容电路继电器, x = 0 ~ 9
    void _set_contactor_io_Cx(uint32_t x, bool enable);

private:
    // io settings 填充步骤
    void _iosettings_handle_NOW();
    void _iosettings_handle_MON_HON();
    void _iosettings_handle_IPx();
    void _iosettings_handle_LVx();
    void _iosettings_handle_PL();
    void _iosettings_handle_MACH();
    void _iosettings_handle_PK();
    void _iosettings_handle_CAPx();

private:
    EleparamDecoder() = default;
    ~EleparamDecoder() = default;

private:
    EleparamDecodeInput::ptr input_;
    EleparamDecodeResult::ptr result_;
};

} // namespace power

} // namespace edm
