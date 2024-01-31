#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

#include <iostream>

std::shared_ptr<spdlog::logger> g_logger;

static void test_sinks() {
    spdlog::init_thread_pool(8192, 1);

    spdlog::set_level(spdlog::level::trace);

    // logger1

    auto sink_stdout_mt = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(
        spdlog::color_mode::always);

    auto sink_file_mt = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "mylog.txt", 1024 * 1024 * 10, 3);

    std::vector<spdlog::sink_ptr> sinks{sink_stdout_mt, sink_file_mt};
    auto logger = std::make_shared<spdlog::async_logger>(
        "log1", sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    spdlog::register_logger(logger);
    logger->set_level(spdlog::level::trace);

    g_logger = logger;

    // logger2
    // spdlog::drop(logger->name());
    spdlog::debug("log1 exist in global: {}", !!(spdlog::get("log1")) );

    auto sink_stdout_mt2 = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(
        spdlog::color_mode::never);

    // auto sink_file_mt2 = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    //     "mylog.txt", 1024 * 1024 * 10, 3);
    std::vector<spdlog::sink_ptr> sinks2{sink_stdout_mt2};//, sink_file_mt2};
    auto logger2 = std::make_shared<spdlog::async_logger>(
        "log2", sinks2.begin(), sinks2.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);
    // spdlog::register_logger(logger2);
    logger2->set_level(spdlog::level::trace);


    spdlog::debug("log1 exist in global: {}", !!(spdlog::get("log1")) );
    logger->swap(*logger2);
    spdlog::debug("log1 exist in global: {}", !!(spdlog::get("log1")) );


    // sink_stdout_mt->set_pattern("[%C-%m-%d %T.%e] [%n] [%l] [T:%t] %v");
    // sink_stdout_mt->set_color_mode(spdlog::color_mode::always);


    logger->trace("this is 你好 {}", "trace");
    logger->debug("this is 你好 {}", "debug");
    logger->info("this is 你好 {}", "info");
    logger->warn("this is 你好 {}", "warn");
    logger->error("this is 你好 {}", "error");
}

int main() {

    spdlog::info("Hi {}", "spdlog");

    test_sinks();
    spdlog::debug("log1 exist in global: {}", !!(spdlog::get("log1")) );
    spdlog::debug("log2 exist in global: {}", !!(spdlog::get("log2")) );
    spdlog::debug("log1's name: {}", spdlog::get("log1")->name() );

    g_logger->debug("g logger");

    return 0;
}
