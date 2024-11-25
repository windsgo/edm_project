#pragma once

#include "CanReceiveBuffer/CanReceiveBuffer.h"
#include "Motion/JumpDefines.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"

#include "QtDependComponents/ZynqConnection/UdpMessageDefine.h"
#include "SystemSettings/SystemSettings.h"
#include "Utils/DataQueueRecorder/DataQueueRecorder.h"

#include "EcatManager/EcatManager.h"
#include <array>
#include <cstddef>
#include <cstdint>

#include "QtDependComponents/ZynqConnection/ZynqUdpMessageHolder.h"

#include "DataRecordInstance1.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include "config.h"

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
#include "SpindleControl.h"
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

namespace edm {

namespace move {

// 设定参数, 外部通过MotionCmd通道传递整个Setting结构, 保存在MotionSharedData
// Motion组建可以实时读取
struct MotionSettings {
    bool enable_g01_run_each_servo_cmd{true}; // TODO 替代目前的一次读取方式
    bool enable_g01_half_closed_loop{true};
};

//! 用于Motion全局数据交换、记录, 一些关键的数据可以记录在这里
class MotionSharedData final {
public:
    static MotionSharedData *instance() {
        static MotionSharedData i;
        return &i;
    }

public:
    //! 必要的设定
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    inline void set_zynq_udpmessage_holder(
        zynq::ZynqUdpMessageHolder::ptr zynq_udpmessage_holder) {
        zynq_udpmessage_holder_ = zynq_udpmessage_holder;
    }
#else
    inline void set_can_recv_buffer(CanReceiveBuffer::ptr can_recv_buffer) {
        can_recv_buffer_ = can_recv_buffer;
    }
#endif

    const auto &gear_ratios() const { return gear_ratios_; }

    inline void set_ecat_manager(ecat::EcatManager::ptr ecat_manager) {
        ecat_manager_ = ecat_manager;
    }

public:
    inline auto get_ecat_manager() { return ecat_manager_; }

public:
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    inline const auto zynq_udpmessage_holder() const {
        return zynq_udpmessage_holder_;
    }
    inline const auto &cached_udp_message() const {
        return cached_udp_message_;
    }
    inline void update_zynq_udpmessage_holder() {
        if (zynq_udpmessage_holder_) [[likely]] {
            zynq_udpmessage_holder_->get_udp_message(cached_udp_message_);
        }
    }
#else
    // Can 接收与缓存相关
    inline auto &can_recv_buffer() {
        return can_recv_buffer_;
    } // non-const, 需要重置newflag
    inline const auto &cached_servo_data() const { return cached_servo_data_; }
    inline const auto &cached_adc_info() const { return cached_adc_info_; }

    inline void update_can_buffer_cache() {
        if (can_recv_buffer_) {
            can_recv_buffer_->load_servo_data(cached_servo_data_);
#ifndef EDM_IOBOARD_NEW_SERVODATA_1MS
            can_recv_buffer_->load_adc_info(cached_adc_info_);
#endif // EDM_IOBOARD_NEW_SERVODATA_1MS
        }
    }
#endif

public: // 数据记录相关, 一个周期内可能需要在不同地方记录多个数据,
        // 统一设置在这里, 周期结束时push入记录队列
    inline auto get_data_record_instance1() const {
        return data_record_instance1_;
    }

public:
    inline void add_thread_tick() {
        thread_tick_us_ += thread_cycle_us_;
        ++thread_tick_;
    }
    inline const auto get_thread_tick_us() const { return thread_tick_us_; }
    inline const auto get_thread_tick() const { return thread_tick_; }

    inline const auto &get_settings() const { return settings_; }
    inline void set_settings(const MotionSettings &settings) {
        settings_ = settings;
    }

    inline void set_jump_param(const JumpParam &jump_param) {
        jump_param_ = jump_param;
    }
    const auto &get_jump_param() const { return jump_param_; }

public:
    inline const auto &get_global_cmd_axis() const { return global_cmd_axis_; }
    void set_global_cmd_axis(const axis_t &cmd_axis);

    axis_t get_act_axis() const;
    bool get_act_axis(axis_t &axis) const;

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
public:
    auto get_spindle_controller() const { return spindle_control_; }
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

private:
    axis_t global_cmd_axis_; // 全局共享指令位置,
                             // 简化指令坐标在各级之间传递的逻辑, 防止指令突变

private: // Can 接收与缓存相关数据
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    zynq::ZynqUdpMessageHolder::ptr zynq_udpmessage_holder_;
    zynq::servo_return_converted_data_t cached_udp_message_;
#else
    CanReceiveBuffer::ptr can_recv_buffer_;
    Can1IOBoard407ServoData
        cached_servo_data_; // 状态机每次开始时从can_recv_buffer_缓存一次
    Can1IOBoard407ADCInfo cached_adc_info_;
#endif

private:
    DataRecordInstance1::ptr data_record_instance1_;

private:
    // 共享ecat manager, 便于获取数据和设定(如速度偏置控制)
    ecat::EcatManager::ptr ecat_manager_;

    // 设定
    MotionSettings settings_{};

    // 抬刀参数存储
    JumpParam jump_param_;

    // 共享的线程us计数器, 每次线程运行时都要加一下 thread_cycle_us_
    uint64_t thread_tick_us_{0}; // us
    uint64_t thread_tick_{0};    // 1 (周期计数)
    const uint32_t thread_cycle_us_ =
        SystemSettings::instance().get_motion_cycle_us();

private:
    // 电子齿轮比 (设定到驱动器用)
    std::array<double, EDM_SERVO_NUM> gear_ratios_;

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
private:
    SpindleControl::ptr spindle_control_;

    // 打孔和加工时自动开关主轴和水泵标志位
    bool auto_set_spindle_and_pump_{true};
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

private:
    MotionSharedData() {
        data_record_instance1_ = std::make_shared<DataRecordInstance1>(
            RecordData1BinDir, RecordData1DecodeDir);

        MotionUtils::ClearAxis(global_cmd_axis_);

        for (size_t i = 0; i < EDM_SERVO_NUM; ++i) {
            gear_ratios_[i] = 1.0;
        }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
        gear_ratios_[EDM_DRILL_S_AXIS_IDX] = 100.0; // S轴100倍

        spindle_control_ = std::make_shared<SpindleControl>();
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    }

    MotionSharedData(const MotionSharedData &) = delete;
    MotionSharedData(MotionSharedData &&) = delete;
    MotionSharedData &operator=(const MotionSharedData &) = delete;
    MotionSharedData &operator=(MotionSharedData &&) = delete;

public:
    const QString DataSaveRootDir =
        QString::fromStdString(SystemSettings::instance().get_datasave_dir());
    const QString RecordData1BinDir =
        DataSaveRootDir + "/MotionRecordData/Bin/";
    const QString RecordData1DecodeDir =
        DataSaveRootDir + "/MotionRecordData/Decode/";
};

} // namespace move

} // namespace edm
