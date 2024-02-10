#include "Logger/LogManager.h"
#include "Logger/LogDefine.h"
#include "Exception/exception.h"
#include "Utils/Format/edm_format.h"
#include "config.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <optional>

#include <json.hpp>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace edm {

namespace log {

LogManager::LogManager() { 
    _init(); 
    root_logger_ = get_logger("root");
}

LogManager *LogManager::instance() {
    // 使用懒汉式, 第一次调用时初始化,
    // 可以保证初始化顺序在调用LogManager方法之前
    static LogManager instance;
    return &instance;
}

logger_ptr LogManager::get_logger(const std::string &logger_name) {
    lockguard_t guard(mutex_);

    if (auto logger = spdlog::get(logger_name)) {
        return logger;
    } else {
        auto new_logger = spdlog::stdout_color_mt(logger_name);
        new_logger->set_level(spdlog::level::trace);
        // spdlog::register_logger(new_logger);
        spdlog::info("get_logger create new logger: {}", logger_name);

        return new_logger;
    }
}

logger_ptr LogManager::get_root_logger() const { 
    return root_logger_; 
}

void LogManager::_init() {
    std::ifstream ifs(EDM_LOG_CONFIG_FILE);
    // std::stringstream ss;
    // ss << ifs.rdbuf();
    // std::string str = ss.str();

    // fmt::print("str:\n{}\n", str);

    // auto ret = json::parse(str);

    auto ret = json::parse(ifs, false);
    ifs.close();

    if (!ret) {
        std::cout << EDM_FMT::format("error parse\n");
        throw exception("log config parse error");
    }

    auto j_totallogdefines = std::move(ret.value());

    // fmt::print("{}\n ****************\n", j_totallogdefines.format());

    TotalLogDefines tlds;
    try {
        tlds = (TotalLogDefines)j_totallogdefines;
    } catch (const json::exception &je) {
        throw exception(EDM_FMT::format("log config not valid, {}", je.what()));
    }

    // initialize loggers

    // tlds.do_tp_init(); // this must be called before to_loggers()
    configure_global_threadpool(tlds.get_tp_q_size(), tlds.get_tp_thread_count());
    spdlog::default_logger()->set_level(tlds.get_default_logger_level());
    auto loggers = tlds.to_loggers();

    if (loggers.empty()) {
        spdlog::warn("logger init: no loggers");
        return;
    }

    for (auto &logger : loggers) {
        if (auto exist_logger = spdlog::get(logger->name())) {
            exist_logger->swap(*logger);
            spdlog::info("logger init: swap exisiting logger: {}",
                         logger->name());
        } else {
            spdlog::register_logger(logger);
            spdlog::info("logger init: register new logger: {}",
                         logger->name());
        }
    }
}

struct ThreadPoolIniter {
    ThreadPoolIniter(std::size_t q_size, std::size_t thread_count) {
        spdlog::init_thread_pool(q_size, thread_count);
    }
};
void LogManager::configure_global_threadpool(std::size_t q_size,
                                             std::size_t thread_count) {
    static ThreadPoolIniter _once_initer{q_size,
                                         thread_count}; // 保证只初始化一次
}

} // namespace log

} // namespace edm
