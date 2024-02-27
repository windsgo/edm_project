#pragma once

#include <vector>

#include <bitset>

namespace edm {

namespace util {

template <int size> class SlidingCounter {
    static_assert(size > 0);

public:
    SlidingCounter()
        : current_window_size_(size), cursor_(0), valid_count_(0),
          invalid_count_(0) {}
    ~SlidingCounter() = default;

    void clear() {
        cursor_ = 0;
        valid_count_ = 0;
        invalid_count_ = 0;
    }

    void resize(std::size_t new_size) {
        if (new_size == current_window_size_ || new_size == 0) {
            return;
        }

        current_window_size_ = new_size;
        clear();
    }

    bool full() const {
        return valid_count_ + invalid_count_ >= current_window_size_;
    }

    void push_back(bool valid) {
        if (valid) {
            push_back_valid();
        } else {
            push_back_invalid();
        }
    }

    void push_back_valid() {
        if (!full()) {
            bits_[cursor_] = 1;
            ++valid_count_;
        } else {
            auto old_value = bits_[cursor_];
            if (!old_value) {
                bits_[cursor_] = 1;
                ++valid_count_;
                --invalid_count_;
            }
        }

        cursor_ = (cursor_ + 1) % current_window_size_;
    }

    void push_back_invalid() {
        if (!full()) {
            bits_[cursor_] = 0;
            ++invalid_count_;
        } else {
            auto old_value = bits_[cursor_];
            if (old_value) {
                bits_[cursor_] = 0;
                --valid_count_;
                ++invalid_count_;
            }
        }
        
        cursor_ = (cursor_ + 1) % current_window_size_;
    }

    double valid_rate() const {
        return (double)valid_count_ / (double)current_window_size_;
    }

    // void test_print() const {
    //     printf("valid_count: %d, invalid_count: %d\n", valid_count_, invalid_count_);
    //     printf("current_window_size: %d, cursor: %d\n", current_window_size_, cursor_);
    // }

private:
    static constexpr const std::size_t max_size_ = size;
    std::bitset<size> bits_;

    std::size_t current_window_size_;
    std::size_t cursor_;

    std::size_t valid_count_;
    std::size_t invalid_count_;
};

} // namespace util

} // namespace edm