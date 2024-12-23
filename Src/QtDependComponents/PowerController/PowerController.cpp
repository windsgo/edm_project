#include "PowerController.h"

#include "Logger/LogMacro.h"
#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"

#include "QtDependComponents/PowerController/EleparamDecoder.h"
#include "QtDependComponents/ZynqConnection/TcpMessageDefine.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
#include "Utils/Format/edm_format.h"
#include <chrono>
#include <cstdint>
#include <cstring>

namespace edm {

namespace power {

// static auto s_can_ctrler = can::CanController::instance();
// static auto s_io_ctrler = io::IOController::instance();

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

// 给访问缓冲区与bytearray快捷操作宏
#define ELEBUFFER                    curr_elesettings_buffer_.canframe_buffer_
#define OR_EQUAL(__ori, __or_value)  (__ori) = (__ori) | (__or_value)
#define AND_EQUAL(__ori, __or_value) (__ori) = (__ori) & (__or_value)

PowerController::PowerController(can::CanController::ptr can_ctrler,
                                 io::IOController::ptr io_ctrler,
#ifdef EDM_USE_ZYNQ_SERVOBOARD
                                 zynq::ZynqConnectController::ptr zynq_ctrler,
#endif
                                 int can_device_index)
    : can_ctrler_(can_ctrler), io_ctrler_{io_ctrler},
#ifdef EDM_USE_ZYNQ_SERVOBOARD
      zynq_ctrler_(zynq_ctrler),
#endif
      can_device_index_(can_device_index) {
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    memset(&servo_setting_, 0, sizeof(servo_setting_));
    memset(&ioboard_eleparam_, 0, sizeof(ioboard_eleparam_));
#endif

    memset(&curr_eleparam_, 0, sizeof(curr_eleparam_));
}

void PowerController::set_highpower_on(bool on) {
    highpower_on_flag_ = on;
    update_eleparam_and_send(); // 要把mach继电器开这个信息计算出来发到io,
                                // 以及machbit发到电源
}

bool PowerController::is_highpower_on() const { return highpower_on_flag_; }

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
void PowerController::set_machbit_on(bool on) {
    machpower_flag_ = on;
    update_eleparam_and_send(); // machbit发到电源
}

bool PowerController::is_machbit_on() const { return machpower_flag_; }

void PowerController::set_power_on(bool on) {
    static const uint32_t pwon_io_1 = 1 << (EleContactorOut_PWON_JF1 - 1);
    static const uint32_t sof_io_2 = 1 << (EleContactorOut_SOF - 33);

    if (on) {
        io_ctrler_->set_can_machineio_1_withmask(pwon_io_1, pwon_io_1);
        io_ctrler_->set_can_machineio_2_withmask(sof_io_2, sof_io_2);
    } else {
        // 通过设置mask并设置io0来关闭对应io
        io_ctrler_->set_can_machineio_1_withmask(0x00, pwon_io_1);
        io_ctrler_->set_can_machineio_2_withmask(0x00, sof_io_2);
    }
}

bool PowerController::is_power_on() const {
    static const uint32_t pwon_io_1 = 1 << (EleContactorOut_PWON_JF1 - 1);
    static const uint32_t sof_io_2 = 1 << (EleContactorOut_SOF - 33);

    // s_logger->debug("pwon_io_1: {:08X}", pwon_io_1);
    // s_logger->debug("s_io_ctrler->get_can_machineio_1_safe() : {:08X}",
    //                 s_io_ctrler->get_can_machineio_1_safe());
    return !!(io_ctrler_->get_can_machineio_1_safe() & pwon_io_1);
}

void PowerController::set_finishing_cut_flag(bool on) {
    finishing_cut_flag_ = static_cast<uint8_t>(on);
}

bool PowerController::is_finishing_cut_flag_on() const {
    return finishing_cut_flag_;
}

void PowerController::_trigger_send_canbuffer() {
    QCanBusFrame frame1{POWERCAN_TXID, curr_result_->can_buffer()[0]};
    can_ctrler_->send_frame(can_device_index_, frame1);

    QCanBusFrame frame2{POWERCAN_TXID, curr_result_->can_buffer()[1]};
    can_ctrler_->send_frame(can_device_index_, frame2);
}
#endif

void PowerController::_trigger_send_io_value() {
    if (!curr_result_)
        return;

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    io_ctrler_->set_can_machineio_1_withmask(
        curr_result_->io_1(), EleparamDecodeResult::get_io_1_mask());
    io_ctrler_->set_can_machineio_2_withmask(
        curr_result_->io_2(), EleparamDecodeResult::get_io_2_mask());
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    io_ctrler_->set_can_machineio_output_withmask(
        curr_result_->io(), EleparamDecodeResult::get_io_mask());
#endif

    // 让IO控制器强制发送一次IO
    io_ctrler_->trigger_send_current_io();
}

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
void PowerController::_trigger_send_ioboard_eleparam() {
    if (!eleparam_inited_)
        return;

    // 赋值
    ioboard_eleparam_.ip = static_cast<uint8_t>(curr_eleparam_.ip);
    ioboard_eleparam_.on = curr_eleparam_.pulse_on;
    ioboard_eleparam_.off = curr_eleparam_.pulse_off;
    ioboard_eleparam_.pp = curr_eleparam_.pp;
    ioboard_eleparam_.is_finishing_cut =
        curr_eleparam_.upper_index == 901 ||
        curr_eleparam_.upper_index == 902; // TODO 精加工标志位

    // 装载
    QByteArray ba{reinterpret_cast<char *>(&ioboard_eleparam_), 8};
    QCanBusFrame frame(IOBOARD_ELEPARAMS_TXID, ba);

    // 发送
    can_ctrler_->send_frame(can_device_index_, frame);
}
#endif

void PowerController::_update_eleparam_and_send(
    const EleParam_dkd_t &eleparam) {
    // s_logger->trace("update_eleparam_and_send:");
    // auto strs = eleparam_to_string(eleparam);
    // for (const auto &s : strs) {
    //     s_logger->trace(s);
    // }

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    // 心跳处理
    _add_canframe_pulse_value();

    // 构造输入
    auto input = std::make_shared<EleparamDecodeInput>(
        eleparam, highpower_on_flag_, machpower_flag_, canframe_pulse_value_);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    auto input =
        std::make_shared<EleparamDecodeInput>(eleparam, highpower_on_flag_);
#endif

    // 获取decode输出
    curr_result_ = EleparamDecoder::decode(input);

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    // 发送can buffer
    _trigger_send_canbuffer();
#endif

    // 触发设置io
    _trigger_send_io_value();

    // 检查伺服参数设定, 如果变化, 重新发送
    _handle_servo_settings();
}

void PowerController::_handle_servo_settings() {
    if (!eleparam_inited_)
        return;

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    uint8_t bz_enable = _is_bz_enable();

    //! 防止CAN丢包导致伺服设定没发下去, 这里定时发
    //! (PowerManager会定时调用到这个函数)
    static int64_t last_send_time = 0;
    bool forced = false;
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    if (now - last_send_time > 5) {
        // 平均5s发一次
        last_send_time = now;
        forced = true;
    }

    if (curr_eleparam_.servo_sensitivity != servo_setting_.servo_sensitivity ||
        curr_eleparam_.servo_speed != servo_setting_.servo_speed ||
        curr_eleparam_.UpperThreshold != servo_setting_.servo_voltage_1 ||
        curr_eleparam_.LowerThreshold != servo_setting_.servo_voltage_2 ||
        curr_eleparam_.sv != servo_setting_.servo_sv ||
        bz_enable != servo_setting_.touch_zigbee_warning_enable || forced) {
        // 有变化

        // 全部重新赋值
        servo_setting_.servo_sensitivity = curr_eleparam_.servo_sensitivity;
        servo_setting_.servo_speed = curr_eleparam_.servo_speed;
        servo_setting_.servo_voltage_1 = curr_eleparam_.UpperThreshold;
        servo_setting_.servo_voltage_2 = curr_eleparam_.LowerThreshold;
        servo_setting_.servo_sv = curr_eleparam_.sv;
        servo_setting_.touch_zigbee_warning_enable = bz_enable;

        s_logger->trace(
            "send servo settings: ss: {}, s: {}, ut: {}, lt: {}, "
            "sv: {}, bz: {}",
            curr_eleparam_.servo_sensitivity, curr_eleparam_.servo_speed,
            curr_eleparam_.UpperThreshold, curr_eleparam_.LowerThreshold,
            curr_eleparam_.sv, bz_enable);

        // 装载 frame
        QByteArray ba{reinterpret_cast<char *>(&servo_setting_), 8};
        QCanBusFrame frame{EDM_CAN_TXID_IOBOARD_SERVOSETTING, ba};

        // 发送 frame
        can_ctrler_->send_frame(can_device_index_, frame);
    }
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // TODO

    static zynq::upper_servo_settings_t prev_servo_s;

    zynq::upper_servo_settings_t servo_s;

    servo_s.servo_speed = curr_eleparam_.servo_speed;
    servo_s.servo_ref_voltage = curr_eleparam_.sv;
    servo_s.servo_voltage_diff_level = curr_eleparam_.UpperThreshold;
    servo_s.servo_sensitivity = curr_eleparam_.servo_sensitivity;
    servo_s.servo_ref_voltage_low = curr_eleparam_.LowerThreshold;

    static int64_t last_send_time = 0;
    bool forced = false;
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    if (now - last_send_time > 5) {
        // 平均5s发一次
        last_send_time = now;
        forced = true;
    }

    if (forced ||
        memcmp(&prev_servo_s, &servo_s, sizeof(zynq::upper_servo_settings_t))) {
        auto pkg = zynq::ZynqConnectController::MakeTcpPackage(
            zynq::COMM_FRAME_SET_SERVO_SETTINGS, servo_s);

        zynq_ctrler_->send_tcp_bytearray(pkg);

        prev_servo_s = servo_s;
    }

#endif
}

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
bool PowerController::_is_bz_enable() {
    //! 要注意这里的嵌套加锁情况
    // TODO
    return is_power_on() && (!is_highpower_on());
}

void PowerController::_add_canframe_pulse_value() {
    ++canframe_pulse_value_;
    if (canframe_pulse_value_ > 0xC3FF) {
        canframe_pulse_value_ = 0x9C00;
    }
}
#endif

// PowerController *PowerController::instance() {
//     static PowerController instance;
//     return &instance;
// }

std::array<std::string, 3>
PowerController::eleparam_to_string(const EleParam_dkd_t &e) {
    static constexpr const char *ele_fmt1 =
        "ON: {}, OF: {}, IP: {}, HP: {}, PP: {}, LV: {}, C : {}, JM: {}";
    static constexpr const char *ele_fmt2 =
        "MA: {}, AL: {}, LD: {}, OC: {}, PL: {}, UP: {}, DN: {}, JS: {}";
    static constexpr const char *ele_fmt3 =
        "SV: {}, SS: {}, UT: {}, LT: {}, S : {}, 电参数: {}";

    // 摇动参数不打印, 这个系统不做摇动

    return {
        EDM_FMT::format(ele_fmt1, e.pulse_on, e.pulse_off, e.ip, e.hp, e.pp,
                        e.lv, e.c, e.jump_jm),
        EDM_FMT::format(ele_fmt2, e.ma, e.al, e.ld, e.oc, e.pl, e.up, e.dn,
                        e.jump_js),
        EDM_FMT::format(ele_fmt3, e.sv, e.servo_sensitivity, e.UpperThreshold,
                        e.LowerThreshold, e.servo_speed, e.upper_index),
    };
}

// void PowerController::init(int can_device_index) {
//     inited_ = true;
//     can_device_index_ = can_device_index;
// }

void PowerController::update_eleparam_and_send(
    const EleParam_dkd_t &new_eleparam) {
    // 存储当前结构体
    curr_eleparam_ = new_eleparam;
    if (!eleparam_inited_)
        eleparam_inited_ = true;

    _update_eleparam_and_send(curr_eleparam_);
}

void PowerController::update_eleparam_and_send(
    EleParam_dkd_t::ptr new_eleparam) {
    curr_eleparam_ = *new_eleparam;
    if (!eleparam_inited_)
        eleparam_inited_ = true;

    _update_eleparam_and_send(curr_eleparam_);
}

void PowerController::update_eleparam_and_send() {
    if (!eleparam_inited_) {
        return;
    }

    _update_eleparam_and_send(curr_eleparam_);
}

void PowerController::trigger_send_eleparam() {
    if (!curr_result_)
        return;

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    _add_canframe_pulse_value();
    curr_result_->set_pulse_count(canframe_pulse_value_);

    // 发送can buffer
    _trigger_send_canbuffer();
#endif

    // 检查伺服参数设定, 如果变化, 重新发送
    _handle_servo_settings();
}

void PowerController::trigger_send_contactors_io() {
    s_logger->trace("PowerController::trigger_send_contactors_io");
    // 触发设置io
    _trigger_send_io_value();
}

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
void PowerController::trigger_send_ioboard_eleparam() {
    _trigger_send_ioboard_eleparam();
}
#endif

} // namespace power

} // namespace edm
