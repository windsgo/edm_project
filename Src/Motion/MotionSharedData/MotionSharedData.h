#pragma once

#include "CanReceiveBuffer/CanReceiveBuffer.h"
#include "Motion/MoveDefines.h"

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
    // Can 接收与缓存相关
    inline void set_can_recv_buffer(CanReceiveBuffer::ptr can_recv_buffer) {
        can_recv_buffer_ = can_recv_buffer;
    }

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

private:
    CanReceiveBuffer::ptr can_recv_buffer_;
    Can1IOBoard407ServoData
        cached_servo_data_; // 状态机每次开始时从can_recv_buffer_缓存一次
    Can1IOBoard407ADCInfo cached_adc_info_;

private:
    MotionSharedData() = default;
    MotionSharedData(const MotionSharedData &) = delete;
    MotionSharedData(MotionSharedData &&) = delete;
    MotionSharedData &operator=(const MotionSharedData &) = delete;
    MotionSharedData &operator=(MotionSharedData &&) = delete;
};

} // namespace move

} // namespace edm
