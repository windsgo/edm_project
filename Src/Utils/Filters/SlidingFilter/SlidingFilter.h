#pragma once

#include <cmath>
#include <cstdio>
#include <typeinfo>
#include <vector>
#include <memory>

namespace edm {

namespace util {

class SlidingFilter {
public:
    using ptr = std::shared_ptr<SlidingFilter>;

    using value_type = int32_t;
    using sum_value_type = int64_t;
    using average_type = double;
    using stderr_type = double;

    SlidingFilter(std::size_t size)
        : overflowed_loopback_(false), cursor_(0), sum_((sum_value_type)0),
          sq_sum_((sum_value_type)0) {
        if (size == 0) {
            size = 1; // size cannot be zero
        }
        data_.resize(size);
    }
    ~SlidingFilter() = default;

    void clear() {
        overflowed_loopback_ = false;
        cursor_ = 0;
        sum_ = (sum_value_type)0;
        sq_sum_ = (sum_value_type)0;
    }

    void push_back(const value_type &new_value) {
        value_type old_value = (value_type)0;
        if (overflowed_loopback_) {
            old_value = data_[cursor_];
        }

        data_[cursor_] = new_value;

        cursor_ = (cursor_ + 1) % data_.size();
        if (cursor_ == 0 && !overflowed_loopback_) {
            overflowed_loopback_ = true;
        }

        sum_ = sum_ + new_value - old_value;

        sum_value_type new_value2 = new_value * new_value;
        sum_value_type old_value2 = old_value * old_value;
        sq_sum_ = sq_sum_ + new_value2 - old_value2;
    }

    void resize(std::size_t size) {
        if (size == data_.size() || size == 0) {
            return;
        }

        clear();
        data_.resize(size);
    }

    std::size_t size() const noexcept { return data_.size(); }

    average_type average() const noexcept {
        return (average_type)sum_ / data_.size();
    }

    stderr_type get_stderr() const noexcept {
        average_type sq_avg = (average_type)sq_sum_ / data_.size();
        average_type avg = (average_type)sum_ / data_.size();

        stderr_type D2 = sq_avg - avg * avg; // 方差 ED^2 = E(X^2) - E(X)^2
        if (D2 < 0) {
            D2 = 0;
        }

        return (stderr_type)std::sqrt(D2);
    }

private:
    std::vector<value_type> data_;
    bool overflowed_loopback_;
    std::size_t cursor_;

    sum_value_type sum_;

    sum_value_type sq_sum_; // 平方和
};

} // namespace util

} // namespace edm
