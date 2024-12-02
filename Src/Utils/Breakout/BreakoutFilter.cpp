#include "BreakoutFilter.h"

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

namespace edm {
namespace util {

BreakoutFilter::BreakoutFilter() {
    realtime_voltage_average_filter_ = std::make_shared<SlidingFilter>(10);
    averaged_voltage_stderr_filter_ = std::make_shared<SlidingFilter>(10);
    kn_sliding_counter_ = std::make_shared<SlidingCounter<KnSlidingCounterMaxSize>>();
}

void BreakoutFilter::set_breakout_params(const move::DrillBreakOutParams& breakout_params)
{
    set_realtime_voltage_average_filter_size(breakout_params.voltage_average_filter_window_size);
    set_averaged_voltage_stderr_filter_size(breakout_params.stderr_filter_window_size);
    set_stderr_threshold(breakout_params.kn_valid_threshold);
    set_kn_sliding_counter_size(breakout_params.kn_sc_window_size);
    set_kn_sliding_counter_valid_rate_threshold(breakout_params.kn_valid_rate_threshold);
    set_kn_cnt_threshold(breakout_params.kn_valid_rate_ok_cnt_threshold);
    set_kn_cnt_max(breakout_params.kn_valid_rate_ok_cnt_maximum);
    
}

void BreakoutFilter::clear() {
    realtime_voltage_average_filter_->clear();
    averaged_voltage_stderr_filter_->clear();
    kn_sliding_counter_->clear();

    kn_cnt_ = 0;

    last_realtime_voltage_ = 0;
    last_averaged_voltage_ = 0;
    last_stderr_ = 0;
    last_kn_sliding_counter_valid_rate_ = 0;
}

void BreakoutFilter::push_back_realtime_voltage(int realtime_voltage)
{
    last_realtime_voltage_ = realtime_voltage;

    realtime_voltage_average_filter_->push_back(realtime_voltage);
    last_averaged_voltage_ = realtime_voltage_average_filter_->average();

    averaged_voltage_stderr_filter_->push_back(last_averaged_voltage_);
    last_stderr_ = averaged_voltage_stderr_filter_->get_stderr();

    if (last_stderr_ > stderr_threshold_) {
        kn_sliding_counter_->push_back_valid();
    } else {
        kn_sliding_counter_->push_back_invalid();
    }

    last_kn_sliding_counter_valid_rate_ = kn_sliding_counter_->valid_rate();

    if (last_kn_sliding_counter_valid_rate_ > kn_sliding_counter_valid_rate_threshold_) {
        ++kn_cnt_;
        if (kn_cnt_ > kn_cnt_max_) {
            kn_cnt_ = kn_cnt_max_;
        }
    } else {
        --kn_cnt_;
        if (kn_cnt_ < 0) {
            kn_cnt_ = 0;
        }
    }
}

} // namespace util
} // namespace edm

#endif