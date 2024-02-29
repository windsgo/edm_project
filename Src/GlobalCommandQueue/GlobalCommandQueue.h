#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

#include <cstdint>

#include "GlobalCommand.h"

namespace edm
{

namespace global
{

class GlobalCommandQueue final {
public:
    using ptr = std::shared_ptr<GlobalCommandQueue>;

public:
    GlobalCommandQueue();
    ~GlobalCommandQueue();

    GlobalCommandQueue(const GlobalCommandQueue&) = delete;
    GlobalCommandQueue& operator=(const GlobalCommandQueue&) = delete;
    GlobalCommandQueue(GlobalCommandQueue&&) = delete;
    GlobalCommandQueue& operator=(GlobalCommandQueue&&) = delete;

public:
    void push_command(edm::global::CommandBase::ptr cmd);

    std::size_t size() const;

private:
    static void _ThreadEntry(GlobalCommandQueue* gcq);

    void _run();

private:
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic_bool thread_exit_flag_ {false};

    std::queue<edm::global::CommandBase::ptr> command_queue_;
};
    
} // namespace global

} // namespace edm


