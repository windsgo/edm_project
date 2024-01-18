#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace edm {

namespace log {

using logger_t = spdlog::logger;
using logger_ptr = std::shared_ptr<logger_t>;
using mutex_t = std::mutex;
using lockguard_t = std::lock_guard<mutex_t>;

enum class LogThreadType {
    SingleThread,
    MultiThread
};

enum class LogSyncType {
    Sync,
    Async
};

enum class LogTargetType {
    OnlyStdout,
    OnlyFile,
    StdoutAndFile
};

class LoggerDefine final {
    friend class LoggerFactory;
public:


private:
    LogThreadType thread_type_;
    LogSyncType sync_type;
    LogTargetType target_type_;
    
    std::string filename_ = "default_log.log";
    std::string stdout_format_ = "";
    std::string file_format_;
};

class LoggerFactory final {

};

class LogWrapper final {
public:
    LogWrapper();
    ~LogWrapper();

    logger_ptr get_logger() const { return logger_; }



private:
    logger_ptr logger_;
};

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
    logger_ptr get_root_logger() const;

    // Explicitly add a new logger named `logger_name` to the LogManager
    // instance. Return the input logger. If logger is nullptr, failed. If
    // logger_name exists, this will override that logger.
    logger_ptr add_logger(const std::string &logger_name, logger_ptr logger);

private:
    std::unordered_map<std::string, logger_ptr> logger_list_;

    // Root logger is used to ensure that there is at least one avaiable logger
    // when you get a logger from the LogManager instance.
    logger_ptr root_logger_;

    // Used to ensure the safety of `logger_list_`. Loggers themselves do not
    // need to lock when being get they have been designed and definded to be
    // single-thread or to be multi-thread
    mutable mutex_t mutex_;

private:
    LogManager() noexcept = default;
    ~LogManager() noexcept = default;

    LogManager(const LogManager &) = delete;
    LogManager(LogManager &&) = delete;
};

} // namespace log

} // namespace edm
