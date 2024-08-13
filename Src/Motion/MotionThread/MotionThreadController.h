#pragma once

#include <atomic>
#include <cstdint>
#include <ctime>
#include <functional>
#include <mutex>

#include "EcatManager/EcatManager.h"
#include "Motion/MotionSignalQueue/MotionSignalQueue.h"
#include "Motion/MotionStateMachine/MotionStateMachine.h"
#include "MotionCommandQueue.h"

#include "Motion/TouchDetectHandler/TouchDetectHandler.h"

#include "Utils/Filters/LongPeroidAverager/LongPeroidAverager.h"
#include "Utils/Time/TimeUseStatistic.h"

#include "CanReceiveBuffer/CanReceiveBuffer.h"
#include "Motion/MotionSharedData/MotionSharedData.h"

#include "SystemSettings/SystemSettings.h"

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
    using ptr = std::shared_ptr<MotionThreadController>;
    MotionThreadController(
        std::string_view ifname, MotionCommandQueue::ptr motion_cmd_queue,
        MotionSignalQueue::ptr motion_signal_queue,
        const std::function<void(bool)> &cb_enable_voltage_gate,
        const std::function<void(bool)> &cb_mach_on,
#ifdef EDM_USE_ZYNQ_SERVOBOARD
        zynq::ZynqUdpMessageHolder::ptr zynq_udpmessage_holder,
#else
        CanReceiveBuffer::ptr can_recv_buffer,
#endif
        uint32_t iomap_size, uint32_t servo_num, uint32_t io_num = 0);
    ~MotionThreadController();

    MotionThreadController(const MotionThreadController &) = delete;
    MotionThreadController &operator=(const MotionThreadController &) = delete;
    MotionThreadController(MotionThreadController &&) = delete;
    MotionThreadController &operator=(MotionThreadController &&) = delete;

public:
    // 停止线程
    // void stop_thread() { thread_stop_flag_ = true; }

#ifdef EDM_MOTION_INFO_GET_USE_ATOMIC
    // Try to use atomic
    void load_at_info_cache(MotionInfo &output) const {
        output = at_info_cache_.load();
    }
#else  // EDM_MOTION_INFO_GET_USE_ATOMIC
    // 获取info状态的线程安全接口
    // 利用RVO/NRVO的优化, 这里可以直接返回值 (返回引用是不安全的)
    auto get_info_cache() const {
        std::lock_guard guard(info_cache_mutex_);

        return info_cache_;
    }
#endif // EDM_MOTION_INFO_GET_USE_ATOMIC

private: // Thread
    // pass this function ptr to thread controller
    // mtc is `MotionThreadController*` type
    static void *_ThreadEntry(void *mtc);

    void _thread_cycle_work();

    // the actual thread entry of `MotionThreadController`
    void *_run();

    void _threadstate_stopping(); // return true if the thread can exit
    void _threadstate_running();

    void _ecat_state_switch_to_ready(); // ecat转为ready, 以及需要做的事情

    // 设置 cpu dma latency 防止cpu休眠
    bool _set_cpu_dma_latency();

    // 创建 rt 线程
    bool _create_thread();

    // Fetch Command And Handle
    void _fetch_command_and_handle_and_copy_info_cache();

    // 用于生成获取实际坐标的回调, 以及info cache获取实际坐标
    // bool _get_act_pos(axis_t &axis);

    void _copy_info_cache();

    void _handle_signal();

    void _ecat_sync_wrapper(const std::function<void(void)> &cb,
                            bool run_check = false);

private: // Command
public:  // State
    // Ecat State
    enum class EcatState {
        Init,                      // Init State
        EcatDisconnected,          // Ecat未连接
        EcatConnecting,            // Ecat连接中
        EcatConnectedWaitingForOP, // Ecat等待OP (IGH在连接之后再等待OP,
                                   // 超时则返回Disconnected)
        EcatConnectedNotAllEnabled, // Ecat已连接但需要清错误并使能
        EcatConnectedEnabling,      // Ecat已连接, 正在清错并使能
        EcatReady,                  // Ecat已连接
    } ecat_state_{EcatState::Init};

    // Thread State:
    enum class ThreadState {
        Init,     // Init, 未启动
        Running,  // 线程在跑
        Stopping, // 线程停止中
        CanExit,  // 线程可以退出
    } thread_state_{ThreadState::Init};

    static const char *GetThreadStateStr(ThreadState s);
    static const char *GetEcatStateStr(EcatState s);

private:
    void _switch_thread_state(ThreadState new_thread_state);
    void _switch_ecat_state(EcatState new_ecat_state);

private: // Data
    // 用于获取外部输入的命令的队列
    MotionCommandQueue::ptr motion_cmd_queue_{nullptr};

    // 用于信号发生
    SignalBuffer::ptr signal_buffer_;
    MotionSignalQueue::ptr motion_signal_queue_{nullptr};

    // EcatManager
    ecat::EcatManager::ptr ecat_manager_{nullptr};
    bool ecat_connect_flag_{
        false}; // 启动ecat的标志位, 一旦有这个标志位, 就会检查ecat是否连接,
                // 如果未连接就会在线程的某个周期尝试连接
    bool ecat_clear_fault_reenable_flag_{false}; // 清错重新使能标志位

    int stopping_count_ = 0; // 线程退出时的计数器
    const int stopping_count_max_ =
        2000; // 线程退出时计数器最大值, 超过此值就直接退出

    // 每周期结束获取状态机的状态, 并缓存到:
    MotionInfo info_cache_;
#ifdef EDM_MOTION_INFO_GET_USE_ATOMIC
    std::atomic<MotionInfo> at_info_cache_;
#else  // EDM_MOTION_INFO_GET_USE_ATOMIC
    mutable std::mutex info_cache_mutex_;
#endif // EDM_MOTION_INFO_GET_USE_ATOMIC

    // Thread 相关
    pthread_t thread_;
    bool thread_exit_ = false;
    std::atomic_bool thread_stop_flag_{false}; // 用于指示线程退出
    const int64_t cycletime_ns_{
        SystemSettings::instance().get_motion_cycle_us() * 1000}; // 周期(ns)
#ifdef EDM_ECAT_DRIVER_SOEM
    int64_t toff_{0}; // 每周期cycletime的修正量(ns) (基于DC同步)
#endif                // EDM_ECAT_DRIVER_SOEM

    int32_t latency_target_fd_{-1};
    const int32_t latency_target_value_{0}; // 消除系统时钟偏移(禁止电源休眠)

    // dc 基于从站同步 (Master sync to slave 0) 需要使用的变量
    /** Note: 使用ns直接表示MonoClock是可行的, MonoClock基于系统启动时间,
     * 需要584.94年才会溢出 */
    /** systime 含义: 基于systime_base_ns_的monoclock绝对时间 (即monoclock -
     * systime_base_ns_) */
    /**               systime_base_ns_会根据dc进行调整,
     * 从而对每周期的wakeup_systime_ns_进行调整 */
    int64_t systime_base_ns_{0}; // 系统时间基准 (dc动态调整此系统时间基准)
    int64_t wakeup_systime_ns_{0}; // 每周期的唤醒时间

#ifdef EDM_ECAT_DRIVER_IGH
    int64_t dc_systime_ns_{0}; // 在send pdo之前获取的当前系统时间

    int op_wait_count_ = 0; // 等待OP计数
    const int op_wait_count_max_{
        (int)SystemSettings::instance().get_igh_op_wait_count_max()};
#if (EDM_ECAT_DRIVER_IGH_DC_MODE == EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0)
    int32_t dc_diff_ns_{0};
    int32_t prev_dc_diff_ns_{0};
    int64_t dc_diff_total_ns_{0};
    int64_t dc_delta_total_ns_{0};
    int64_t dc_adjust_ns_{0};
    int32_t dc_filter_idx_{0};
    const int32_t dc_filter_cnt{
        (int32_t)SystemSettings::instance().get_dc_filter_cnt()};
#endif // EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0
#endif // EDM_ECAT_DRIVER_IGH

    // 将ns转换为timespec (MonoClock时间的ns需要584年溢出uint64_t)
    static inline void ns2timespec(uint64_t ns, struct timespec *output) {
        output->tv_sec = ns / (uint64_t)1000000000;
        output->tv_nsec = ns % (uint64_t)1000000000;
    }

    // 将timespec转换为ns (MonoClock时间的ns需要584年溢出uint64_t)
    static inline uint64_t timespec2ns(const struct timespec *time_spec) {
        return (uint64_t)time_spec->tv_sec * 1000000000 + time_spec->tv_nsec;
    }

    // 获取基于 systime_base_ns_ 的时间 systime (ns)
    int64_t _get_systime_ns() const;

    // 从 systime(ns) 转换到 绝对MonoClock时钟(ns)
    int64_t _system2mono(uint64_t systime_ns) const;

    void _sync_distributed_clocks();

    void _update_master_clock();

    void _wait_peroid();

private:
    // Thread周期相关测试数据
    util::LongPeroidAverager<int32_t> latency_averager_;
    uint32_t latency_warning_count_{};

    util::TimeUseStatistic total_time_statistic_;
    util::TimeUseStatistic ecat_time_statistic_;
    util::TimeUseStatistic info_time_statistic_;
    util::TimeUseStatistic statemachine_time_statistic_;

private: // 运动状态机
    MotionStateMachine::ptr motion_state_machine_;

private: // 外部的一些回调
    // 抬刀使能电源位, 该函数应当创建一个操作PowerController的命令,
    // 并push入全局命令列表执行
    std::function<void(bool)> cb_enable_votalge_gate_; // TODO, 外部传入初始化

    // 高频操作回调
    std::function<void(bool)> cb_mach_on_;
};

} // namespace move

} // namespace edm
