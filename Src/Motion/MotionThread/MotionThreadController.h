#pragma once

#include <mutex>

#include "EcatManager/EcatManager.h"
#include "MotionCommandQueue.h"

// C Headers
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace edm {

namespace move {

class MotionThreadController final {
public:
    MotionThreadController(std::string_view ifname,
                           MotionCommandQueue::ptr motion_cmd_queue,
                           uint32_t servo_num, uint32_t io_num = 0);
    ~MotionThreadController();

    MotionThreadController(const MotionThreadController &) = delete;
    MotionThreadController &operator=(const MotionThreadController &) = delete;
    MotionThreadController(MotionThreadController &&) = delete;
    MotionThreadController &operator=(MotionThreadController &&) = delete;

public:
    // 停止线程
    // void stop_thread() { thread_stop_flag_ = true; }

    // TODO 获取info状态的线程安全接口

private: // Thread
    // pass this function ptr to thread controller
    // mtc is `MotionThreadController*` type
    static void *_ThreadEntry(void *mtc);

    // the actual thread entry of `MotionThreadController`
    void *_run();

    void _threadstate_stopping(); // return true if the thread can exit
    void _threadstate_running();

    // 设置 cpu dma latency 防止cpu休眠
    bool _set_cpu_dma_latency();

    // 创建 rt 线程
    bool _create_thread();

    // Fetch Command And Handle
    void _fetch_command_and_handle();

    // 处理DC时间同步
    void _dc_sync();

private: // Command

private: // State
    // Ecat State
    enum class EcatState {
        Init, // Init State
        EcatDisconnected, // Ecat未连接
        EcatConnecting, // Ecat连接中
        EcatConnectedNotAllEnabled, // Ecat已连接但需要清错误并使能
        EcatConnectedEnabling, // Ecat已连接, 正在清错并使能
        EcatReady, // Ecat已连接
    } ecat_state_ {EcatState::Init};

    // Thread State: 
    enum class ThreadState {
        Init, // Init, 未启动
        Running, // 线程在跑
        Stopping, // 线程停止中
        CanExit, // 线程可以退出
    } thread_state_ {ThreadState::Init};

private: // Data
    // 用于获取外部输入的命令的队列
    MotionCommandQueue::ptr motion_cmd_queue_{nullptr};

    // EcatManager
    ecat::EcatManager::ptr ecat_manager_{nullptr};
    bool ecat_connect_flag_{
        false}; // 启动ecat的标志位, 一旦有这个标志位, 就会检查ecat是否连接,
                // 如果未连接就会在线程的某个周期尝试连接
    bool ecat_clear_fault_reenable_flag_{false}; // 清错重新使能标志位
    
    int stopping_count_ = 0; // 线程退出时的计数器
    const int stopping_count_max_ = 2000; // 线程退出时计数器最大值, 超过此值就直接退出

    // 每周期结束获取状态机的状态, 并缓存到:
    // TODO 缓存结构体
    std::mutex info_cache_mutex_; // TODO(改名), 用于保护该缓存结构体

    // Thread 相关
    pthread_t thread_;
    std::atomic_bool thread_stop_flag_{false}; // 用于指示线程退出
    const int64_t cycletime_ns_{EDM_SERVO_PEROID_NS}; // 周期(ns)
    int64_t toff_{0}; // 每周期cycletime的修正量(ns) (基于DC同步)

    const int32_t latency_target_value_{0}; // 消除系统时钟偏移(禁止电源休眠)
};

} // namespace move

} // namespace edm
