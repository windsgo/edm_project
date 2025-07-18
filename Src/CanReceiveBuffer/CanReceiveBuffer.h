#pragma once

#include <atomic>
#include <memory>
#include <functional>
#include <random>

#include "config.h"

#include "Motion/MoveDefines.h"

#include "QtDependComponents/PowerController/PowerController.h"
#include "QtDependComponents/PowerController/EleparamDefine.h"

namespace edm
{

#ifdef EDM_IOBOARD_NEW_SERVODATA_1MS
struct __attribute__((__packed__)) Can1IOBoard407ServoData {
    // index 0~1:
    uint8_t touch_detected : 1;     // 接触感知, 1表示有接触
    uint8_t servo_direction : 1;    // servo_direction 伺服方向, 1:进给 0:后退
    uint16_t servo_distance_0_001um : 14;     // servo_distance 伺服距离 单位0.01um, 即需要除以100后才是um
    
    // index 2~3
    uint16_t average_voltage : 9;  // V
    uint8_t current : 7; // 0.5A (current = 2 means 1A)
    
    // index 4: 
    uint8_t open_rate : 8;          // 开路率
    
    // index 5: 
    uint8_t normal_rate : 8;        // 正常加工率
    
    // index 6: 
    uint8_t arc_rate : 8;           // 拉弧率
    
    // index 7: 
    uint8_t short_rate : 8;         // 短路率 (index 4 ~ 7 应该加起来为100)
};
static_assert(sizeof(Can1IOBoard407ServoData) == 8);
#else // EDM_IOBOARD_NEW_SERVODATA_1MS
/**
 * @brief 伺服板(现407)返回的ServoData, RxID = 0x0231
 * @author lyf
 * @date 2023.6.13
 * @note 名字中加407以防止与其他名称冲突，前缀s_表示静态全局变量，编程规范
 */
struct __attribute__((__packed__)) Can1IOBoard407ServoData {
    // index 0~1:
    uint16_t servo_distance_0_001um : 15;     // servo_distance 伺服距离 单位0.01um, 即需要除以100后才是um
    uint8_t servo_direction : 1;    // servo_direction 伺服方向, 1:进给 0:后退
    
    // index 2
    uint8_t average_voltage : 8;    // average_voltage (uint8_t)
    
    // index 3 ~ index 4
    uint16_t jump_height : 16;      // jump_distance (uint16_t) 抬刀距离
    
    // index 5
    uint8_t touch_detected : 1;     // 接触感知, 1表示有接触
    uint8_t _reserved_7 : 7;
    
    // index 6 ~ 7
    uint8_t _reserved[2];           // reserved
};
#endif // EDM_IOBOARD_NEW_SERVODATA_1MS


/**
 * @brief 伺服板(现407)返回的ADCInfo, RxID = 0x0232
 * @author lyf
 * @date 2023.6.13
 * @note 名字中加407以防止与其他名称冲突，前缀s_表示静态全局变量，编程规范
 */
struct __attribute__((__packed__)) Can1IOBoard407ADCInfo {
    // index 0: 
    uint8_t open_rate : 8;          // 开路率
    
    // index 1: 
    uint8_t normal_rate : 8;        // 正常加工率
    
    // index 2: 
    uint8_t arc_rate : 8;           // 拉弧率
    
    // index 3: 
    uint8_t short_rate : 8;         // 短路率 (index 0 ~ 3 应该加起来为100)
    
    // index 4~5
    uint16_t new_voltage : 9;       // 电压
    uint8_t new_current : 7;        // 电流（乘以2发送）
    
    // index 6 ~ 7:
    uint8_t _reserved[2];           //  reserved
};

struct dummy_8bytes {
    uint8_t dummy[8];
};
static_assert(sizeof(dummy_8bytes) == 8);

// 负责servodata 和 adcinfo 的缓存原子量维护
// 线程安全, 函数可重入
class CanReceiveBuffer final {
public:
    using ptr = std::shared_ptr<CanReceiveBuffer>;
    CanReceiveBuffer(can::CanController::ptr can_ctrler, uint32_t can_device_index);

public:
    void load_servo_data(Can1IOBoard407ServoData& servo_data) const;

    inline void load_adc_info(Can1IOBoard407ADCInfo& adc_info) const { 
        *reinterpret_cast<dummy_8bytes *>(&adc_info) = at_adc_info_.load(); 
    }

    inline bool is_servo_data_new() const {
#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
        return true;
#else // EDM_OFFLINE_MANUAL_SERVO_CMD
        return at_servo_data_is_new_;
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD
    }
    inline void clear_servo_data_new_flag() { at_servo_data_is_new_ = false; }

private:
    void _listen_cb(const QCanBusFrame& frame);

private:
    // 用8字节长度的一块内存来存储, 读取时先load拷贝出, 再强制转换
    std::atomic<dummy_8bytes> at_servo_data_;
    std::atomic<dummy_8bytes> at_adc_info_;

    std::atomic_bool at_servo_data_is_new_ {false};

private: // 用于离线测试时的随机数发生器
#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    // 离线测试时, 给此类输入一个幅值和进给概率, 每次调用, 随机给出一个进给量
    std::atomic<double> manual_servo_cmd_feed_probability_ {0.75}; // 进给概率
    std::atomic<double> manual_servo_cmd_feed_amplitude_um_ {0.5}; // 幅值
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

    mutable std::random_device random_device_;
    mutable std::mt19937 gen_;
    mutable std::uniform_real_distribution<> uniform_real_distribution_;

#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    std::atomic_bool manual_touch_detect_flag_{false};
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

public:
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    inline void set_manual_touch_detect_flag(bool detected) {
        manual_touch_detect_flag_ = detected;
    }
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT

#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
    inline void set_manual_servo_cmd(double feed_probability, double feed_amplitude_um) {
        manual_servo_cmd_feed_probability_ = feed_probability;
        manual_servo_cmd_feed_amplitude_um_ = feed_amplitude_um;
    }
#endif // EDM_OFFLINE_MANUAL_SERVO_CMD

#ifdef EDM_OFFLINE_MANUAL_VOLTAGE
    // 离线手动设置电压
private:
    std::atomic_uint16_t manual_voltage_value_ {10};
public:
    inline void set_manual_voltage(uint16_t voltage) { manual_voltage_value_.store(voltage); }
#endif // EDM_OFFLINE_MANUAL_VOLTAGE

private:
    constexpr static const uint32_t servodata_rxid_ {EDM_CAN_RXID_IOBOARD_SERVODATA};
    constexpr static const uint32_t adcinfo_rxid_ {EDM_CAN_RXID_IOBOARD_ADCINFO};
    
};

} // namespace edm
