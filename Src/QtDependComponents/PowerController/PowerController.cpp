#include "PowerController.h"

#include "Logger/LogMacro.h"
#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"

namespace edm {

namespace power {

static auto s_can_ctrler = can::CanController::instance();
static auto s_io_ctrler = io::IOController::instance();

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

// 给访问缓冲区与bytearray快捷操作宏
#define ELEBUFFER                    curr_elesettings_buffer_.canframe_buffer_
#define OR_EQUAL(__ori, __or_value)  (__ori) = (__ori) | (__or_value)
#define AND_EQUAL(__ori, __or_value) (__ori) = (__ori) & (__or_value)

PowerController::PowerController() {}

void PowerController::init(int can_device_index) {
    inited_ = true;
    can_device_index_ = can_device_index;
}

void PowerController::update_eleparam(const EleParam_dkd_t &new_eleparam) {
    // 获取IO模块当前IO值, 所有的IO需要覆盖在原先IO上
    curr_elesettings_buffer_.init(s_io_ctrler->get_can_machineio_1(),
                                  s_io_ctrler->get_can_machineio_2());

    // 填充 can frame (不包括心跳和校验)
    _fill_buffer_canframe(new_eleparam);

    // 填充 io 值
    _fill_buffer_io_value(new_eleparam);
}

void PowerController::_fill_buffer_canframe(
    const EleParam_dkd_t &new_eleparam) {
    // can frame 填充
    _handle_on(new_eleparam.pulse_on);
    _handle_off(new_eleparam.pulse_off);
    _handle_up_and_dn(new_eleparam.up, new_eleparam.dn);
    _handle_ip(new_eleparam.ip);
    _handle_hp(new_eleparam.hp);
    _handle_ma(new_eleparam.ma);
    _handle_sv(new_eleparam.sv);
    _handle_al(new_eleparam.al);
    _handle_oc(new_eleparam.oc);
    _handle_pp(new_eleparam.pp);
    _handle_machbit(); // 处理高压切断位

    // 完成canframe计算与填充 (除心跳和校验字节)
}

void PowerController::_handle_on(uint8_t param_on) {
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

    ELEBUFFER[0][6] = pulse_on;

    // 设置CK2位
    if (param_on > 15 && param_on < 64) // CK2设为1
        OR_EQUAL(ELEBUFFER[1][6], 0x02);
    else
        AND_EQUAL(ELEBUFFER[1][6], 0xFD);
}

void PowerController::_handle_off(uint8_t param_off) {
    ELEBUFFER[0][7] = param_off;
}

void PowerController::_handle_up_and_dn(uint8_t param_up, uint8_t param_dn) {
    // 不使用电源提供的抬刀参数 (未破解电源的抬刀信号)
    // 全部设定为0 (原先为: 高4位dn, 低4位up)
    ELEBUFFER[1][0] = 0x00;
}

void PowerController::_handle_ip(uint16_t param_ip) {
    uint16_t ip = param_ip;
    uint8_t ip_decimal_part = ip % 10;                    // ip 的 小数部分
    uint8_t ip_real_part = static_cast<uint8_t>(ip / 10); // ip 的整数部分

    ELEBUFFER[1][1] = ip_real_part;

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
        OR_EQUAL(ELEBUFFER[1][1], 0x80);
    } else {
        // ip 不含 0.1A
        AND_EQUAL(ELEBUFFER[1][1], 0x7F);
    }

    // 清空 ELEBUFFER[1][2]的低4位
    AND_EQUAL(ELEBUFFER[1][2], 0xF0);

    if (ip_decimal_part == 2 || ip_decimal_part == 3 || ip_decimal_part == 7 ||
        ip_decimal_part == 8) {
        // ip 含有 0.2A
        OR_EQUAL(ELEBUFFER[1][2], 0x01);
    } else {
        // ip 不含 0.2A
        AND_EQUAL(ELEBUFFER[1][2], 0xFE);
    }

    if (ip_decimal_part == 5 || ip_decimal_part == 6 || ip_decimal_part == 7 ||
        ip_decimal_part == 8) {
        // ip 含有 0.5A
        OR_EQUAL(ELEBUFFER[1][2], 0x02);
    } else {
        // ip 不含 0.5A
        AND_EQUAL(ELEBUFFER[1][2], 0xFD);
    }
}

void PowerController::_handle_hp(uint8_t param_hp) {
    uint8_t hp = param_hp;
    uint8_t hp_units = hp % 10;            // 个位
    uint8_t hp_tens = (hp / 10) % 10;      // 十位
    uint8_t hp_hundreds = (hp / 100) % 10; // 百位

    // 清空 ELEBUFFER[1][2]的高4位
    AND_EQUAL(ELEBUFFER[1][2], 0x0F);
    OR_EQUAL(ELEBUFFER[1][2], hp_units << 4); // hp个位

    if (hp_tens == 2 || hp_tens == 3 || hp_tens == 6 || hp_tens == 7) {
        OR_EQUAL(ELEBUFFER[1][6], 0x01); // CPS bit0 设为1
    } else {
        AND_EQUAL(ELEBUFFER[1][6], 0xFE);
    }
}

void PowerController::_handle_ma(uint8_t param_ma) {
    AND_EQUAL(ELEBUFFER[1][3], 0xF0);    // 清空低4位
    OR_EQUAL(ELEBUFFER[1][3], param_ma); // 低4位设定ma
}

void PowerController::_handle_sv(uint8_t param_sv) {
    // sv 伺服电压 (应当由伺服采样模块(目前为IO采样板)处理, 不需要发送给电源)

    // TODO 要将sv值通过can发送给采样板

    AND_EQUAL(ELEBUFFER[1][3], 0x0F);         // 清空高4位
    OR_EQUAL(ELEBUFFER[1][3], param_sv << 4); // 高4位设定sv
}

void PowerController::_handle_al(uint8_t param_al) {
    ELEBUFFER[1][4] = param_al;
}

void PowerController::_handle_oc(uint8_t param_oc) {
    AND_EQUAL(ELEBUFFER[1][5], 0x0F);         // 清空高4位
    OR_EQUAL(ELEBUFFER[1][5], param_oc << 4); // 高4位设定oc
}

void PowerController::_handle_pp(uint8_t param_pp) {
    uint8_t pp = param_pp;
    if (highpower_on_flag_ == 0) {
        pp = 0; // 高频未使能
    }

    if (pp & 0x01) {
        // TODO
        // 如果pp位*1 (?什么意思), 设置NWS bit4 为1
        OR_EQUAL(ELEBUFFER[1][6], 0x10);
    } else {
        AND_EQUAL(ELEBUFFER[1][6], 0xEF);
    }
}

void PowerController::_handle_machbit() {
    if (highpower_on_flag_ && machpower_flag_) {
        // 高频允许 且 高压允许(非抬刀)
        OR_EQUAL(ELEBUFFER[1][6], 0x20); // 允许高压
    } else {
        AND_EQUAL(ELEBUFFER[1][6], 0xDF); // 切断高压
    }
}

void PowerController::_fill_buffer_io_value(
    const EleParam_dkd_t &new_eleparam) {
    _handle_contactors_NOW(new_eleparam);
    _handle_contactors_MON_HON(new_eleparam);
    _handle_contactors_IP0(new_eleparam.ip);
    _handle_contactors_LVx(new_eleparam.lv);
    _handle_contactors_PL(new_eleparam.pl);
    _handle_contactors_MACH();
    _handle_contactors_CAP(new_eleparam.c);

    // 处理特殊电参数,覆盖, 最后调用
    _handle_special_ele_index(new_eleparam.upper_index);
}

void PowerController::_handle_contactors_NOW(
    const EleParam_dkd_t &new_eleparam) {
    uint8_t pp_tens = (new_eleparam.pp / 10) % 10;
    uint8_t hp_tens = (new_eleparam.hp / 10) % 10;

    if (pp_tens != 1) {
        // pp 十位不为1, NOW 断开 (? 特殊情况?) // TODO
        curr_elesettings_buffer_.set_contactors_io(DEC_NOW_CONTACTORS,
                                                   CONTACTOR_DISABLE);
        return;
    }

    // pp 十位为1, 继续判断其他

    if (highpower_on_flag_) {
        // 高频开启
        if (new_eleparam.ip >= 80 || hp_tens < 4) {
            // ip >= 8 或 hp十位为0,1,2,3
            // NOW 断开
            curr_elesettings_buffer_.set_contactors_io(DEC_NOW_CONTACTORS,
                                                       CONTACTOR_DISABLE);
        } else {
            // ip < 8 且 hp十位>=4
            // NOW 吸合
            curr_elesettings_buffer_.set_contactors_io(DEC_NOW_CONTACTORS,
                                                       CONTACTOR_ENABLE);
        }
    } else {
        // 高频关闭
        // NOW 吸合
        curr_elesettings_buffer_.set_contactors_io(DEC_NOW_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    }
}

void PowerController::_handle_contactors_MON_HON(
    const EleParam_dkd_t &new_eleparam) {
    uint8_t pp_tens = (new_eleparam.pp / 10) % 10;
    uint8_t hp_tens = (new_eleparam.hp / 10) % 10;

    if (pp_tens != 1 || hp_tens >= 8) {
        return;
    }

    // pp_tens == 1 && hp_tens < 8

    if (highpower_on_flag_) {
        // 高频打开
        if (hp_tens % 2 == 0) {
            // hp = *0* *2* *4* *6*
            // MON 吸合 HON 断开
            curr_elesettings_buffer_.set_contactors_io(DEC_MON_CONTACTORS,
                                                       CONTACTOR_ENABLE);
            curr_elesettings_buffer_.set_contactors_io(DEC_HON_CONTACTORS,
                                                       CONTACTOR_DISABLE);
        } else {
            // hp = *1* *3* *5* *7*
            // MON 断开 HON 吸合
            curr_elesettings_buffer_.set_contactors_io(DEC_MON_CONTACTORS,
                                                       CONTACTOR_DISABLE);
            curr_elesettings_buffer_.set_contactors_io(DEC_HON_CONTACTORS,
                                                       CONTACTOR_ENABLE);
        }
    } else {
        // 高频关闭
        // MON 断开 HON 断开
        curr_elesettings_buffer_.set_contactors_io(DEC_MON_CONTACTORS,
                                                   CONTACTOR_DISABLE);
        curr_elesettings_buffer_.set_contactors_io(DEC_HON_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    }
}

void PowerController::_handle_contactors_IP0(uint16_t ip) {
    if (ip > 0 && highpower_on_flag_) {
        // ip > 0 时, IP0 断开, 低压回路上电
        curr_elesettings_buffer_.set_contactors_io(DEC_IP0_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    } else {
        curr_elesettings_buffer_.set_contactors_io(DEC_IP0_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    }
}

void PowerController::_handle_contactors_LVx(uint8_t lv) {
    // 只操作LV1 LV2继电器, 2个只能吸合1个
    if (!highpower_on_flag_) {
        // 都断开
        curr_elesettings_buffer_.set_contactors_io(DEC_LV1_CONTACTORS,
                                                   CONTACTOR_DISABLE);
        curr_elesettings_buffer_.set_contactors_io(DEC_LV2_CONTACTORS,
                                                   CONTACTOR_DISABLE);
        return;
    }

    if (lv == 1) {
        // LV1 吸合, LV2 断开
        curr_elesettings_buffer_.set_contactors_io(DEC_LV1_CONTACTORS,
                                                   CONTACTOR_ENABLE);
        curr_elesettings_buffer_.set_contactors_io(DEC_LV2_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    } else if (lv == 2) {
        // LV1 断开, LV2 吸合
        curr_elesettings_buffer_.set_contactors_io(DEC_LV1_CONTACTORS,
                                                   CONTACTOR_DISABLE);
        curr_elesettings_buffer_.set_contactors_io(DEC_LV2_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    } else {
        // lv = 0和其它
        // 都断开
        curr_elesettings_buffer_.set_contactors_io(DEC_LV1_CONTACTORS,
                                                   CONTACTOR_DISABLE);
        curr_elesettings_buffer_.set_contactors_io(DEC_LV2_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    }
}

void PowerController::_handle_contactors_PL(uint8_t pl) {
    if (pl == 0) {
        // 负极性
        curr_elesettings_buffer_.set_contactors_io(DEC_POSITIVE_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    } else {
        // 正极性
        curr_elesettings_buffer_.set_contactors_io(DEC_POSITIVE_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    }
}

void PowerController::_handle_contactors_MACH() {
    if (highpower_on_flag_) {
        // 高频启动, MACH 吸合
        curr_elesettings_buffer_.set_contactors_io(DEC_MACH_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    } else {
        // 高频关闭, MACH 断开
        curr_elesettings_buffer_.set_contactors_io(DEC_MACH_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    }
}

void PowerController::_handle_contactors_CAP(uint8_t c) {
    // switch (c) {
    // case 1:
        
    // }
}

void PowerController::_handle_special_ele_index(uint16_t up_index) {
    /* C901 C902 条件号时, PK继电器吸合, 其他时候不吸合 */
    if (up_index == 901 || up_index == 902) {
        curr_elesettings_buffer_.set_contactors_io(DEC_PK_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    } else {
        curr_elesettings_buffer_.set_contactors_io(DEC_PK_CONTACTORS,
                                                   CONTACTOR_DISABLE);
    }

    /* C901 C902 条件号时, NOW继电器吸合【不受HP控制, 也不受IP>=8控制】,
     * 其他时候不覆盖吸合设置 */
    if (up_index == 901 || up_index == 902) {
        curr_elesettings_buffer_.set_contactors_io(DEC_NOW_CONTACTORS,
                                                   CONTACTOR_ENABLE);
    } // else 其他情况时不设置, 不然会覆盖IP、HP的控制
}

void PowerController::_calc_buffer_pulse_and_crc() {
    // 处理心跳
    curr_elesettings_buffer_.set_pulse_value(canframe_pulse_value_);
    ++canframe_pulse_value_;

    // 计算校验
    curr_elesettings_buffer_.calc_buffer_crc();
}

} // namespace power

} // namespace edm
