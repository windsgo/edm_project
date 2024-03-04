#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

boost::lockfree::spsc_queue<int, boost::lockfree::fixed_sized<true>,
                            boost::lockfree::capacity<256>>
    spsc_q;
boost::lockfree::queue<int, boost::lockfree::fixed_sized<false>,
                       boost::lockfree::capacity<256>>
    q;

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

using namespace std::chrono_literals;

// static bool exit_flag = false; // oh no
// static std::atomic_bool exit_flag = false; // ok
static volatile bool exit_flag = false; // ok

#include "GlobalCommandQueue/GlobalCommandQueue.h"

using namespace edm::global;

GlobalCommandQueue::ptr gcq{nullptr};

static void fff(int x) { s_logger->info("fff: x = {}", x); }
static void fff2(int x, const std::string &s) {
    s_logger->info("fff2: x = {}, s = {}", x, s);
}
static void fff1() {
    s_logger->info("fff1: no arg");
    std::this_thread::sleep_for(600ms);
}

static void push_a_command(int x) {
    auto f = [](int x) { s_logger->info("hi: x = {}", x); };

    // auto cmd_func = std::bind(f, x);

    auto cmd = CommandCommonFunctionFactory::bind(fff, x);
    auto cmd2 = CommandCommonFunctionFactory::bind(fff2, x, "hi");
    auto cmd1 = CommandCommonFunctionFactory::bind(fff1);

    auto cmd3 = CommandCommonFunctionFactory::bind(std::bind(fff, x));
    auto cmd4 = CommandCommonFunctionFactory::bind(f, x);
    auto cmd5 =
        CommandCommonFunctionFactory::bind(std::bind_front(fff2, x), "aaaa");

    gcq->push_command(cmd);
    gcq->push_command(cmd1);
    gcq->push_command(cmd2);
    gcq->push_command(cmd3);
    gcq->push_command(cmd4);
    gcq->push_command(cmd5);
}

static void push_cmds() {
    for (int i = 0; i < 10; ++i) {
        push_a_command(i);
    }

    std::this_thread::sleep_for(1s);

    for (int i = 10; i < 20; ++i) {
        push_a_command(i);
        std::this_thread::sleep_for(200ms);
    }
}

static void test_queue() {
    gcq = std::make_shared<GlobalCommandQueue>();

    push_cmds();
}

static void fun_sig(int sig) {
    exit_flag = true;
    printf("ctrl c\n");

    signal(sig, SIG_DFL); // switch to default
}

#include "Motion/MotionSignalQueue/MotionSignalQueue.h"
static void test_spsc_queue() {

    auto msq = edm::move::MotionSignalQueue{};

    edm::move::MotionSignal s;

    msq.pop(s);

    // spsc_q.consume_all([&](int i){
    //     s_logger->debug("consume: {}", i);
    // });
    // s_logger->debug("size: {}, {}", spsc_q.read_available(),
    // spsc_q.write_available()); s_logger->info("push ret: {}",
    // spsc_q.push(1)); s_logger->info("push ret: {}", spsc_q.push(5));
    // s_logger->info("push ret: {}", spsc_q.push(2));
    // s_logger->info("push ret: {}", spsc_q.push(3));
    // s_logger->info("push ret: {}", spsc_q.push(3));
    // s_logger->info("push ret: {}", spsc_q.push(3));
    // s_logger->info("push ret: {}", spsc_q.push(3));

    // q.consume_all([](){});

    spsc_q.consume_all([&](int i) { s_logger->debug("consume: {}", i); });
    // s_logger->debug("size: {}, {}", spsc_q.read_available(),
    // spsc_q.write_available());

    std::thread t1([&]() {
        std::this_thread::sleep_for(1s);
        for (int i = 0; i < 1000; ++i) {
            std::this_thread::sleep_for(2ms);
            s_logger->info("pushing: {}", i);
            // auto ret = spsc_q.push(i);
            edm::move::MotionSignal ms;
            ms.type = edm::move::MotionSignalType::MotionSignal_AutoStopped;
            // ms.signal_data1 = i * 10;
            ms.info.curr_cmd_axis_blu[1] = i + i;
            auto ret = msq.push(ms);
            s_logger->info("pushing over: {}", ret);
        }
    });

    std::thread t2([&]() {
        while (1) {
            // spsc_q.consume_one([&](int v){
            //     s_logger->info("consuming: {}", v);
            //     std::this_thread::sleep_for(0.5s);
            //     s_logger->info("consume over");
            // });
            msq.consume_one([&](const edm::move::MotionSignal& ms) {
                s_logger->info("consuming: {}, {}", (int)ms.type, ms.info.curr_cmd_axis_blu[1]);
                std::this_thread::sleep_for(10ms);
                s_logger->info("consume over");
            });
        }
    });

    t1.join();
    t2.join();
}

int main(int argc, char **argv) {

    test_spsc_queue();

    return 0;

    signal(SIGINT, fun_sig);

    exit_flag = false;
    printf("exit_flag: %d\n", exit_flag);

    test_queue();

    for (;;) {
        if (exit_flag) {
            printf("break: exit_flag: %d\n", exit_flag);
            break;
        }
    }

    return 0;
}