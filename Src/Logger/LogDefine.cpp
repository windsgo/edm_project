#include "LogDefine.h"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

namespace edm {

namespace log {

sink_ptr SinkDefine::to_sink() const {

    sink_ptr sink;

    if (this->is_multithread()) {
        // mt
        if (is_type_stdout()) {
            sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        } else {
            sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                this->filename, this->max_size, this->max_files,
                this->rotate_on_open);
        }
    }

    else {
        // st
        if (this->is_type_stdout()) {
            sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        } else {
            sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                this->filename, this->max_size, this->max_files,
                this->rotate_on_open);
        }
    }

    if (!sink)
        return sink;

    sink->set_level(spdlog::level::from_str(this->level));
    if (!this->format.empty()) {
        sink->set_pattern(this->format);
    }

    return sink;
}

static spdlog::async_overflow_policy policy_from_str(const std::string &str) {
    if (str == "discard_new") {
        return spdlog::async_overflow_policy::discard_new;
    } else if (str == "overrun_oldest") {
        return spdlog::async_overflow_policy::overrun_oldest;
    } else {
        return spdlog::async_overflow_policy::block;
    }
}

logger_ptr LogDefine::to_logger() const {
    std::vector<sink_ptr> spdlog_sinks;

    for (const auto &sink_define : this->sinks) {
        spdlog_sinks.emplace_back(sink_define.to_sink());
    }

    logger_ptr logger;
    if (this->is_async()) {
        logger = std::make_shared<spdlog::async_logger>(
            this->name, spdlog_sinks.begin(), spdlog_sinks.end(),
            spdlog::thread_pool(), policy_from_str(this->async_policy));
    } else {
        logger = std::make_shared<spdlog::logger>(
            this->name, spdlog_sinks.begin(), spdlog_sinks.end());
    }

    if (!logger)
        return logger;

    logger->set_level(spdlog::level::from_str(this->level));

    return logger;
}

std::vector<logger_ptr> TotalLogDefines::to_loggers() const {

    std::vector<logger_ptr> spd_loggers;

    for (const auto &logdefine : this->loggers) {
        if (auto logger = logdefine.to_logger()) {
            spd_loggers.emplace_back(logger);
        }
    }

    return spd_loggers;
}

} // namespace log

} // namespace edm