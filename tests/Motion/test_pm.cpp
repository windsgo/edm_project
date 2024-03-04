#include "Logger/LogMacro.h"

#include "Motion/MotionThread/MotionCommandQueue.h"
#include "Motion/MotionThread/MotionThreadController.h"

#include "Motion/MotionSignalQueue/MotionSignalQueue.h"

#include "GlobalCommandQueue/GlobalCommandQueue.h"

#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"

#include <QCoreApplication>
#include <QObject>

#include <signal.h>
#include <unistd.h>

#include <json.hpp>

EDM_STATIC_LOGGER_NAME(s_logger, "test");

static auto global_command_queue =
    std::make_shared<edm::global::GlobalCommandQueue>();
static auto motion_signal_queue =
    std::make_shared<edm::move::MotionSignalQueue>();
static auto motion_cmd_queue =
    std::make_shared<edm::move::MotionCommandQueue>();

static edm::move::MotionThreadController::ptr motion_controller;

static edm::InfoDispatcher *info_dispatcher{nullptr};

static std::atomic_bool exit_flag{false};

static std::thread test_thread;

static void system_init() {

    const char *cfg_file = EDM_ROOT_DIR "Conf/ecatdefine.json";

    std::ifstream ifs(cfg_file);

    if (!ifs.is_open()) {
        s_logger->critical("ecat cfg: {}, open failed", cfg_file);
        exit(-1);
    }

    auto parse_ret = json::parse(ifs, false);

    ifs.close();

    if (!parse_ret) {
        s_logger->critical("ecat cfg: {}, parse failed", cfg_file);
        exit(-1);
    }

    auto &&settings = (*parse_ret)["settings"];
    auto netif_name = settings["netif_name"].as_string();
    auto iomap_size = settings["iomap_size"].as_unsigned();

    motion_controller = std::make_shared<edm::move::MotionThreadController>(
        netif_name, motion_cmd_queue, motion_signal_queue, iomap_size,
        EDM_SERVO_NUM, 0);

    info_dispatcher = new edm::InfoDispatcher(motion_signal_queue,
                                              motion_controller, nullptr, 500);
}

using namespace std::chrono_literals;

static void cmd_ecat_connect() {
    auto ecat_connect_cmd =
        std::make_shared<edm::move::MotionCommandSettingTriggerEcatConnect>();
    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [&](edm::move::MotionCommandSettingTriggerEcatConnect::ptr) {
            motion_cmd_queue->push_command(ecat_connect_cmd);
        },
        ecat_connect_cmd);

    global_command_queue->push_command(gcmd);
}

static void cmd_start_pointmove() {

    while (!info_dispatcher->get_info().EcatAllEnabled()) {
        std::this_thread::sleep_for(1s);
    }

    auto start_pos = info_dispatcher->get_info().curr_cmd_axis_blu;
    auto target_pos = start_pos;
    target_pos[0] += 5000;

    s_logger->debug("start: {}, target: {}", start_pos[0], target_pos[0]);

    edm::move::MoveRuntimePlanSpeedInput speed;
    speed.nacc = 60;
    speed.entry_v = 0;
    speed.exit_v = 0;
    speed.cruise_v = 50000;
    speed.acc0 = 500000;
    speed.dec0 = -speed.acc0;

    auto start_pointmove_cmd =
        std::make_shared<edm::move::MotionCommandManualStartPointMove>(
            start_pos, target_pos, speed);

    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [&](edm::move::MotionCommandManualStartPointMove::ptr) {
            motion_cmd_queue->push_command(start_pointmove_cmd);
        },
        start_pointmove_cmd);

    global_command_queue->push_command(gcmd);
}

static void test_init() {

    QObject::connect(
        info_dispatcher, &edm::InfoDispatcher::info_updated,
        [](const edm::move::MotionInfo &info) {
            s_logger->info(
                "* info: cmd: {}, act: {} | ecat: {}, {}, {:04b} | mode: {}",
                info.curr_cmd_axis_blu[0], info.curr_act_axis_blu[0],
                info.EcatConnected(), info.EcatAllEnabled(), info.bit_state1,
                edm::move::MotionStateMachine::GetMainModeStr(info.main_mode));
        });

    test_thread = std::thread([]() {
        std::this_thread::sleep_for(1s);

        s_logger->debug("cmd_ecat_connect");
        cmd_ecat_connect();

        std::this_thread::sleep_for(2s);
        s_logger->debug("cmd_start_pointmove");
        cmd_start_pointmove();
    });
}

static void test_quit() {}

static void system_quit() { delete info_dispatcher; }

static void fun_sig(int sig) {
    exit_flag = true;
    QCoreApplication::quit();
    s_logger->info("ctrl c");

    signal(sig, SIG_DFL); // switch to default
}

int main(int argc, char **argv) {

    QCoreApplication app(argc, argv);
    signal(SIGINT, fun_sig);

    system_init();

    test_init();

    auto ret = app.exec();

    test_quit();
    s_logger->debug("after app.exec(): {}", ret);

    system_quit();
    s_logger->debug("after system_quit()");

    test_thread.join();

    return ret;
}
