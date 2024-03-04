#include "GlobalCommandQueue.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace global {

GlobalCommandQueue::GlobalCommandQueue() {
    // start the queue thread
    thread_ = std::thread(GlobalCommandQueue::_ThreadEntry, this);
}

GlobalCommandQueue::~GlobalCommandQueue() {
    s_logger->trace("{}", __PRETTY_FUNCTION__);
    // stop the thread
    thread_exit_flag_ = true;

    // 信号量唤醒可能正在睡眠的线程, 执行退出
    cv_.notify_all();

    // wait for thread exit
    if (thread_.joinable()) {
        thread_.join();
    }
}

void GlobalCommandQueue::push_command(edm::global::CommandBase::ptr cmd) {
    std::lock_guard guard(mutex_);

    command_queue_.push(cmd);
    cv_.notify_all(); // 唤醒线程执行命令
}

std::size_t GlobalCommandQueue::size() const {
    std::lock_guard guard(mutex_);

    return command_queue_.size();
}

void GlobalCommandQueue::_ThreadEntry(GlobalCommandQueue *gcq) {
    s_logger->trace("GlobalCommandQueue::_ThreadEntry entry");
    gcq->_run();
    s_logger->trace("GlobalCommandQueue::_ThreadEntry exit");
}

void GlobalCommandQueue::_run() {
    while (1) {
        CommandBase::ptr fetched_cmd {nullptr};
        // fetch command from queue
        {
            std::unique_lock ul(mutex_);

            // 等待被唤醒, 且退出flag置位或队列非空
            cv_.wait(ul, [this]() -> bool {
                return this->thread_exit_flag_ || !command_queue_.empty();
            });

            // 如果队列非空, 取出一个命令
            if (!command_queue_.empty()) {
                fetched_cmd = command_queue_.front();
                command_queue_.pop();
            }
        }

        // 如果取出命令有效, 执行命令(无锁环境), 防止调用方阻塞
        if (fetched_cmd) {
            fetched_cmd->run();
        }

        // 如果停止位置位, 退出线程
        if (thread_exit_flag_) {
            break;
        }
    }
}

} // namespace global

} // namespace edm
