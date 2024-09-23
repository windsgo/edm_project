#pragma once

#include <array>
#include <memory>
#include <mutex>

#include <QByteArray>

#include "EleparamDecoder.h"
#include "EleparamDefine.h"

#include "config.h"

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"

namespace edm {

namespace power {

class PowerController final {
public:
    // static PowerController *instance();

    static std::array<std::string, 3>
    eleparam_to_string(const EleParam_dkd_t &ele_param);

public:
    using ptr = std::shared_ptr<PowerController>;
    PowerController(can::CanController::ptr can_ctrler,
                    io::IOController::ptr io_ctrler,
#ifdef EDM_USE_ZYNQ_SERVOBOARD
                    zynq::ZynqConnectController::ptr zynq_ctrler,
#endif
                    int can_device_index);
    ~PowerController() noexcept = default;

    // void init(int can_device_index);

    // 更新电参数缓冲区, 设置标志位之后需要重新调用
    // 根据电参数结构体、高频等标志位，设定缓冲区报文、缓冲区io
    void update_eleparam_and_send(const EleParam_dkd_t &new_eleparam);
    void
    update_eleparam_and_send(EleParam_dkd_t::ptr new_eleparam); // deprecated
    void update_eleparam_and_send(); // no input, use current eleparam

    // 将缓冲区can报文进行心跳与校验设置，并发送出去
    void trigger_send_eleparam();

    // 将缓冲区io再发送一遍
    void trigger_send_contactors_io();

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    // 触发 IO 板循环心跳包, 内容是
    //! 需要外层至少5s内调用, IO板5s内收不到会报警
    void trigger_send_ioboard_eleparam();
#endif

    // 高频开关接口
    //! 注意, 外部设定高频开关等之后, 只是修改了标志位
    //! 还需要再调用update_eleparam()来刷新电参数, 并调用trigger函数发送
    void set_highpower_on(bool on);
    bool is_highpower_on() const;

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    void set_machbit_on(bool on);
    bool is_machbit_on() const;

    // Power ON 接口 设计 PWON, SOF 继电器, 单独设置, 与电参数刷新无关
    // 直接设置IO
    void set_power_on(bool on);
    bool is_power_on() const;

    // 精加工标志位设定
    void set_finishing_cut_flag(bool on);
    bool is_finishing_cut_flag_on() const;
#endif

    const auto &get_current_param() const { return curr_eleparam_; }

private:
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    void _trigger_send_canbuffer(); // 内部函数, 无锁, 不会操作心跳值
    void _trigger_send_ioboard_eleparam(); // 内部函数, 无锁
#endif
    void _trigger_send_io_value(); // 内部函数, 无锁

    void _update_eleparam_and_send(const EleParam_dkd_t &eleparam);

private:
    // 向伺服IO板发送 伺服参数 (如果发生变化)
    void _handle_servo_settings();

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    bool _is_bz_enable();
    void _add_canframe_pulse_value();
#endif

private:
    can::CanController::ptr can_ctrler_;
    io::IOController::ptr io_ctrler_;

#ifdef EDM_USE_ZYNQ_SERVOBOARD
    zynq::ZynqConnectController::ptr zynq_ctrler_;
#endif

    int can_device_index_ = -1; // used to send can frames by can::CanController

    mutable std::mutex mutex_;

    //! 标志位
    // 高频开关标志位(影响继电器, 切换加工时用)
    uint8_t highpower_on_flag_ = 0;

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    // 允许高频标志位(影响电源can帧标志位, 抬刀时切换电压用)
    uint8_t machpower_flag_ = 1;

    // 精加工标志位(发送给IO板标识)
    uint8_t finishing_cut_flag_ = 0;

    //! 心跳 16位 9C00 - C3FF
    // 电参数can帧心跳值
    uint16_t canframe_pulse_value_ = 0x9C00;
#endif

    //! decode 缓存
    // 存储当前的电参数 参数结构体
    bool eleparam_inited_{false};
    EleParam_dkd_t curr_eleparam_;

    // 存储当前的 decode 结果缓存
    EleparamDecodeResult::ptr curr_result_{nullptr};

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    //! io板发送缓存
    // 存储上一次发送的 伺服参数 结构体
    CanIOBoardServoSettingStrc servo_setting_;

    // 存储 io板心跳包
    CanIOBoardEleParamsStrc ioboard_eleparam_;
#endif

private:
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    constexpr static const int POWERCAN_TXID =
        EDM_CAN_TXID_POWER; // 电源参数CAN帧TxID
    constexpr static const int IOBOARD_SERVOSETTINGS_TXID =
        EDM_CAN_TXID_IOBOARD_SERVOSETTING; // IO板: 伺服参数设定 CAN帧TxID
    constexpr static const int IOBOARD_ELEPARAMS_TXID =
        EDM_CAN_TXID_IOBOARD_ELEPARAMS; // IO板: 心跳包,
                                        // 部分电参数与精加工标志位
#endif
};

} // namespace power

} // namespace edm
