#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "config.h"

namespace edm {

namespace move {

// 一些常量值, 好于使用宏定义, 保证了数据类型
class ConstantValues final {
public:
    constexpr static const double blu_per_um = (double)EDM_BLU_PER_UM;
    constexpr static const double blu_per_mm = blu_per_um * 1000.0;
    constexpr static const double um_per_blu = (double)1 / blu_per_um;
};

using unit_t = double;

using axis_t = std::array<unit_t, EDM_AXIS_NUM>;

enum class MotionMainMode {
    Idle,
    Manual, // 点动
    Auto,   // GCode任务
    // TODO
};

enum class MotionAutoState {
    NormalMoving,
    Pausing,
    Paused,
    Resuming,
    Stopping,
    Stopped,
};

enum MotionInfoBitState1 {
    MotionInfoBitState1_EcatConnected = 1 << 0, // 驱动器已连接, 否则为未连接
    MotionInfoBitState1_EcatAllEnabled = 1 << 1, // 驱动器全部使能, 否则为有错

    MotionInfoBitState1_TouchDetectEnabled =
        1 << 2, // 接触感知检测使能 //! 应该用不到此状态, 因为现在都是ST机制
    MotionInfoBitState1_TouchDetected = 1 << 3, // 接触感知检测到（物理状态）
    MotionInfoBitState1_TouchWarning = 1 << 4, // 接触感知报警(需要清错)
};

struct MotionInfo {
    //! 坐标值都是驱动器(绝对坐标系)下的值, 单位为blu
    //! 即这里的值与驱动器的值是对应的. e.g:
    //! 若um_per_blu=0.1,
    //! curr_cmd_axis[0] = 100.0 意味着 当前指令位置为100.0blu = 100.0blu *
    //! um_per_blu = 10.0um
    axis_t curr_cmd_axis_blu{0.0}; // 当前周期计算指令位置(计算坐标) //!
                                   // 目前等于设置到驱动器的坐标,
                                   // 但如果后续加入坐标处理等算法, 就不一定了
    axis_t curr_act_axis_blu{0.0}; // 当前周期编码器实际位置

    MotionMainMode main_mode{MotionMainMode::Idle};       // 当前主模式
    MotionAutoState auto_state{MotionAutoState::Stopped}; // auto模式下的state

    struct LatencyData {
        int curr_latency{};
        int min_latency{};
        int max_latency{};
        int avg_latency{};
    } latency_data;

    //! see `enum MotionInfoBitState1`
    uint32_t bit_state1{0};

public: // 便捷接口
#define XX_(n__, interface__)                                                \
    inline bool interface__() const {                                        \
        return !!(bit_state##n__ & MotionInfoBitState##n__##_##interface__); \
    }                                                                        \
    inline void set##interface__(bool v) {                                   \
        if (v) {                                                             \
            bit_state##n__ |= MotionInfoBitState##n__##_##interface__;       \
        } else {                                                             \
            bit_state##n__ &= ~MotionInfoBitState##n__##_##interface__;      \
        }                                                                    \
    }

    XX_(1, EcatConnected)
    XX_(1, EcatAllEnabled)
    XX_(1, TouchDetectEnabled)
    XX_(1, TouchDetected)
    XX_(1, TouchWarning)

#undef XX_
};

enum MotionSignalType {
    MotionSignal_ManualPointMoveStarted,
    MotionSignal_ManualPointMoveStopped,

    MotionSignal_AutoStarted,
    MotionSignal_AutoPaused,
    MotionSignal_AutoResumed,
    MotionSignal_AutoStopped,

    // TODO

    MotionSignal_MAX
};

struct MotionSignal {
    MotionSignalType type;
    MotionInfo info;
};

} // namespace move

} // namespace edm
