#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "Utils/Filters/LongPeroidAverager/LongPeroidAverager.h"
#include "config.h"

#ifdef EDM_ENABLE_TIMEUSE_STAT
// 时间统计宏
#define TIMEUSESTAT(TimeUseStatistic__, work__, stat_condition__) \
    {                                                             \
        edm::util::TimeUseCalcWrapper t__;                        \
        work__;                                                   \
        if ((stat_condition__)) {                                 \
            TimeUseStatistic__.push(t__.elapsed_ns());            \
        }                                                         \
    }
#else // EDM_ENABLE_TIMEUSE_STAT
#define TIMEUSESTAT(TimeUseStatistic__, work__, stat_condition__) work__;
#endif // EDM_ENABLE_TIMEUSE_STAT

#define TIMEUSESTAT_AVG(TimeUseStatistic__) \
    TimeUseStatistic__.averager().average()
#define TIMEUSESTAT_MIN(TimeUseStatistic__) TimeUseStatistic__.averager().min()
#define TIMEUSESTAT_MAX(TimeUseStatistic__) TimeUseStatistic__.averager().max()
#define TIMEUSESTAT_LATEST(TimeUseStatistic__) \
    TimeUseStatistic__.averager().latest()

namespace edm {

namespace util {

class TimeUseCalcWrapper final {
public:
    TimeUseCalcWrapper() { restart(); };

    inline void restart() { clock_gettime(CLOCK_MONOTONIC, &start); }

    inline int64_t elapsed_ns() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        return _calcdiff_ns(now, start);
    }

private:
    static inline int64_t _calcdiff_ns(const timespec &t1, const timespec &t2) {
        int64_t diff = 1000000000 * (int64_t)((int)t1.tv_sec - (int)t2.tv_sec);
        diff += ((int)t1.tv_nsec - (int)t2.tv_nsec);
        return diff;
    }

private:
    struct timespec start;
};

class TimeUseStatistic final {
public:
    using ptr = std::shared_ptr<TimeUseStatistic>;
    TimeUseStatistic() = default;

    inline void clear() { averager_.clear(); } 

    inline void push(int64_t v) { averager_.push(v); }

    inline auto &averager() const { return averager_; }

private:
    LongPeroidAverager<int64_t> averager_;
};

} // namespace util

} // namespace edm
