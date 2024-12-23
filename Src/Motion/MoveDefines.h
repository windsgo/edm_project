#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include <optional>

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

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    MotionInfoBitState1_BreakoutDetected = 1 << 5,
    MotionInfoBitState1_KnDetected = 1 << 6,
#endif
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

    int sub_line_number {-1}; // 子行号, 用于G01GroupMotionCommand

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    unit_t spindle_axis_blu{0.0};
    bool is_spindle_on{false};

    unit_t drill_total_blu{0.0};     // 打孔总深度
    unit_t drill_remaining_blu{0.0}; // 打孔剩余深度

    struct BreakoutData {
        int realtime_voltage{0};
        int averaged_voltage{0};
        double kn{0};
        double kn_valid_rate{0};
        int kn_cnt{0};
    } breakout_data;
#endif

    MotionMainMode main_mode{MotionMainMode::Idle};       // 当前主模式
    MotionAutoState auto_state{MotionAutoState::Stopped}; // auto模式下的state

    struct LatencyData {
        int curr_latency{};
        // int min_latency{}; // 最小值没必要记录
        int max_latency{};
        int avg_latency{};
        int warning_count{};
    } latency_data;

    struct TimeUseData {
        int total_time_use_avg{};
        int total_time_use_max{};

        int info_time_use_avg{};
        int info_time_use_max{};

        int ecat_time_use_avg{};
        int ecat_time_use_max{};

        int statemachine_time_use_avg{};
        int statemachine_time_use_max{};
    } time_use_data;

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
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    XX_(1, BreakoutDetected)
    XX_(1, KnDetected)
#endif 

#undef XX_
};

enum MotionSignalType {
    MotionSignal_ManualPointMoveStarted,
    MotionSignal_ManualPointMoveStopped,

    MotionSignal_AutoStarted,
    MotionSignal_AutoPaused,
    MotionSignal_AutoResumed,
    MotionSignal_AutoStopped,

    MotionSignal_AutoNotify,

    // TODO

    MotionSignal_MAX
};

struct MotionSignal {
    MotionSignalType type;
    MotionInfo info;
};

// 记录器记录数据
struct RecordMotionData1 {
    axis_t cmd;
    axis_t act;

    unit_t servo_cmd;
};

struct MotionCallbacks {
    std::function<void(bool)> cb_enable_voltage_gate;
    std::function<void(bool)> cb_mach_on;

    std::function<void(void)> cb_trigger_bz_once;

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    std::function<void(bool)> cb_opump_on;
    std::function<void(bool)> cb_ipump_on;
#endif
};

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
struct DrillBreakOutParams {
    uint32_t voltage_average_filter_window_size{200};
    uint32_t stderr_filter_window_size{200};
    double kn_valid_threshold{20};
    uint32_t kn_sc_window_size{300};
    double kn_valid_rate_threshold{0.01};
    uint32_t kn_valid_rate_ok_cnt_threshold{320};
    uint32_t kn_valid_rate_ok_cnt_maximum{600};

    double max_move_um_after_breakout_start_detected{200};
    double breakout_start_detect_length_percent{0.25};
    double speed_rate_after_breakout_start_detected{0.8};
    uint32_t wait_time_ms_after_breakout_end_judged{1000};

    uint32_t ctrl_flags{0};
};

struct DrillParams {
    double touch_return_um{500}; // 碰边后返回距离
    double touch_speed_um_ms{2}; // 碰边速度

    bool auto_switch_opump{false};
    bool auto_switch_ipump{false};

    DrillBreakOutParams breakout_params; // 穿透参数
};

struct DrillStartParams {
    double depth_um;
    int holdtime_ms;
    bool touch;
    bool breakout;
    bool back;
    std::optional<double> spindle_speed_blu_ms_opt;
};
#endif


struct G01GroupItem {
    move::axis_t incs;
    int line{-1};
};

struct G01GroupStartParam {
    std::vector<G01GroupItem> items;
};

} // namespace move

} // namespace edm
