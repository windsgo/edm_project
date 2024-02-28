#include "MotionCommandQueue.h"

namespace edm {

namespace move {

std::optional<MotionCommandBase::ptr> MotionCommandQueue::try_get_command() {
    if (!queue_mutex_.try_lock()) {
        return std::nullopt;
    }

    //! lock is acquired

    if (command_queue_.empty()) {
        queue_mutex_.unlock();
        return std::nullopt;
    }

    auto cmd = command_queue_.front();
    command_queue_.pop();

    queue_mutex_.unlock();

    if (!cmd) {
        return std::nullopt;
    } else [[likely]] {
        return cmd;
    }
}

std::optional<MotionCommandBase::ptr> MotionCommandQueue::get_command() {
    std::lock_guard guard(queue_mutex_);

    if (command_queue_.empty()) {
        return std::nullopt;
    }

    auto cmd = command_queue_.front();
    command_queue_.pop();

    if (!cmd) {
        return std::nullopt;
    } else [[likely]] {
        return cmd;
    }
}

void MotionCommandQueue::push_command(MotionCommandBase::ptr command) {
    std::lock_guard guard(queue_mutex_);

    command_queue_.push(command);
}

void MotionCommandQueue::clear() {
    std::lock_guard guard(queue_mutex_);

    while (!command_queue_.empty()) {
        command_queue_.pop();
    }
}

std::size_t MotionCommandQueue::size() const {
    std::lock_guard guard(queue_mutex_);

    return command_queue_.size();
}

bool MotionCommandQueue::empty() const {
    std::lock_guard guard(queue_mutex_);

    return command_queue_.empty();
}

} // namespace move

} // namespace edm
