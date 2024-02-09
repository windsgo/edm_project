#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "Logger/LogDefine.h"

namespace edm {

namespace log {

using mutex_t = std::mutex;
using lockguard_t = std::lock_guard<mutex_t>;

class LogManager final {
public:
    static LogManager *instance();

public:
    // Get a logger named `logger_name`, you can use it, modified it, or reset
    // it to another/new logger. If it doesn't exist, a default logger is
    // constructed and returned
    logger_ptr get_logger(const std::string &logger_name);

    // Gget root logger, which is initialized when LogManager instance is
    // constructed
    logger_ptr get_root_logger();

private:
    void _init();

private:
    static void configure_global_threadpool(std::size_t q_size, std::size_t thread_count);

private:
    // Root logger is used to ensure that there is at least one avaiable logger
    // when you get a logger from the LogManager instance.
    logger_ptr root_logger_;

private:
    LogManager();
    ~LogManager() noexcept = default;

    LogManager(const LogManager &) = delete;
    LogManager(LogManager &&) = delete;
};

} // namespace log

} // namespace edm
