#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace edm {

namespace util {

template <typename DataType> class LongPeroidAverager {
public:
    using ptr = std::shared_ptr<LongPeroidAverager<DataType>>;
    using NewMaxCallback_t = std::function<void(const DataType &new_max_value)>;
    LongPeroidAverager() = default;
    LongPeroidAverager(const NewMaxCallback_t &cb_new_max)
        : cb_new_max_(cb_new_max) {}

    void push(const DataType &d) {
        latest_ = d;

        if (count_ > 0) [[likely]] {
            if (d < min_) {
                min_ = d;
            }
        } else {
            min_ = d;
        }

        if (d > max_) {
            max_ = d;
            if (cb_new_max_) {
                cb_new_max_(max_);
            }
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

    NewMaxCallback_t cb_new_max_;
};

} // namespace util

} // namespace edm
