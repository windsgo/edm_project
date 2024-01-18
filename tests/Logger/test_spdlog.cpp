#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>

static void test_sinks() {
    spdlog::init_thread_pool(8192, 1);

    auto sink_stdout_mt = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(
        spdlog::color_mode::always);

    auto sink_file_mt = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "mylog.txt", 1024 * 1024 * 10, 3);

    std::vector<spdlog::sink_ptr> sinks{sink_stdout_mt, sink_file_mt};
    auto logger = std::make_shared<spdlog::async_logger>(
        "log1", sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    logger->set_level(spdlog::level::trace);
    sink_stdout_mt->set_level(spdlog::level::debug);
    sink_file_mt->set_level(spdlog::level::info);

    spdlog::register_logger(logger);

    sink_stdout_mt->set_pattern("[%C-%m-%d %T.%e] [%n] [%l] [T:%t] %v");
    sink_stdout_mt->set_color_mode(spdlog::color_mode::always);


    logger->trace("this is 你好 {}", "trace");
    logger->debug("this is 你好 {}", "debug");
    logger->info("this is 你好 {}", "info");
    logger->warn("this is 你好 {}", "warn");
    logger->error("this is 你好 {}", "error");
}

int main() {

    spdlog::info("Hi {}", "spdlog");

    test_sinks();

    return 0;
}
