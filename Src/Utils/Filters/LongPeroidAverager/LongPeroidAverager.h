#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace edm {

namespace util {

template <typename DataType> class LongPeroidAverager {
public:
    using ptr = std::shared_ptr<LongPeroidAverager<DataType>>;
    LongPeroidAverager() = default;

    void push(const DataType &d) {
        latest_ = d;

        if (count_ > 0) [[likely]] {
            if (d < min_) {
                min_ = d;
            }
        } else {
            min_ = d;
        }

        if (d > max_) [[unlikely]] {
            max_ = d;
        }

        ++count_;
        average_ = (double)(count_ - 1) / (double)count_ * average_ +
                   d / (double)count_;
    }

    inline void clear() {
        count_ = 0;
        average_ = 0;
        max_ = 0;
        min_ = 0;
        latest_ = 0;
    }

    inline const auto &latest() const { return latest_; }
    inline const auto &min() const { return min_; }
    inline const auto &max() const { return max_; }
    inline const auto &average() const { return average_; }

private:
    int64_t count_{0};

    DataType latest_{};
    DataType min_{};
    DataType max_{};
    DataType average_{};
};

} // namespace util

} // namespace edm
