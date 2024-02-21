#include "EleparamDecoder.h"

#include "Logger/LogMacro.h"

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace power {

// 用于计算IO1Mask, 将需要以IO1控制的继电器IO枚举值填入
static constexpr const std::array<uint32_t, 13> s_contactors_io_1_arr{
    EleContactorOut_V1_JV1,   EleContactorOut_V2_JV2,  EleContactorOut_C0_JC0,
    EleContactorOut_C1_JC1,   EleContactorOut_C2_JC2,  EleContactorOut_C3_JC3,
    EleContactorOut_C4_JC4,   EleContactorOut_C5_JC5,  EleContactorOut_C6_JC6,
    EleContactorOut_C7_JC7,   EleContactorOut_C8_JC8,  EleContactorOut_C9_JC9,
    EleContactorOut_MACH_JF0, /* EleContactorOut_PWON_JF1 */};

// 用于计算IO2Mask, 将需要以IO2控制的继电器IO枚举值填入 (>32)
static constexpr const std::array<uint32_t, 12> s_contactors_io_2_arr{
    /* EleContactorOut_BZ_JF2, */ EleContactorOut_HON_JF3,
    EleContactorOut_MON_JF4,
    EleContactorOut_UNUSED_JF5,
    EleContactorOut_PK_JF6, /* EleContactorOut_FULD_JF7, */
    EleContactorOut_RVNM_JF8,
    EleContactorOut_PK0_JF9,
    EleContactorOut_UNUSED_JFA,
    EleContactorOut_IP0_JFB,
    EleContactorOut_IP7_JFC,
    EleContactorOut_NOW_JFD,
    EleContactorOut_UNUSED_JFE,
    EleContactorOut_IP15_JFF,
    /* EleContactorOut_SOF */};

static uint32_t _CalcCanIO1Mask() {
    uint32_t mask = 0x00;
    for (const auto &contactor_index : s_contactors_io_1_arr) {
        if (contactor_index == 0 || contactor_index > 32) {
            continue;
        }

        mask |= 1 << (contactor_index - 1);
    }

    return mask;
}

static uint32_t _CalcCanIO2Mask() {
    uint32_t mask = 0x00;
    for (const auto &contactor_index : s_contactors_io_2_arr) {
        if (contactor_index <= 32 || contactor_index > 64) {
            continue;
        }

        mask |= 1 << (contactor_index - 33);
    }

    return mask;
}

const uint32_t EleparamDecodeResult::io_1_mask = _CalcCanIO1Mask();
const uint32_t EleparamDecodeResult::io_2_mask = _CalcCanIO2Mask();

EleparamDecodeResult::ptr
EleparamDecoder::decode(EleparamDecodeInput::ptr input) {
    auto decoder = EleparamDecoder();
    decoder._decode(input);

    return decoder._get_result();
}

void EleparamDecoder::_decode(EleparamDecodeInput::ptr input) {
    input_ = input; // save input

    // initialize result ptr
    result_ = std::make_shared<EleparamDecodeResult>();

    // start step by step decode

    _fill_can_buffer();

    _fill_io_settings();
}

void EleparamDecoder::_fill_can_buffer() {
    _canframe_fill_fixedbytes();
    
    _canframe_handle_on();
    _canframe_handle_off();
    _canframe_handle_up_and_on();
    _canframe_handle_ip();
    _canframe_handle_hp();
    _canframe_handle_ma();
    _canframe_handle_sv();
    _canframe_handle_al();
    _canframe_handle_ld();
    _canframe_handle_oc();
    _canframe_handle_pp();
    _canframe_handle_lv();
    _canframe_handle_pl();
    _canframe_handle_machbit();

    _canframe_handle_pulsecounter();
    _canframe_calc_crc();
}

void EleparamDecoder::_fill_io_settings() {
    _iosettings_handle_NOW();
    _iosettings_handle_MON_HON();
    _iosettings_handle_IPx();
    _iosettings_handle_LVx();
    _iosettings_handle_PL();
    _iosettings_handle_MACH();
    _iosettings_handle_PK();
    _iosettings_handle_CAPx();
}

#define CAN_BUFFER                   result_->can_buffer()
#define OR_EQUAL(__ori, __or_value)  (__ori) = (__ori) | (__or_value)
#define AND_EQUAL(__ori, __or_value) (__ori) = (__ori) & (__or_value)

void EleparamDecoder::_canframe_fill_fixedbytes() {
    CAN_BUFFER[0][0] = 0xEB;
    CAN_BUFFER[0][1] = 0x90;
    CAN_BUFFER[0][2] = 0xE9;
    CAN_BUFFER[0][5] = 0x09;
}

void EleparamDecoder::_canframe_handle_on() {
    uint8_t param_on = input_->ele_param()->pulse_on; // 参数值
    uint8_t pulse_on = param_on;

    if ((pulse_on > 63 && pulse_on < 100) || pulse_on > 107) {
        s_logger->error("eleparam error, pulse_on = {}", pulse_on);
        pulse_on = 0;
    }

    if (param_on <= 7) {
        pulse_on &= 0x3F; // 第6,7位设为 0
    } else if (param_on < 64) {
        pulse_on |= 0x40; // 第6位设为 1
        pulse_on &= 0x7F; // 第7位设为 0
        // pulse_on |= 0xC0; //第6,7位设为 1
    } else if (param_on >= 100 && param_on <= 107) {
        pulse_on -= 100;  // 100-107按0-7
        pulse_on &= 0xBF; // 第6位设为0
        pulse_on &= 0x7F; // 第7位设为 0
        // pulse_on |= 0x80; // 第7为设为1
    }

    CAN_BUFFER[0][6] = pulse_on;

    // 设置CK2位
    if (param_on > 15 && param_on < 64) // CK2设为1
        OR_EQUAL(CAN_BUFFER[1][6], 0x02);
    else
        AND_EQUAL(CAN_BUFFER[1][6], 0xFD);
}

void EleparamDecoder::_canframe_handle_off() {
    CAN_BUFFER[0][7] = input_->ele_param()->pulse_off;
}

void EleparamDecoder::_canframe_handle_up_and_on() {
    // do nothing
    // TODO up 和 dn 要设置给运动控制(抬刀用), 靠外层设置, 这里只是decode
}

void EleparamDecoder::_canframe_handle_ip() {
    uint16_t param_ip = input_->ele_param()->ip;
    uint16_t ip = param_ip;
    uint8_t ip_decimal_part = ip % 10;                    // ip 的 小数部分
    uint8_t ip_real_part = static_cast<uint8_t>(ip / 10); // ip 的整数部分

    CAN_BUFFER[1][1] = ip_real_part;

    /**
     * 0.1  = 0.1
     * 0.2  =       0.2
     * 0.3  = 0.1 + 0.2
     * 0.4 x
     * 0.5  =             0.5
     * 0.6  = 0.1 +       0.5
     * 0.7  =       0.2 + 0.5
     * 0.8  = 0.1 + 0.2 + 0.5
     * 0.9 x
     */
    if (ip_decimal_part == 1 || ip_decimal_part == 3 || ip_decimal_part == 6 ||
        ip_decimal_part == 8) {
        // ip 含有 0.1A
        OR_EQUAL(CAN_BUFFER[1][1], 0x80);
    } else {
        // ip 不含 0.1A
        AND_EQUAL(CAN_BUFFER[1][1], 0x7F);
    }

    // 清空 ELEBUFFER[1][2]的低4位
    AND_EQUAL(CAN_BUFFER[1][2], 0xF0);

    if (ip_decimal_part == 2 || ip_decimal_part == 3 || ip_decimal_part == 7 ||
        ip_decimal_part == 8) {
        // ip 含有 0.2A
        OR_EQUAL(CAN_BUFFER[1][2], 0x01);
    } else {
        // ip 不含 0.2A
        AND_EQUAL(CAN_BUFFER[1][2], 0xFE);
    }

    if (ip_decimal_part == 5 || ip_decimal_part == 6 || ip_decimal_part == 7 ||
        ip_decimal_part == 8) {
        // ip 含有 0.5A
        OR_EQUAL(CAN_BUFFER[1][2], 0x02);
    } else {
        // ip 不含 0.5A
        AND_EQUAL(CAN_BUFFER[1][2], 0xFD);
    }
}

void EleparamDecoder::_canframe_handle_hp() {
    uint8_t param_hp = input_->ele_param()->hp;
    uint8_t hp = param_hp;
    uint8_t hp_units = hp % 10;            // 个位
    uint8_t hp_tens = (hp / 10) % 10;      // 十位
    uint8_t hp_hundreds = (hp / 100) % 10; // 百位

    // 清空 ELEBUFFER[1][2]的高4位
    AND_EQUAL(CAN_BUFFER[1][2], 0x0F);
    OR_EQUAL(CAN_BUFFER[1][2], hp_units << 4); // hp个位

    if (hp_tens == 2 || hp_tens == 3 || hp_tens == 6 || hp_tens == 7) {
        OR_EQUAL(CAN_BUFFER[1][6], 0x01); // CPS bit0 设为1
    } else {
        AND_EQUAL(CAN_BUFFER[1][6], 0xFE);
    }
}

void EleparamDecoder::_canframe_handle_ma() {
    AND_EQUAL(CAN_BUFFER[1][3], 0xF0);                   // 清空低4位
    OR_EQUAL(CAN_BUFFER[1][3], (input_->ele_param()->ma) & 0x0F); // 低4位设定ma
}

void EleparamDecoder::_canframe_handle_sv() {
    // sv 伺服电压 (应当由伺服采样模块(目前为IO采样板)处理, 不需要发送给电源)
    // TODO 外部要将sv值通过can发送给采样板

    // AND_EQUAL(CAN_BUFFER[1][3], 0x0F);                        // 清空高4位
    // OR_EQUAL(CAN_BUFFER[1][3], input_->ele_param()->sv << 4); // 高4位设定sv
}

void EleparamDecoder::_canframe_handle_al() {
    CAN_BUFFER[1][4] = input_->ele_param()->al;
}

void EleparamDecoder::_canframe_handle_ld() {
    AND_EQUAL(CAN_BUFFER[1][5], 0xF0);                   // 清空低4位
    OR_EQUAL(CAN_BUFFER[1][5], (input_->ele_param()->ld) & 0x0F); // 低4位设定ld
}

void EleparamDecoder::_canframe_handle_oc() {
    AND_EQUAL(CAN_BUFFER[1][5], 0x0F);                        // 清空高4位
    OR_EQUAL(CAN_BUFFER[1][5], input_->ele_param()->oc << 4); // 高4位设定oc
}

void EleparamDecoder::_canframe_handle_pp() {
    uint8_t pp = input_->ele_param()->pp;
    if (input_->highpower_flag() == 0) {
        pp = 0; // 高频未使能
    }

    if (pp & 0x01) {
        // TODO
        // 如果pp位*1 (?什么意思), 设置NWS bit4 为1
        OR_EQUAL(CAN_BUFFER[1][6], 0x10);
    } else {
        AND_EQUAL(CAN_BUFFER[1][6], 0xEF);
    }
}

void EleparamDecoder::_canframe_handle_machbit() {
    if (input_->highpower_flag() && input_->machpower_flag()) {
        // 高频允许 且 高压允许(非抬刀)
        OR_EQUAL(CAN_BUFFER[1][6], 0x20); // 允许高压
    } else {
        AND_EQUAL(CAN_BUFFER[1][6], 0xDF); // 切断高压
    }
}

void EleparamDecoder::_canframe_handle_pulsecounter() {
    CAN_BUFFER[0][3] = static_cast<uint8_t>(input_->counter());
}

void EleparamDecoder::_canframe_calc_crc() {
    uint16_t end_check = 0x00;
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 8 - i; ++j) {
            end_check += static_cast<uint8_t>(CAN_BUFFER[i][j]);
        }
    }

    CAN_BUFFER[1][7] = static_cast<uint8_t>(end_check);
}

void EleparamDecoder::_set_contactor_io(uint32_t contactor_index, bool enable) {
    // 处理反逻辑 NOW, PK
    if (contactor_index == EleContactorOut_NOW_JFD ||
        contactor_index == EleContactorOut_PK_JF6) {
        enable = !enable;
    }

    if (enable) {
        if (contactor_index < 33) {
            result_->io_1() |= 1 << (contactor_index - 1);
        } else {
            result_->io_2() |= 1 << (contactor_index - 33);
        }
    } else {
        if (contactor_index < 33) {
            result_->io_1() &= ~(1 << (contactor_index - 1));
        } else {
            result_->io_2() &= ~(1 << (contactor_index - 33));
        }
    }
}

void EleparamDecoder::_set_contactor_io_NOW(bool enable) {
    static const uint32_t now_io = 1 << (EleContactorOut_NOW_JFD - 33);

    //! NOW 反逻辑 enable(吸合) 为 0
    if (!enable) {
        result_->io_2() |= now_io; 
    } else {
        result_->io_2() &= ~now_io;
    }
}

void EleparamDecoder::_set_contactor_io_PK(bool enable) {
    static const uint32_t pk_io = 1 << (EleContactorOut_PK_JF6 - 33);

    //! PK 反逻辑 enable(吸合) 为 0
    if (!enable) {
        result_->io_2() |= pk_io; 
    } else {
        result_->io_2() &= ~pk_io;
    }
}

void EleparamDecoder::_set_contactor_io_MON(bool enable) {
    static const uint32_t mon_io = 1 << (EleContactorOut_MON_JF4 - 33);

    if (enable) {
        result_->io_2() |= mon_io; 
    } else {
        result_->io_2() &= ~mon_io;
    }
}

void EleparamDecoder::_set_contactor_io_HON(bool enable) {
    static const uint32_t hon_io = 1 << (EleContactorOut_HON_JF3 - 33);

    if (enable) {
        result_->io_2() |= hon_io; 
    } else {
        result_->io_2() &= ~hon_io;
    }
}

void EleparamDecoder::_set_contactor_io_IP0(bool enable) {
    static const uint32_t ip0_io = 1 << (EleContactorOut_IP0_JFB - 33);

    //! IP0 反逻辑 enable(吸合) 为 0
    if (!enable) {
        result_->io_2() |= ip0_io; 
    } else {
        result_->io_2() &= ~ip0_io;
    }
}

void EleparamDecoder::_set_contactor_io_IP7(bool enable) {
    static const uint32_t ip7_io = 1 << (EleContactorOut_IP7_JFC - 33);

    if (enable) {
        result_->io_2() |= ip7_io; 
    } else {
        result_->io_2() &= ~ip7_io;
    }
}

void EleparamDecoder::_set_contactor_io_IP15(bool enable) {
    static const uint32_t ip15_io = 1 << (EleContactorOut_IP15_JFF - 33);

    if (enable) {
        result_->io_2() |= ip15_io; 
    } else {
        result_->io_2() &= ~ip15_io;
    }
}

void EleparamDecoder::_set_contactor_io_LV1(bool enable) {
    static const uint32_t lv1_io = 1 << (EleContactorOut_V1_JV1 - 1);

    if (enable) {
        result_->io_1() |= lv1_io; 
    } else {
        result_->io_1() &= ~lv1_io;
    }
}

void EleparamDecoder::_set_contactor_io_LV2(bool enable) {
    static const uint32_t lv2_io = 1 << (EleContactorOut_V2_JV2 - 1);

    if (enable) {
        result_->io_1() |= lv2_io; 
    } else {
        result_->io_1() &= ~lv2_io;
    }
}

void EleparamDecoder::_set_contactor_io_RVNM(bool enable) {
    static const uint32_t rvnm_io = 1 << (EleContactorOut_RVNM_JF8 - 33);

    if (enable) {
        result_->io_2() |= rvnm_io; 
    } else {
        result_->io_2() &= ~rvnm_io;
    }
}

void EleparamDecoder::_set_contactor_io_MACH(bool enable) {
    static const uint32_t mach_io = 1 << (EleContactorOut_MACH_JF0 - 1);

    if (enable) {
        result_->io_1() |= mach_io; 
    } else {
        result_->io_1() &= ~mach_io;
    }
}

void EleparamDecoder::_set_contactor_io_Cx(uint32_t x, bool enable) {
    if (x > 9) {
        s_logger->warn("_set_contactor_io_Cx x({}) > 9, set to 9", x);
        x = 9;
    }

    uint32_t cx_io = 1 << (EleContactorOut_C0_JC0 + x - 1);
    
    if (enable) {
        result_->io_1() |= cx_io; 
    } else {
        result_->io_1() &= ~cx_io;
    }
}

void EleparamDecoder::_iosettings_handle_NOW() {
    // 先处理特殊条件 C901 C902
    auto upper_index =  input_->ele_param()->upper_index;
    if (upper_index == 901 || upper_index == 902) {
        _set_contactor_io_NOW(false); // 不吸合
        return;
    }

    // 处理特殊 IP > 7 
    if (input_->ele_param()->ip > 70) {
        _set_contactor_io_NOW(true); // 吸合
        return;
    }

    // 处理普通 HP 控制
    uint8_t hp_tens = (input_->ele_param()->hp / 10) % 10;
    if (hp_tens < 4) {
        _set_contactor_io_NOW(true); // HP = 0x,1x,2x,3x 吸合
    } else {
        _set_contactor_io_NOW(false); // HP = 4x,5x,6x,7x 不吸合
    }
}

void EleparamDecoder::_iosettings_handle_MON_HON() {
    uint8_t pp_tens = (input_->ele_param()->pp / 10) % 10;
    uint8_t hp_tens = (input_->ele_param()->hp / 10) % 10;

    if (pp_tens != 1) {
        _set_contactor_io_MON(false);
        _set_contactor_io_HON(false);
        return;
    }

    if (hp_tens % 2 == 0) {
        // hp = *0* *2* *4* *6*
        // MON 吸合 HON 断开
        _set_contactor_io_MON(true);
        _set_contactor_io_HON(false);
    } else {
        // hp = *1* *3* *5* *7*
        // MON 断开 HON 吸合
        _set_contactor_io_MON(false);
        _set_contactor_io_HON(true);
    }
}

void EleparamDecoder::_iosettings_handle_IPx() {
    auto ip = input_->ele_param()->ip;
    auto upper_index = input_->ele_param()->upper_index;
    // 特殊情况 C901, C902, 全部关断S

    if (ip == 0 || upper_index == 901 || upper_index == 902) {
        _set_contactor_io_IP0(false);
        _set_contactor_io_IP7(false);
        _set_contactor_io_IP15(false);
    } else if (ip <= 70) {
        // ip > 0, ip <= 7
        _set_contactor_io_IP0(true);
        _set_contactor_io_IP7(false);
        _set_contactor_io_IP15(false);
    } else if (ip <= 150) {
        // ip > 7, ip <= 15
        _set_contactor_io_IP0(true);
        _set_contactor_io_IP7(true);
        _set_contactor_io_IP15(false);
    } else {
        // ip > 15
        _set_contactor_io_IP0(true);
        _set_contactor_io_IP7(true);
        _set_contactor_io_IP15(true);
    }
}

void EleparamDecoder::_iosettings_handle_LVx() {
    auto lv = input_->ele_param()->lv;

    if (lv == 1) {
        _set_contactor_io_LV1(true);
        _set_contactor_io_LV2(false);
    } else if (lv == 2) {
        _set_contactor_io_LV1(false);
        _set_contactor_io_LV2(true);
    } else {
        _set_contactor_io_LV1(false);
        _set_contactor_io_LV2(false);
    }
}

void EleparamDecoder::_iosettings_handle_PL() {
    uint8_t pl = input_->ele_param()->pl;

    if (pl == 0) { /* pl = 0 (对应上位机发的是 (+), 正极性) */
        // 正极性, OUT39 = 0, RV继电器实际为吸合(反逻辑)
        _set_contactor_io_RVNM(false);
    } else { /* pl = 1 (对应上位机发的是 (-), 负极性) */
        // 负极性, OUT39 = 1, RV继电器实际为不吸合(反逻辑)
        _set_contactor_io_RVNM(true);
    }
}

void EleparamDecoder::_iosettings_handle_MACH() {
    if (input_->highpower_flag()) {
        // 高频打开
        _set_contactor_io_MACH(true);
    } else {
        // 高频关闭
        _set_contactor_io_MACH(false);
    }
}

void EleparamDecoder::_iosettings_handle_PK() {
    auto upper_index =  input_->ele_param()->upper_index;
    if (upper_index == 901 || upper_index == 902) {
        _set_contactor_io_PK(false); // 不吸合 (只有901, 902吸合)
    } else {
        _set_contactor_io_PK(true); // 吸合
    }
}

void EleparamDecoder::_iosettings_handle_CAPx() {
    uint8_t c = input_->ele_param()->c;

    // 先全部设置关闭
    for (uint32_t i = 0; i <= 9; ++i) {
        _set_contactor_io_Cx(i, false);
    }

    // 再设置对应电容继电器开启
    _set_contactor_io_Cx(c, true);
}

} // namespace power

} // namespace edm
