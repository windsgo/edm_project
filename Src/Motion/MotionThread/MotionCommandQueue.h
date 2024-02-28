#pragma once

#include "MotionCommand.h"

#include <memory>
#include <mutex>
#include <optional>
#include <queue>

namespace edm {

namespace move {

class MotionCommandQueue final {
public:
    using ptr = std::shared_ptr<MotionCommandQueue>;
    MotionCommandQueue() noexcept = default;
    ~MotionCommandQueue() noexcept = default;

    MotionCommandQueue(const MotionCommandQueue&) = delete;
    MotionCommandQueue& operator=(const MotionCommandQueue&) = delete;
    MotionCommandQueue(MotionCommandQueue&&) = delete;
    MotionCommandQueue& operator=(MotionCommandQueue&&) = delete;

    // try lock the queue and get command
    // if lock is gotten succussfully and there is command in queue, pop and
    // return the command, or return an std::nullopt
    std::optional<MotionCommandBase::ptr> try_get_command();

    // lock the queue and get command
    // in contrast to `try_get_command`, if lock cannot be gotten immediately,
    // the caller thread will sleep until the lock is available
    //! not recommanded to be called by the `Motion Thread`
    std::optional<MotionCommandBase::ptr> get_command();

    // lock the queue and push command
    void push_command(MotionCommandBase::ptr command);

    // lock and clear the command queue
    void clear();

    // lock and get the size of the command queue
    std::size_t size() const;

    // lock and judge if the command queue is empty
    bool empty() const;

private:
    std::queue<MotionCommandBase::ptr> command_queue_;
    mutable std::mutex queue_mutex_;
};

} // namespace move

} // namespace edm
