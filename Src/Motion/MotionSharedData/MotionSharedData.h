#pragma once

#include "CanReceiveBuffer/CanReceiveBuffer.h"
#include "Motion/MoveDefines.h"

#include "Utils/DataQueueRecorder/DataQueueRecorder.h"

#include "EcatManager/EcatManager.h"

namespace edm {

namespace move {

//! 用于Motion全局数据交换、记录, 一些关键的数据可以记录在这里
class MotionSharedData final {
public:
    static MotionSharedData *instance() {
        static MotionSharedData i;
        return &i;
    }

public:
    //! 必要的设定
    inline void set_can_recv_buffer(CanReceiveBuffer::ptr can_recv_buffer) {
        can_recv_buffer_ = can_recv_buffer;
    }

    inline void set_ecat_manager(ecat::EcatManager::ptr ecat_manager) {
        ecat_manager_ = ecat_manager;
    }

public:
    inline auto get_ecat_manager() { return ecat_manager_; }

public:
    // Can 接收与缓存相关
    inline auto &can_recv_buffer() {
        return can_recv_buffer_;
    } // non-const, 需要重置newflag
    inline const auto &cached_servo_data() const { return cached_servo_data_; }
    inline const auto &cached_adc_info() const { return cached_adc_info_; }

    inline void update_can_buffer_cache() {
        if (can_recv_buffer_) {
            can_recv_buffer_->load_servo_data(cached_servo_data_);
            can_recv_buffer_->load_adc_info(cached_adc_info_);
        }
    }

public: // 数据记录相关, 一个周期内可能需要在不同地方记录多个数据,
        // 统一设置在这里, 周期结束时push入记录队列
    struct RecordData1 {
        // move::axis_t last_cmd_axis; // 周期开始时(上一周期的指令位置)

        // 周期结束时新的指令位置
        move::axis_t new_cmd_axis{0.0};

        // 周期开始时获取的实际位置(上一周期给出指令后的执行状态)
        move::axis_t act_axis{0.0};

        // 周期开始时获取驱动器返回的跟随误差值
        move::axis_t following_error_axis{0.0};

        // TODO 后续速度辅助控制, 速度值返回等记录

        // 是否为G01直线伺服加工(排除抬刀等状态)
        bool is_g01_normal_servoing{false};

        // 当前周期g01伺服指令值 (默认Z轴负方向加工)
        move::unit_t g01_servo_cmd{0.0};

        // 放电状态数据
        uint8_t normal_charge_rate {0};
        uint8_t short_charge_rate {0};
        uint8_t open_charge_rate {0};
        uint8_t current {0}; // 电流
        uint16_t average_voltage {0};

        inline void clear() {
            new_cmd_axis.fill(0.0);
            act_axis.fill(0.0);
            following_error_axis.fill(0.0);
            is_g01_normal_servoing = false;
            g01_servo_cmd = 0.0;

            normal_charge_rate = 0;
            short_charge_rate = 0;
            open_charge_rate = 0;
            current = 0;
            average_voltage = 0;
        }
    };

    inline void clear_data_record() { record_data1_cache_.clear(); }
    inline auto &get_record_data1_ref() { return record_data1_cache_; }

    // 判断记录器是否在运行(使能), 如果未使能可以不记录或push数据
    inline bool is_data_recorder_running() const {
        return record_data1_queuerecorder_->is_running();
    }

    // 将这一周期缓存的所有记录数据丢给记录器队列(线程)
    inline void push_data_to_recorder() {
        record_data1_queuerecorder_->push_data(record_data1_cache_);
    }

    // 获取记录器指针(上层获取, 记录器线程安全, 直接在上层开始、结束,
    // 运动线程只需要判断是否running即可)
    inline auto get_record_data1_queuerecorder() const {
        return record_data1_queuerecorder_;
    }

private: // Can 接收与缓存相关数据
    CanReceiveBuffer::ptr can_recv_buffer_;
    Can1IOBoard407ServoData
        cached_servo_data_; // 状态机每次开始时从can_recv_buffer_缓存一次
    Can1IOBoard407ADCInfo cached_adc_info_;

private:
    RecordData1 record_data1_cache_;
    util::DataQueueRecorder<RecordData1>::ptr record_data1_queuerecorder_;

private:
    // 共享ecat manager, 便于获取数据和设定(如速度偏置控制)
    ecat::EcatManager::ptr ecat_manager_;

private:
    MotionSharedData()
        : record_data1_queuerecorder_(
              std::make_shared<util::DataQueueRecorder<RecordData1>>()) {}
    MotionSharedData(const MotionSharedData &) = delete;
    MotionSharedData(MotionSharedData &&) = delete;
    MotionSharedData &operator=(const MotionSharedData &) = delete;
    MotionSharedData &operator=(MotionSharedData &&) = delete;
};

} // namespace move

} // namespace edm
