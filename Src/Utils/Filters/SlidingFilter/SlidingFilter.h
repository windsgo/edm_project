#pragma once

#include <vector>

namespace edm {

namespace utils {

template <typename value_type> class SlidingFilter {
public:
    SlidingFilter(std::size_t size)
        : overflowed_loopback_(false), cursor_(0), sum_((value_type)0) {
        if (size == 0) {
            size = 1; // size cannot be zero
        }
        data_.resize(size);
    }
    ~SlidingFilter() = default;

    void clear() {
        overflowed_loopback_ = false;
        cursor_ = 0;
        sum_ = (value_type)0;
    }

    void push_back(const value_type &new_value) {
        value_type old_value = (value_type) 0;
        if (overflowed_loopback_) {
            old_value = data_[cursor_];
        }

        data_[cursor_] = new_value;

        cursor_ = (cursor_ + 1) % data_.size();
        if (cursor_ == 0 && !overflowed_loopback_) {
            overflowed_loopback_ = true;
        }

        sum_ = sum_ + new_value - old_value;
    }

    void resize(std::size_t size) { 
        if (size == data_.size() || size == 0) {
            return;
        }

        clear();
        data_.resize(size);
    }

    std::size_t size() const noexcept {
        return data_.size();
    }

    value_type average() const noexcept {
        return sum_ / data_.size();
    }

private:
    std::vector<value_type> data_;
    bool overflowed_loopback_;
    std::size_t cursor_;

    value_type sum_;
};

} // namespace utils

} // namespace edm
