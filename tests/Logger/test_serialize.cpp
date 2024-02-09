#include "json.hpp"
using namespace json::literals;

#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "Logger/LogDefine.h"
#include "config.h"
#include "Logger/LogMacro.h"
#include "Logger/LogManager.h"
#include "Utils/Format/edm_format.h"

#include <filesystem>

#if 0
struct SinkDefine {
    std::string type;
    std::string filename;
    std::string format;

    std::string level{SPDLOG_LEVEL_NAME_TRACE.begin(),
                      SPDLOG_LEVEL_NAME_TRACE.end()};

    MEO_JSONIZATION(type, MEO_OPT filename, MEO_OPT format, MEO_OPT level);
};

struct LogDefine {
    std::string name;
    bool multi_thread = false;
    bool async_mode = false;
    std::string level{SPDLOG_LEVEL_NAME_TRACE.begin(),
                      SPDLOG_LEVEL_NAME_TRACE.end()};
    std::vector<SinkDefine> sinks;

    MEO_JSONIZATION(name, multi_thread, async_mode, MEO_OPT level, sinks);
};

static std::shared_ptr<spdlog::sinks::sink>
from_sink_define(const SinkDefine &sd, bool mt) {
    if (sd.type == "file") {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            sd.filename, 1024 * 1024 * 10, 3);
        if (!sd.format.empty()) {
            sink->set_pattern(sd.format);
        }
        sink->set_level(spdlog::level::from_str(sd.level));
        return sink;
    } else if (sd.type == "stdout") {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        if (!sd.format.empty()) {
            sink->set_pattern(sd.format);
        }
        sink->set_level(spdlog::level::from_str(sd.level));
        return sink;
    } else {
        return nullptr;
    }
}

static std::shared_ptr<spdlog::logger> from_log_define(const LogDefine &ld) {
    std::vector<spdlog::sink_ptr> sinks;

    for (const auto &sd : ld.sinks) {
        sinks.emplace_back(from_sink_define(sd, ld.multi_thread));
    }

    auto logger = std::make_shared<spdlog::async_logger>(
        ld.name, sinks.begin(), sinks.end(), spdlog::thread_pool(),
        spdlog::async_overflow_policy::discard_new);
    
    logger->set_level(spdlog::level::from_str(ld.level));

    return logger;
}

static void test_logdefine() {

    spdlog::init_thread_pool(8192, 1);

#if 1
    std::ifstream ifs(
        "/home/windsgo/Documents/edmproject/edm_project/Conf/logdefine.json");
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string str = ss.str();

    // fmt::print("str:\n{}\n", str);

    auto ret = json::parse(str);

    if (!ret) {
        fmt::print("error parse\n");
        return;
    }

    // bool ret = json::deserialize();

    auto jlogdefines = std::move(ret.value());

    auto jld = jlogdefines.as_array().at(0);

    fmt::print("{}\n", jld.format());
#endif
    // LogDefine ld1{"ld1logger", true, true};
    // json::value jld1 = ld1;

    // std::cout << jld1 << std::endl;

    LogDefine ld = (LogDefine)jld;

    json::value ld_to_j = ld;
    std::cout << ld_to_j << "\n";

    auto logger = from_log_define(ld);

    logger->trace("{}", "hello");
    logger->debug("{}", "hello");
    logger->info("{}", "hello");
    logger->warn("{}", "hello");
    logger->error("{}", "hello");
    logger->critical("{}", "hello");
}
#endif // 0

struct MyStruct {
    // int x = 0;
    std::string x{};

    // 让我们加点魔法
    MEO_JSONIZATION(x);
};

static void test_MyStruct() {

    MyStruct mine;
    mine.x = "abc";

    // 是的，它是那么直观和流畅！
    json::value j_mine = mine;

    j_mine["x"] = json::value{"123"};

    // output:
    // {"map":{"key_1":[{"inner_key_1":[7,8,9]},{"inner_key_2":[10]}]},"vec":[0.500000],"x":0}
    std::cout << j_mine << std::endl;

    // 恰恰，我们也可以把它转回来！
    MyStruct new_mine = (MyStruct)j_mine;
}

static void test_opt() {
    struct Data {
        std::string str_a;
        std::string str_b;
        std::string str_c;

        MEO_JSONIZATION(str_a, MEO_OPT str_b, MEO_OPT str_c);
    };

    struct OptionalFields {
        int a = 0;
        double b = 0;
        std::vector<Data> c;

        MEO_JSONIZATION(a, b, c);
    };

    json::value ja = {{"a", 100},
                      {"b", 123.456},
                      {"c", json::array{{// data 1
                                         {"str_a", "hi a1"}},
                                        {// data 2
                                         {"str_a", "hi a2"},
                                         {"str_b", "hi b2"}}}}};

    std::cout << ja << std::endl;

    if (ja.is<OptionalFields>()) {
        OptionalFields var = (OptionalFields)ja;
        // output: 100
        std::cout << var.a << std::endl;
    }
}

static void test_log() {
    // std::ifstream ifs(
    //     "/home/windsgo/Documents/edmproject/edm_project/Conf/logdefine.json");
    std::ifstream ifs(EDM_LOG_CONFIG_FILE);
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string str = ss.str();

    // fmt::print("str:\n{}\n", str);

    auto ret = json::parse(str);

    if (!ret) {
        fmt::print("error parse\n");
        return;
    }

    // bool ret = json::deserialize();

    auto jtotallogdefines = std::move(ret.value());

    fmt::print("{}\n ****************\n", jtotallogdefines.format());

    edm::log::TotalLogDefines tlds =
        (edm::log::TotalLogDefines)jtotallogdefines;

    json::value examine_jtlds = tlds;

    std::cout << examine_jtlds << "\n";

    auto loggers = tlds.to_loggers();
    auto logger = loggers.at(0);

    logger->trace("{}", "hello!");
    logger->debug("{}", "hello!");
    logger->info("{}", "hello!");
    logger->warn("{}", "hello!");
    logger->error("{}", "hello!");
    logger->critical("{}, {}", "hello!",
                     std::filesystem::current_path().c_str());

    // for (int i = 0; i < 100000; ++i) {
    //     if (i == 50000)
    //         spdlog::init_thread_pool(8192 * 20, 1);
    //     logger->warn("test load {}", i);
    // }
}

EDM_STATIC_LOGGER_NAME(s_motion_logger, "motion");
EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());
static void test_log_manager() {

    using namespace edm::log;
    using namespace std::chrono_literals;

    auto s_logger = LogManager::instance()->get_logger("motion");
    auto l2 = LogManager::instance()->get_logger("other");

    // std::this_thread::sleep_for(1s);

    s_logger->warn("haha");

    // LogManager::instance()->re_init();

    s_motion_logger->warn("haha");

    s_root_logger->warn("111");
    l2->warn("222");
}

int main() {

    // test_MyStruct();

    // test_logdefine();

    // test_opt();

    // test_log();

    test_log_manager();

    s_motion_logger->debug(std::format("hi std format\n"));
    s_motion_logger->debug(EDM_FMT::format("hi std format 2\n"));

    return 0;
}