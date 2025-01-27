#pragma once

#include "Motion/MoveDefines.h"
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

#include "Utils/Filters/SlidingFilter/SlidingFilter.h"
#include "Utils/Filters/SlidingCounter/SlidingCounter.h"

namespace edm {
namespace util {

class BreakoutFilter {
public:
    using ptr = std::shared_ptr<BreakoutFilter>;
    BreakoutFilter();

    void set_realtime_voltage_average_filter_size(std::size_t size) {
        realtime_voltage_average_filter_->resize(size);
    }

    void set_averaged_voltage_stderr_filter_size(std::size_t size) {
        averaged_voltage_stderr_filter_->resize(size);
    }

    void set_stderr_threshold(double stderr_threshold) {
        stderr_threshold_ = stderr_threshold;
    }

    void set_kn_sliding_counter_size(std::size_t size) {
        kn_sliding_counter_->resize(size);
    }

    void set_kn_sliding_counter_valid_rate_threshold(double kn_sliding_counter_valid_rate_threshold) {
        kn_sliding_counter_valid_rate_threshold_ = kn_sliding_counter_valid_rate_threshold;
    }

    void set_kn_cnt_threshold(int kn_cnt_threshold) {
        kn_cnt_threshold_ = kn_cnt_threshold;
    }

    void set_kn_cnt_max(int kn_cnt_max) {
        kn_cnt_max_ = kn_cnt_max;
    }

    void set_breakout_params(const move::DrillBreakOutParams& breakout_params);

    void clear();

public:
    auto get_last_realtime_voltage() const { return last_realtime_voltage_; }
    auto get_last_averaged_voltage() const { return last_averaged_voltage_; }
    auto get_last_stderr() const { return last_stderr_; }
    auto get_last_kn_sliding_counter_valid_rate() const { return last_kn_sliding_counter_valid_rate_; }
    auto get_last_kn_cnt() const { return kn_cnt_; }

    bool is_kn_detected() const { return kn_cnt_ > kn_cnt_threshold_; }

public:
    void push_back_realtime_voltage(int realtime_voltage);

private:

    SlidingFilter::ptr realtime_voltage_average_filter_; // 瞬时电压的平均滤波器 
    SlidingFilter::ptr averaged_voltage_stderr_filter_; // 平均电压的标准差滤波器
    
    double stderr_threshold_ {20.0}; // 标准差阈值

    constexpr static int KnSlidingCounterMaxSize = 4000;
    SlidingCounter<KnSlidingCounterMaxSize>::ptr kn_sliding_counter_; // 用于计算电压的滑动计数器
    double kn_sliding_counter_valid_rate_threshold_ {0.01};

    int kn_cnt_threshold_ {0};
    int kn_cnt_max_ {0};
    int kn_cnt_ {0};

    // temp data
    int last_realtime_voltage_ {0};
    int last_averaged_voltage_ {0};
    double last_stderr_ {0};
    double last_kn_sliding_counter_valid_rate_ {0};

};

}   // namespace util
}   // namespace edm

#endif