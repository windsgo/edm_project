#pragma once
#include <memory>
#include <string>

// json serialize
#include <json.hpp>

// spdlog
#include <spdlog/spdlog.h>

namespace edm {

namespace log {

using logger_ptr = std::shared_ptr<spdlog::logger>;
using sink_ptr = spdlog::sink_ptr;

struct GlobalThreadPoolDefine {
    std::size_t q_size = 8192;
    std::size_t thread_count = 1;

    MEO_JSONIZATION(MEO_OPT q_size, MEO_OPT thread_count)
};

struct SinkDefine {
    std::string type;     // "file" or "stdout"
    std::string filename; // used for log file if type is "file"
    std::string format;   // pattern, if "", set to default pattern
    std::string level{SPDLOG_LEVEL_NAME_TRACE.begin(),
                      SPDLOG_LEVEL_NAME_TRACE.end()};
    bool multi_thread = false;               // thread safety
    bool rotate_on_open = false;             // for file
    std::size_t max_size = 1024 * 1024 * 10; // for file
    std::size_t max_files = 5;               // for file

    MEO_JSONIZATION(type, MEO_OPT filename, MEO_OPT format, MEO_OPT level,
                    MEO_OPT multi_thread, MEO_OPT rotate_on_open,
                    MEO_OPT max_size, MEO_OPT max_files);

    sink_ptr to_sink() const;

private:
    bool is_multithread() const { return multi_thread; }
    bool is_type_stdout() const { return type == "stdout"; }
};

struct LogDefine {
    std::string name;
    std::string level{SPDLOG_LEVEL_NAME_TRACE.begin(),
                      SPDLOG_LEVEL_NAME_TRACE.end()};
    std::string flush_level{SPDLOG_LEVEL_NAME_INFO.begin(),
                            SPDLOG_LEVEL_NAME_INFO.end()};
    std::vector<SinkDefine> sinks;
    bool async_mode = false;
    std::string async_policy{"block"};

    MEO_JSONIZATION(name, sinks, MEO_OPT async_mode, MEO_OPT async_policy,
                    MEO_OPT level, MEO_OPT flush_level);

    logger_ptr to_logger() const;

private:
    bool is_async() const { return async_mode; }
};

using LogDefineArray = std::vector<LogDefine>;

struct TotalLogDefines {
    GlobalThreadPoolDefine global_thread_pool;
    LogDefineArray loggers;
    std::string default_logger_level{SPDLOG_LEVEL_NAME_TRACE.begin(),
                                     SPDLOG_LEVEL_NAME_TRACE.end()};

    MEO_JSONIZATION(MEO_OPT global_thread_pool, MEO_OPT loggers,
                    MEO_OPT default_logger_level);

    std::vector<logger_ptr> to_loggers() const;
    std::size_t get_tp_q_size() const { return global_thread_pool.q_size; }
    std::size_t get_tp_thread_count() const {
        return global_thread_pool.thread_count;
    }
    spdlog::level::level_enum get_default_logger_level() const {
        return spdlog::level::from_str(default_logger_level);
    }
};

} // namespace log

} // namespace edm