#pragma once

#include "Motion/MoveDefines.h"
#include "MotionAutoTask.h"
#include <optional>

namespace edm {
namespace move {

class DrillAutoTask : public AutoTask {
public:
    struct StartParams {
        double depth_blu;
        int holdtime_ms;
        bool touch;
        bool breakout;
        std::optional<double> spindle_speed_blu_ms_opt;
    };

public:
    DrillAutoTask(const StartParams &start_params, const MotionCallbacks &cbs);

private:
    void _all_pump_and_spindle_on(bool on);
    void _mach_on(bool on);

private:
    MotionCallbacks cbs_;

private:
    struct _Runtime_t {
        // (S轴, 下同)
        double touch_start_pos; // 碰边开始位置
        double touched_pos;     // 碰边触发位置

        // 加工开始位置(如果是M66M70, 该位置是touched_pos向上抬回退距离)
        double mach_start_pos;
        // 加工目标位置(如果是M66M70, 该位置会在碰边后设定)
        double mach_target_pos;

        uint32_t delay_end_time_ms; // 结束延时时间(绝对时钟)

        // 穿透检测用的记录
        uint8_t start_breakthrough_detect;
        uint8_t detect_state; // 0 未检测到; 1 检测到穿透开始; 2
                              // 持续原地放电等待时间
        double start_detect_pos;

        // 检测到穿透开始的位置(用于限制穿透开始后最大移动距离)
        double breakout_start_detected_pos;
        // 检测到穿透开始后, S轴可以到达的最小的位置, 提前计算好
        double min_pos_after_breakout_start_detected;
        // 穿透检测判定完成时的时间, 用于在完成穿透后原地加工一小段时间
        uint32_t breakout_end_judged_time_ms;

        void clear() {
            touch_start_pos = 0;
            touched_pos = 0;
            mach_start_pos = 0;
            mach_target_pos = 0;
            delay_end_time_ms = 0;
            start_breakthrough_detect = 0;
            detect_state = 0;
            start_detect_pos = 0;
            breakout_start_detected_pos = 0;
            min_pos_after_breakout_start_detected = 0;
            breakout_end_judged_time_ms = 0;
        }
    } runtime_;
};

} // namespace move
} // namespace edm