#include "MotionThreadController.h"
#include "EcatManager/ServoDevice.h"

#include "Exception/exception.h"
#include "Motion/MotionThread/MotionCommand.h"
#include "Motion/MoveDefines.h"
#include "QtDependComponents/ZynqConnection/ZynqUdpMessageHolder.h"
#include "Utils/Format/edm_format.h"
#include "Utils/Time/TimeUseStatistic.h"
#include "config.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>

#ifdef EDM_ECAT_DRIVER_SOEM
#include "ethercat.h"
#endif // EDM_ECAT_DRIVER_SOEM

#define NSEC_PER_SEC   (1000000000L)
#define TIMESPEC2NS(T) ((uint64_t)(T).tv_sec * NSEC_PER_SEC + (T).tv_nsec)
#ifdef EDM_ECAT_DRIVER_IGH
#include "ecrt.h"
#endif

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

MotionThreadController::MotionThreadController(
    std::string_view ifname, MotionCommandQueue::ptr motion_cmd_queue,
    MotionSignalQueue::ptr motion_signal_queue, const MotionCallbacks &cbs,
// const std::function<void(bool)> &cb_enable_voltage_gate,
// const std::function<void(bool)> &cb_mach_on,
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    zynq::ZynqUdpMessageHolder::ptr zynq_udpmessage_holder,
#else
    CanReceiveBuffer::ptr can_recv_buffer,
#endif
    uint32_t iomap_size, uint32_t servo_num, uint32_t io_num)
    : motion_cmd_queue_(motion_cmd_queue),
      motion_signal_queue_(motion_signal_queue), cbs_(cbs)
//   cb_enable_votalge_gate_(cb_enable_voltage_gate), cb_mach_on_(cb_mach_on)
{
    //! 需要注意的是, 构造函数中的代码运行在Caller线程, 不运行在新的线程
    //! 所以线程要最后创建, 防止数据竞争, 和使用未初始化成员变量的问题

    ecat_manager_ = std::make_shared<ecat::EcatManager>(ifname, iomap_size,
                                                        servo_num, io_num);
    if (!ecat_manager_) {
        s_logger->critical("MotionThreadController ecat_manager create failed");
        throw exception("MotionThreadController ecat_manager create failed");
    }

    signal_buffer_ = std::make_shared<SignalBuffer>();
    s_motion_shared->set_signal_buffer(signal_buffer_);

    //! 初始化公共数据的can buffer, ecat_manager_
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    s_motion_shared->set_zynq_udpmessage_holder(zynq_udpmessage_holder);
#else
    s_motion_shared->set_can_recv_buffer(can_recv_buffer);
#endif
    s_motion_shared->set_ecat_manager(ecat_manager_);

    //! 创建motion状态机

    motion_state_machine_ =
        std::make_shared<MotionStateMachine>(signal_buffer_, cbs_);
    if (!motion_state_machine_) {
        s_logger->critical("MotionStateMachine create failed");
        throw exception("MotionStateMachine create failed");
    }

    // ecat_manager_->connect_ecat(1);

    // latency_averager_ = std::make_shared<util::LongPeroidAverager<int32_t>>(
    //     [](int max) { s_logger->warn("latency_averager_ max: {}", max); });

    //! 线程最后创建, 在成员变量都初始化完毕后再创建
    if (!_create_thread()) {
        s_logger->critical("Motion Thread Create Failed");
        throw exception("Motion Thread Create Failed");
    }
}

MotionThreadController::~MotionThreadController() {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    // 停止线程
    thread_stop_flag_ = true;

    // 等待线程退出
    pthread_join(this->thread_, NULL);

    if (latency_target_fd_ >= 0)
        close(latency_target_fd_);
}

void *MotionThreadController::_ThreadEntry(void *mtc) {
    auto controller = static_cast<MotionThreadController *>(mtc);

    s_logger->trace("MotionThreadController _ThreadEntry enter.");

    void *ret = controller->_run();

    s_logger->trace("MotionThreadController _ThreadEntry exit.");

    return ret;
}

/* add ns to timespec */
static void add_timespec(struct timespec *ts, int64_t addtime) {
    int64_t sec, nsec;

    nsec = addtime % NSEC_PER_SEC;
    sec = (addtime - nsec) / NSEC_PER_SEC;
    ts->tv_sec += sec;
    ts->tv_nsec += nsec;
    if (ts->tv_nsec >= NSEC_PER_SEC) {
        nsec = ts->tv_nsec % NSEC_PER_SEC;
        ts->tv_sec += (ts->tv_nsec - nsec) / NSEC_PER_SEC;
        ts->tv_nsec = nsec;
    }
}

/* PI calculation to get linux time synced to DC time */
static void ec_sync(int64_t reftime, int64_t cycletime, int64_t *offsettime) {
    static int64_t integral = 0;
    int64_t delta;
    /* set linux sync point 50us later than DC sync, just as example */
    delta = (reftime - 50000) % cycletime;
    if (delta > (cycletime / 2)) {
        delta = delta - cycletime;
    }
    if (delta > 0) {
        integral++;
    }
    if (delta < 0) {
        integral--;
    }
    *offsettime = -(delta / 100) - (integral / 20);
    //    gl_delta = delta;
}

static inline int64_t calcdiff_ns(const timespec &t1, const timespec &t2) {
    int64_t diff = NSEC_PER_SEC * (int64_t)((int)t1.tv_sec - (int)t2.tv_sec);
    diff += ((int)t1.tv_nsec - (int)t2.tv_nsec);
    return diff;
}

void MotionThreadController::_thread_cycle_work() {
    switch (thread_state_) {
    default:
    case ThreadState::Init:
        _switch_thread_state(ThreadState::Running);
        break;
    case ThreadState::Running: {
        _threadstate_running();

        if (thread_stop_flag_) {
            _switch_thread_state(ThreadState::Stopping); //! 准备退出线程
        }
        break;
    }
    case ThreadState::Stopping: {
        _threadstate_stopping();
        break;
    }
    case ThreadState::CanExit: {
        thread_exit_ = true; // 退出循环
        break;
    }
    }

    //! 在最外层 返回info, 以及处理要返回的信号

    // 周期的最后, 处理命令, 处理info cache
    TIMEUSESTAT(info_time_statistic_,
                _fetch_command_and_handle_and_copy_info_cache(),
                thread_state_ == ThreadState::Running &&
                    ecat_state_ == EcatState::EcatReady);
    // _fetch_command_and_handle_and_copy_info_cache();

    // handle signal and clear signal buffer
    _handle_signal();
}

bool MotionThreadController::_create_thread() {

    // TODO set_latency_target
    if (!_set_cpu_dma_latency()) {
        s_logger->warn("MotionThreadController _set_cpu_dma_latency failed.");
    }

    struct sched_param param;
    pthread_attr_t attr;
    int ret;

#ifndef EDM_OFFLINE_NO_REALTIME_THREAD
    /* Lock memory */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        s_logger->critical("MotionThreadController mlockall failed: {}",
                           strerror(errno));
        // exit(-2);
        return false;
    }
#endif // EDM_OFFLINE_NO_REALTIME_THREAD

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        s_logger->critical("init pthread attributes failed: {}", ret);
        return false;
    }

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, EDM_MOTION_THREAD_STACK * 2);
    if (ret) {
        s_logger->critical("pthread setstacksize failed: {}", ret);
        return false;
    }

#ifndef EDM_OFFLINE_NO_REALTIME_THREAD
    /* Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        s_logger->critical("pthread setschedpolicy failed: {}", ret);
        return false;
    }
    param.sched_priority = 99;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        s_logger->critical(
            "pthread setschedparam failed, priority: {}, ret: {}",
            param.sched_priority, ret);
        return false;
    }

    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        s_logger->critical("pthread setinheritsched failed: {}", ret);
        return false;
    }

    /* Set cpu affinity */
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(2, &mask);
    CPU_SET(3, &mask);
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
    if (ret) {
        s_logger->critical("pthread_attr_setaffinity_np failed: {}", ret);
        return false;
    }
#endif // EDM_OFFLINE_NO_REALTIME_THREAD

    /* Create a pthread with specified attributes */
    ret = pthread_create(&this->thread_, &attr,
                         MotionThreadController::_ThreadEntry,
                         static_cast<void *>(this)); //! 传入this 指针
    if (ret) {
        s_logger->critical("create pthread failed: {}", ret);
        return false;
    }

    return true;
}

void *MotionThreadController::_run() {

#ifdef EDM_ECAT_DRIVER_IGH
    // init dc_systime_ns_
    dc_systime_ns_ = _get_systime_ns();
    s_logger->debug("dc_systime_ns_ init: {}", dc_systime_ns_);
#endif // EDM_ECAT_DRIVER_IGH

    // init wakeup_systime_ns_
    wakeup_systime_ns_ = _get_systime_ns() + 100 * cycletime_ns_;
    s_logger->debug("wakeup_systime_ns_ init: {}", wakeup_systime_ns_);

#ifdef EDM_ECAT_DRIVER_SOEM
    ec_send_processdata();
#endif // EDM_ECAT_DRIVER_SOEM

    while (!thread_exit_) {

#ifndef EDM_OFFLINE_RUN_NO_ECAT
#ifdef EDM_ECAT_DRIVER_SOEM
        if (!ecat_manager_->is_ecat_connected()) {
            toff_ = 0; // 未连接, toff是不对的值
        }
#endif // EDM_ECAT_DRIVER_SOEM
#endif // EDM_OFFLINE_RUN_NO_ECAT

        // 等待下一个周期唤醒
        _wait_peroid();

        // 线程计数
        s_motion_shared->add_thread_tick();

        // while (true) {
        //     struct timespec ttt;
        //     clock_gettime(CLOCK_MONOTONIC, &ttt);

        //     if (ttt.tv_sec >= next_.tv_sec && ttt.tv_nsec > next_.tv_nsec) {
        //         auto t = calcdiff_ns(ttt, this->next_);
        //         if (t > 0)
        //             latency_averager_->push(t);

        //         next_ = ttt;
        //         break;
        //     }
        // }

        // 这一周期理论的周期开始时间: ts
        // 这一周期实际的开始时间: temp_curr_ts > ts (os保证)

#if 0
#ifndef EDM_OFFLINE_NO_REALTIME_THREAD
        // 统计时防止上一周期因为在连接而导致时长超时
        struct timespec temp_curr_ts;
        clock_gettime(CLOCK_MONOTONIC, &temp_curr_ts);
        auto t = calcdiff_ns(temp_curr_ts, this->next_);
        if (t > 0 && ecat_time_statistic_.averager().latest() < cycletime_ns_) {
            latency_averager_.push(t);

            if (t > 45000) {
                // 超过45us的延迟, 累加警告计数
                ++latency_warning_count_;
            }
        }

        //! 防止固定间隔的时间戳跟不上 // TODO 后续细看DC时间同步
        if (t > cycletime_ns_) {
            s_logger->warn("t {} > cycletime_ns_; toff {}, last ecat: {}, last "
                           "total: {}, last info: {}, last sm: {}",
                           t, toff_, ecat_time_statistic_.averager().latest(),
                           total_time_statistic_.averager().latest(),
                           info_time_statistic_.averager().latest(),
                           statemachine_time_statistic_.averager().latest());
            s_logger->warn("last next: {}, {}", test_last_next.tv_sec,
                           test_last_next.tv_nsec);
            s_logger->warn("next: {}, {}", next_.tv_sec, next_.tv_nsec);
            s_logger->warn("now: {}, {}", temp_curr_ts.tv_sec,
                           temp_curr_ts.tv_nsec);
            // add_timespec(&this->next_, cycletime_ns_);
            this->next_ = temp_curr_ts;
        }
#endif // EDM_OFFLINE_NO_REALTIME_THREAD
#endif

        TIMEUSESTAT(total_time_statistic_, _thread_cycle_work(),
                    thread_state_ == ThreadState::Running &&
                        ecat_state_ == EcatState::EcatReady);
    }

    return NULL;
}

void MotionThreadController::_threadstate_stopping() {
#ifndef EDM_OFFLINE_RUN_NO_ECAT
    ++stopping_count_;
    if (stopping_count_ > stopping_count_max_) {
        s_logger->warn("stopping count overflow: {} > {}", stopping_count_,
                       stopping_count_max_);
        ecat_manager_->disconnect_ecat();
        _switch_thread_state(ThreadState::CanExit);
        return;
    }

    if (!ecat_manager_->is_ecat_connected()) {
        // ecat_manager_->disconnect_ecat();
        _switch_thread_state(ThreadState::CanExit);
        return;
    }

    if (ecat_manager_->servo_all_disabled()) {
        ecat_manager_->disconnect_ecat(); //! 一定要调用, 不然驱动器报警
        _switch_thread_state(ThreadState::CanExit);
        return;
    }

    _ecat_sync_wrapper([this]() { ecat_manager_->disable_cycle_run_once(); });
#else  // EDM_OFFLINE_RUN_NO_ECAT
    _switch_thread_state(ThreadState::CanExit);
#endif // EDM_OFFLINE_RUN_NO_ECAT
}

void MotionThreadController::_threadstate_running() {
    switch (ecat_state_) {
    default:
    case EcatState::Init:
        _switch_ecat_state(EcatState::EcatDisconnected);
        // ecat_connect_flag_ = true;
        // ecat_clear_fault_reenable_flag_ = true;
        break;
    case EcatState::EcatDisconnected:
        if (ecat_connect_flag_) {
            _switch_ecat_state(EcatState::EcatConnecting);
            ecat_connect_flag_ = false;
        }
        break;
    case EcatState::EcatConnecting: {
        ecat_connect_flag_ = false;

#ifdef EDM_OFFLINE_RUN_NO_ECAT
        _switch_ecat_state(EcatState::EcatConnectedWaitingForOP);
#else // EDM_OFFLINE_RUN_NO_ECAT
        bool ret = ecat_manager_->connect_ecat(1);
        if (ret) {
            _switch_ecat_state(EcatState::EcatConnectedWaitingForOP);
#ifdef EDM_ECAT_DRIVER_IGH
            op_wait_count_ = 0;
#endif // EDM_ECAT_DRIVER_IGH
       // wakeup_systime_ns_ = _get_systime_ns() + cycletime_ns_ * 50;
        } else {
            _switch_ecat_state(EcatState::EcatDisconnected);
        }

#ifdef EDM_ECAT_DRIVER_SOEM
        ec_send_processdata();
#endif // EDM_ECAT_DRIVER_SOEM

#endif // EDM_OFFLINE_RUN_NO_ECAT
        break;
    }
    case EcatState::EcatConnectedWaitingForOP: {
        bool op_reached{false};

        // _switch_ecat_state(EcatState::EcatConnectedNotAllEnabled);
        // break;
#ifdef EDM_OFFLINE_RUN_NO_ECAT
        op_reached = true;
#else // EDM_OFFLINE_RUN_NO_ECAT

#ifdef EDM_ECAT_DRIVER_SOEM
        op_reached = true;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
        // 在规定的时间内检查是否到达OP
        ++op_wait_count_;
        if (op_wait_count_ > op_wait_count_max_) {
            s_logger->error("igh: wait for op timeout");
            _switch_ecat_state(EcatState::EcatDisconnected);
            ecat_manager_->disconnect_ecat();
            break;
        }

        _ecat_sync_wrapper([&, this]() -> void {
            op_reached = ecat_manager_->igh_check_op();
        });
#endif // EDM_ECAT_DRIVER_IGH

#endif // EDM_OFFLINE_RUN_NO_ECAT

        if (op_reached) {
            _switch_ecat_state(EcatState::EcatConnectedNotAllEnabled);
            wakeup_systime_ns_ = _get_systime_ns() + cycletime_ns_ * 1;
            s_logger->info("op reached");
        }
        break;
    }
    case EcatState::EcatConnectedNotAllEnabled: {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
        _ecat_state_switch_to_ready();
#else  // EDM_OFFLINE_RUN_NO_ECAT

        if (ecat_manager_->servo_all_operation_enabled()) {
            _ecat_state_switch_to_ready();
            ecat_clear_fault_reenable_flag_ = false;
            break;
        }

        if (ecat_clear_fault_reenable_flag_) {
            _switch_ecat_state(EcatState::EcatConnectedEnabling);
            ecat_clear_fault_reenable_flag_ = false;
            break;
        }
#endif // EDM_OFFLINE_RUN_NO_ECAT
        break;
    }
    case EcatState::EcatConnectedEnabling: {
        ecat_clear_fault_reenable_flag_ = false;
#ifdef EDM_OFFLINE_RUN_NO_ECAT
        _ecat_state_switch_to_ready();
#else // EDM_OFFLINE_RUN_NO_ECAT

        _ecat_sync_wrapper([this]() -> void {
            if (!ecat_manager_->servo_all_operation_enabled()) {
                ecat_manager_->clear_fault_cycle_run_once();
            } else {
                _ecat_state_switch_to_ready();
            }
        });

        if (!ecat_manager_->is_ecat_connected()) {
            s_logger->warn("in EcatConnectedEnabling, ecat disconnected");
            _switch_ecat_state(EcatState::EcatDisconnected);
            ecat_connect_flag_ = false;
            // 重置 运动状态机
            motion_state_machine_->reset();
            break;
        }

#endif // EDM_OFFLINE_RUN_NO_ECAT
        break;
    }
    case EcatState::EcatReady: {
        ecat_clear_fault_reenable_flag_ = false;

#ifndef EDM_OFFLINE_RUN_NO_ECAT

        /** Ecat Sync start*/
        _ecat_sync_wrapper(
            [this]() -> void {
                // const auto &cmd_axis = motion_state_machine_->get_cmd_axis();
                const auto &cmd_axis = s_motion_shared->get_global_cmd_axis();

                for (int i = 0; i < cmd_axis.size(); ++i) {
                    const auto device = ecat_manager_->get_servo_device(i);
                    device->set_target_position(static_cast<int32_t>(
                        std::lround(cmd_axis[i]) *
                        s_motion_shared->gear_ratios()[i]));
                    device->cw_enable_operation();
                    device->set_operation_mode(OM_CSP);

                    if (device->type() ==
                        ecat::ServoType::Panasonic_A5B_WithVOffset) {
                        // 设置速度偏置
                        auto v_offset =
                            s_motion_shared->get_global_v_offsets()[i];
                        if (s_motion_shared
                                ->get_global_v_offsets_forced_zero()[i]) {
                            v_offset = 0; // 强制为0
                        }
                        auto with_voffset_device = std::static_pointer_cast<
                            ecat::PanasonicServoDeviceWithVOffset>(device);
                        with_voffset_device->set_v_offset(
                            v_offset); // 设置速度偏置
                    }
                }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
                const auto spindle_device =
                    ecat_manager_->get_servo_device(EDM_DRILL_SPINDLE_AXIS_IDX);
                spindle_device->set_target_position(static_cast<int32_t>(
                    s_motion_shared->get_spindle_controller()->current_axis()));
                spindle_device->cw_enable_operation();
                spindle_device->set_operation_mode(OM_CSP);
#endif
            },
            true);

        /** Ecat sync over */

        // 先检查驱动器情况
        if (!ecat_manager_->is_ecat_connected()) {
            s_logger->warn("in EcatReady, ecat disconnected");
            _switch_ecat_state(EcatState::EcatDisconnected);
            // 重置 运动状态机
            motion_state_machine_->reset();
            break;
        }

        if (!ecat_manager_->servo_all_operation_enabled() && false) {
            // s_logger->debug(
            //     "{:08b}",
            //     ecat_manager_->get_servo_device(0)->get_status_word());

            // for (int i = 0; i < 7; ++i) {
            //     auto d = ecat_manager_->get_servo_device(i);
            //     s_logger->debug("servo {}: sw: {:08b}", i,
            //                     d->get_status_word());
            // }

            s_logger->warn("in EcatReady, ecat not all enabled");
            _switch_ecat_state(EcatState::EcatConnectedNotAllEnabled);
            // 重置 运动状态机
            motion_state_machine_->reset();
            break;
        }
#endif // EDM_OFFLINE_RUN_NO_ECAT

        // StateMachine Operate Cycle
        TIMEUSESTAT(statemachine_time_statistic_,
                    motion_state_machine_->run_once(), true);

        break;
    }
    }
}

void MotionThreadController::_ecat_state_switch_to_ready() {
    _switch_ecat_state(EcatState::EcatReady);
    motion_state_machine_->reset();

    axis_t act_axis;
    auto ret = s_motion_shared->get_act_axis(act_axis);
    if (!ret) {
        throw exception(
            EDM_FMT::format("s_motion_shared->get_act_axis failed, in {}",
                            __PRETTY_FUNCTION__));
    }

    axis_t cmd_axis;
    MotionUtils::ClearAxis(cmd_axis);
    for (int i = 0; i < cmd_axis.size(); ++i) {
        cmd_axis[i] = act_axis[i];
        s_logger->info("init axis: servo {}: act: {}, cmd: {}", i, act_axis[i],
                       cmd_axis[i]);
    }

    s_motion_shared->set_global_cmd_axis(cmd_axis);

#ifndef EDM_OFFLINE_RUN_NO_ECAT
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    int spindle_act_pulse = this->ecat_manager_->get_servo_actual_position(
        EDM_DRILL_SPINDLE_AXIS_IDX);
    double spindle_act_pos =
        (double)spindle_act_pulse /
        s_motion_shared->gear_ratios()[EDM_DRILL_SPINDLE_AXIS_IDX];

    s_motion_shared->get_spindle_controller()->init_current_axis(
        spindle_act_pos);
#endif
#endif

    motion_state_machine_->set_enable(true);

    // 重新统计数
    latency_averager_.clear();

    total_time_statistic_.clear();
    ecat_time_statistic_.clear();
    info_time_statistic_.clear();
    statemachine_time_statistic_.clear();
}

bool MotionThreadController::_set_cpu_dma_latency() {
    /* 消除系统时钟偏移函数，取自cyclic_test */
    struct stat s;
    int ret;

    if (stat("/dev/cpu_dma_latency", &s) == 0) {
        latency_target_fd_ = open("/dev/cpu_dma_latency", O_RDWR);
        if (latency_target_fd_ == -1) {
            s_logger->error("open /dev/cpu_dma_latency failed");
            return false;
        }

        ret = write(latency_target_fd_, &latency_target_value_, 4);
        if (ret <= 0) {
            s_logger->error("# error setting cpu_dma_latency to {}!: {}\n",
                            latency_target_value_, strerror(errno));
            close(latency_target_fd_);
            return false;
        }

        //! 不可以在这里关闭!!, 不然就直接无效了
        // close(latency_target_fd_);
        s_logger->info("# /dev/cpu_dma_latency {} set to {}us\n",
                       latency_target_fd_, latency_target_value_);
        return true;
    } else {
        return false;
    }
}

void MotionThreadController::_fetch_command_and_handle_and_copy_info_cache() {
    auto cmd_opt = motion_cmd_queue_->try_get_command();
    if (!cmd_opt) {
        // 没有命令, 只是拷贝info cache
        // TIMEUSESTAT(info_time_statistic_, _copy_info_cache(), true);
        _copy_info_cache();

        return;
    }

    auto cmd = *cmd_opt;

    // s_logger->debug("cmd type: {}", (int)cmd->type());

    bool accept_cmd_flag = false;

    // TODO handle cmd
    switch (cmd->type()) {
    case MotionCommandManual_StartPointMove: {
        s_logger->trace("Handle MotionCmd: Manual_StartPointMove");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }

        auto spm_cmd =
            std::static_pointer_cast<MotionCommandManualStartPointMove>(cmd);

        auto ret = motion_state_machine_->start_manual_pointmove(
            spm_cmd->speed_param(), spm_cmd->end_pos());

        // 接触感知开启设定 //! 注意在点动结束后关闭接触感知
        if (ret) {
            motion_state_machine_->get_touch_detect_handler()
                ->set_detect_enable(spm_cmd->touch_detect_enable());
        }

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommandManual_StopPointMove: {
        s_logger->trace("Handle MotionCmd: Manual_StopPointMove");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }

        auto spm_cmd =
            std::static_pointer_cast<MotionCommandManualStopPointMove>(cmd);

        auto ret =
            motion_state_machine_->stop_manual_pointmove(spm_cmd->immediate());

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommandAuto_StartG00FastMove: {
        s_logger->trace("Handle MotionCmd: Auto_StartG00FastMove");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }

        auto g00_cmd =
            std::static_pointer_cast<MotionCommandAutoStartG00FastMove>(cmd);

        auto ret = motion_state_machine_->start_auto_g00(
            g00_cmd->speed_param(), g00_cmd->end_pos(),
            g00_cmd->touch_detect_enable());
        
        if (!ret) {
            s_logger->warn("motion: start auto g00 failed");
        }

        accept_cmd_flag = ret;

        break;
    }
    case MotionCommandAuto_StartG01ServoMove: {
        s_logger->trace("Handle MotionCmd: Auto_StartG01ServoMove");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }

        auto g01_cmd =
            std::static_pointer_cast<MotionCommandAutoStartG01ServoMove>(cmd);

        auto ret = motion_state_machine_->start_auto_g01(
            g01_cmd->end_pos(), g01_cmd->max_jump_height_from_begin());

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommandAuto_G04Delay: {
        s_logger->trace("Handle MotionCmd: Auto_G04Delay");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }

        auto g04_cmd = std::static_pointer_cast<MotionCommandAutoG04Delay>(cmd);

        auto ret = motion_state_machine_->start_auto_g04(g04_cmd->delay_s());

        accept_cmd_flag = ret;

        break;
    };
    case MotionCommandAuto_M00FakePauseTask: {
        s_logger->trace("Handle MotionCmd: Auto_M00FakePauseTask");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }
        auto ret = motion_state_machine_->start_auto_m00fake();

        accept_cmd_flag = ret;

        break;
    }
    case MotionCommandAuto_Stop: {
        s_logger->trace("Handle MotionCmd: Auto_Stop");
        auto autostop_cmd =
            std::static_pointer_cast<MotionCommandAutoStop>(cmd);
        auto ret = motion_state_machine_->stop_auto(autostop_cmd->immediate());

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommandAuto_Pause: {
        s_logger->trace("Handle MotionCmd: Auto_Pause");
        auto ret = motion_state_machine_->pause_auto();

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommandAuto_Resume: {
        s_logger->trace("Handle MotionCmd: Auto_Resume");
        auto ret = motion_state_machine_->resume_auto();

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommandManual_EmergencyStopAllMove: {
        s_logger->trace("Handle MotionCmd: EmergencyStopAllMove");
        motion_state_machine_->reset();
        motion_state_machine_->set_enable(true);

        accept_cmd_flag = true;
        break;
    }
    case MotionCommandSetting_TriggerEcatConnect: {
        s_logger->trace("Handle MotionCmd: Setting_TriggerEcatConnect");
        ecat_connect_flag_ = true;
        ecat_clear_fault_reenable_flag_ = true;

        accept_cmd_flag = true;
        break;
    }
    case MotionCommandSetting_ClearWarning: {
        s_logger->trace("Handle MotionCmd: Setting_ClearWarning");

        // 目前只有接触感知报警
        motion_state_machine_->get_touch_detect_handler()->clear_warning();

        accept_cmd_flag = true;
        break;
    }
    case MotionCommandSetting_ClearStatData: {

        info_time_statistic_.clear();
        ecat_time_statistic_.clear();
        total_time_statistic_.clear();
        statemachine_time_statistic_.clear();

        latency_averager_.clear();
        latency_warning_count_ = 0;

        accept_cmd_flag = true;

        break;
    }
    case MotionCommandSetting_SetJumpParam: {
        s_logger->trace("Handle MotionCmd: Setting_SetJumpParam");

        auto set_jump_param_cmd =
            std::static_pointer_cast<MotionCommandSettingSetJumpParam>(cmd);

        s_motion_shared->set_jump_param(set_jump_param_cmd->jump_param());

        s_logger->debug("***** JumpParam Received");

        accept_cmd_flag = true;
        break;
    }
    case MotionCommandSetting_MotionSettings: {
        s_logger->trace("Handle MotionCmd: Setting_MotionSettings");

        auto set_motion_settings_cmd =
            std::static_pointer_cast<MotionCommandSettingMotionSettings>(cmd);

        s_motion_shared->set_settings(
            set_motion_settings_cmd->motion_settings());

        const auto &settings = set_motion_settings_cmd->motion_settings();
        s_logger->debug("motion_settings: {}, {}, {}, {}",
                        settings.enable_g01_run_each_servo_cmd,
                        settings.enable_g01_half_closed_loop,
                        settings.enable_g01_servo_with_dynamic_strategy,
                        settings.g01_servo_dynamic_strategy_type);

        accept_cmd_flag = true;
        break;
    }
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    case MotionCommand_SetSpindleState: {
        s_logger->trace("Handle MotionCmd: SetSpindleState");

        auto set_spindle_state_cmd =
            std::static_pointer_cast<MotionCommandSetSpindleState>(cmd);

        if (set_spindle_state_cmd->spindle_start()) {
            s_motion_shared->get_spindle_controller()->start_spindle();
        } else {
            s_motion_shared->get_spindle_controller()->stop_spindle();
        }

        accept_cmd_flag = true;
        break;
    }
    case MotionCommand_SetSpindleParam: {
        s_logger->trace("Handle MotionCmd: SetSpindleParam");

        auto set_spindle_param_cmd =
            std::static_pointer_cast<MotionCommandSetSpindleParam>(cmd);

        if (set_spindle_param_cmd->speed_blu_ms_opt()) {
            s_motion_shared->get_spindle_controller()
                ->set_spindle_target_speed_blu_ms(
                    set_spindle_param_cmd->speed_blu_ms_opt().value());
        }

        if (set_spindle_param_cmd->acc_blu_ms_opt()) {
            s_motion_shared->get_spindle_controller()
                ->set_spindle_max_acc_blu_ms(
                    set_spindle_param_cmd->acc_blu_ms_opt().value());
        }

        accept_cmd_flag = true;
        break;
    }
    case MotionCommandAuto_StartDrillMove: {
        s_logger->trace("Handle MotionCmd: Auto_StartDrillMove");

        auto drill_cmd =
            std::static_pointer_cast<MotionCommandAutoStartDrillMove>(cmd);

        const auto &start_param = drill_cmd->start_params();
        if (start_param.spindle_speed_blu_ms_opt) {
            s_motion_shared->get_spindle_controller()
                ->set_spindle_target_speed_blu_ms(
                    start_param.spindle_speed_blu_ms_opt.value());
            s_logger->debug("***** Spindle Speed Set: {}",
                            start_param.spindle_speed_blu_ms_opt.value());
        }
        s_logger->debug("打孔了哦, depth_um: {}, holdtime_ms: {}, touch: {}, "
                        "breakout: {}, back: {}",
                        start_param.depth_um, start_param.holdtime_ms,
                        start_param.touch, start_param.breakout,
                        start_param.back);

        // 开启打孔任务
        bool ret =
            motion_state_machine_->start_auto_drill(drill_cmd->start_params());

        accept_cmd_flag = ret;
        break;
    }
    case MotionCommand_SetDrillParams: {
        s_logger->trace("Handle MotionCmd: SetDrillParams");

        auto set_drill_params_cmd =
            std::static_pointer_cast<MotionCommandSetDrillParams>(cmd);

        s_motion_shared->set_drill_params(set_drill_params_cmd->drill_params());

        s_logger->debug("***** DrillParams Received");
        const DrillParams &params = set_drill_params_cmd->drill_params();
        s_logger->debug("DrillParams: touch_return_um={}, touch_speed_um_ms={}",
                        params.touch_return_um, params.touch_speed_um_ms);
        s_logger->debug(
            "DrillBreakOutParams: voltage_average_filter_window_size={}, "
            "stderr_filter_window_size={}, kn_valid_threshold={}, "
            "kn_sc_window_size={}, kn_valid_rate_threshold={}, "
            "kn_valid_rate_ok_cnt_threshold={}, "
            "kn_valid_rate_ok_cnt_maximum={}, "
            "max_move_um_after_breakout_start_detected={}, "
            "breakout_start_detect_length_percent={}, "
            "speed_rate_after_breakout_start_detected={}, "
            "wait_time_ms_after_breakout_end_judged={}, ctrl_flags={}",
            params.breakout_params.voltage_average_filter_window_size,
            params.breakout_params.stderr_filter_window_size,
            params.breakout_params.kn_valid_threshold,
            params.breakout_params.kn_sc_window_size,
            params.breakout_params.kn_valid_rate_threshold,
            params.breakout_params.kn_valid_rate_ok_cnt_threshold,
            params.breakout_params.kn_valid_rate_ok_cnt_maximum,
            params.breakout_params.max_move_um_after_breakout_start_detected,
            params.breakout_params.breakout_start_detect_length_percent,
            params.breakout_params.speed_rate_after_breakout_start_detected,
            params.breakout_params.wait_time_ms_after_breakout_end_judged,
            params.breakout_params.ctrl_flags);
        accept_cmd_flag = true;
        break;
    }
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    case MotionCommandAuto_G01Group: {
        s_logger->trace("Handle MotionCmd: Auto_G01Group");

        auto g01_group_cmd =
            std::static_pointer_cast<MotionCommandAutoG01Group>(cmd);

        /*
        s_logger->debug("***** G01Group Received");
        s_logger->debug("G01Group: size={}",
        g01_group_cmd->start_param().items.size()); for (int i = 0; i <
        g01_group_cmd->start_param().items.size(); ++i) {
            s_logger->debug("G01Group: item[{}]", i);
            const auto& item = g01_group_cmd->start_param().items[i];
            for (int i = 0; i < item.incs.size(); ++i) {
                s_logger->debug("  inc[{}]: {}", i, item.incs[i]);
            }
            s_logger->debug("  line_number: {}", item.line);
        }
        */

        auto ret = motion_state_machine_->start_auto_g01_group(
            g01_group_cmd->start_param());

        accept_cmd_flag = ret;

        break;
    }
    case MotionCommandSetting_TestVOffset: {
        s_logger->trace("Handle MotionCmd: Setting_TestVOffset");

        auto test_voffset_cmd =
            std::static_pointer_cast<MotionCommandSettingTestVOffset>(cmd);

        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            break;
        }

        // 设置速度偏置
        const auto &v_offset_cmd = test_voffset_cmd->cmd();

        s_logger->debug("***** TestVOffset Received, {}, {}, {}",
                        v_offset_cmd.set_axis_index,
                        (int)v_offset_cmd.set_cmd_type, v_offset_cmd.set_value);

        accept_cmd_flag = true;
        switch (v_offset_cmd.set_cmd_type) {
        case MotionCommandSettingTestVOffset::VOffsetSetCmdType::SetValue: {
            s_logger->debug("SetValue: axis {}, value {}",
                            v_offset_cmd.set_axis_index,
                            v_offset_cmd.set_value);

            if (v_offset_cmd.set_axis_index >=
                s_motion_shared->get_global_v_offsets().size()) {
                s_logger->error("Invalid axis index: {}",
                                v_offset_cmd.set_axis_index);
                break;
            }

            s_motion_shared->get_global_v_offsets().at(
                v_offset_cmd.set_axis_index) = v_offset_cmd.set_value;
            break;
        }
        case MotionCommandSettingTestVOffset::VOffsetSetCmdType::IncValue: {
            s_logger->debug("IncValue: axis {}, value {}",
                            v_offset_cmd.set_axis_index,
                            v_offset_cmd.set_value);

            if (v_offset_cmd.set_axis_index >=
                s_motion_shared->get_global_v_offsets().size()) {
                s_logger->error("Invalid axis index: {}",
                                v_offset_cmd.set_axis_index);
                break;
            }

            s_motion_shared->get_global_v_offsets().at(
                v_offset_cmd.set_axis_index) += v_offset_cmd.set_value;
            break;
        }
        case MotionCommandSettingTestVOffset::VOffsetSetCmdType::ForcedZero: {
            s_logger->debug("ForcedZero: axis {}, value {}",
                            v_offset_cmd.set_axis_index,
                            v_offset_cmd.set_value);

            if (v_offset_cmd.set_axis_index >=
                s_motion_shared->get_global_v_offsets().size()) {
                s_logger->error("Invalid axis index: {}",
                                v_offset_cmd.set_axis_index);
                break;
            }

            s_motion_shared->get_global_v_offsets_forced_zero().at(
                v_offset_cmd.set_axis_index) = (v_offset_cmd.set_value > 0.5);

            break;
        }
        default: {
            s_logger->error("Unsupported VOffsetSetCmdType: {}",
                            (int)v_offset_cmd.set_cmd_type);
            accept_cmd_flag = false;
            break;
        }
        }

        break;
    }
    case MotionCommandSetting_SetG01SpeedRatio: {
        s_logger->trace("Handle MotionCmd: Setting_SetG01SpeedRatio");

        auto set_g01_speed_ratio_cmd =
            std::static_pointer_cast<MotionCommandSettingSetG01SpeedRatio>(cmd);

        s_motion_shared->set_g01_speed_ratio(
            set_g01_speed_ratio_cmd->speed_ratio());

        s_logger->debug("***** G01 Speed Ratio Set: {}",
                        set_g01_speed_ratio_cmd->speed_ratio());

        accept_cmd_flag = true;
        break;
    }
    default:
        s_logger->warn("Unsupported MotionCommandType: {}", (int)cmd->type());
        cmd->ignore();
        break;
    }

    // 在处理完命令之后, accept/ignore之前, 缓存info,
    // 保证外界在发送并等待完指令被accept/ignore后,
    // 直接查询的info一定是正确的
    // TIMEUSESTAT(info_time_statistic_, _copy_info_cache(), true);
    _copy_info_cache();

    if (accept_cmd_flag) {
        cmd->accept();
    } else {
        cmd->ignore();
    }
}

// bool MotionThreadController::_get_act_pos(axis_t &axis) {
// #ifdef EDM_OFFLINE_RUN_NO_ECAT
//     axis = s_motion_shared->get_global_cmd_axis();
//     // axis = motion_state_machine_->get_cmd_axis(); // 离线, 返回指令位置
// #else                                             // EDM_OFFLINE_RUN_NO_ECAT
//     if (!this->ecat_manager_->is_ecat_connected()) {
//         // MotionUtils::ClearAxis(axis);
//         return false;
//     }

//     for (int i = 0; i < axis.size(); ++i) {
//         axis[i] = (double)this->ecat_manager_->get_servo_actual_position(i);
//     }
// #endif                                            // EDM_OFFLINE_RUN_NO_ECAT

//     return true;
// }

void MotionThreadController::_copy_info_cache() {
#ifndef EDM_MOTION_INFO_GET_USE_ATOMIC
    std::lock_guard guard(info_cache_mutex_);
#endif // EDM_MOTION_INFO_GET_USE_ATOMIC

    // bit_state1
    info_cache_.bit_state1 = 0;

    info_cache_.curr_cmd_axis_blu = s_motion_shared->get_global_cmd_axis();
    s_motion_shared->get_act_axis(info_cache_.curr_act_axis_blu);

    // info_cache_.curr_v_offsets_blu = s_motion_shared->get_global_v_offsets();
    for (std::size_t i = 0; i < s_motion_shared->get_global_v_offsets().size();
         ++i) {
        info_cache_.curr_v_offsets_blu[i] =
            (s_motion_shared->get_global_v_offsets_forced_zero().at(i))
                ? 0
                : s_motion_shared->get_global_v_offsets()[i];
    }

    info_cache_.sub_line_number = s_motion_shared->get_sub_line_num();

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    info_cache_.spindle_axis_blu =
        s_motion_shared->get_spindle_controller()->current_axis();
    info_cache_.is_spindle_on =
        s_motion_shared->get_spindle_controller()->is_spindle_on();

    info_cache_.drill_total_blu =
        s_motion_shared->get_current_drill_total_blu();
    info_cache_.drill_remaining_blu =
        s_motion_shared->get_current_drill_remaining_blu();

    const auto breakout_filter = s_motion_shared->get_breakout_filter();
    info_cache_.breakout_data.realtime_voltage =
        breakout_filter->get_last_realtime_voltage();
    info_cache_.breakout_data.averaged_voltage =
        breakout_filter->get_last_averaged_voltage();
    info_cache_.breakout_data.kn = breakout_filter->get_last_stderr();
    info_cache_.breakout_data.kn_valid_rate =
        breakout_filter->get_last_kn_sliding_counter_valid_rate();
    info_cache_.breakout_data.kn_cnt = breakout_filter->get_last_kn_cnt();

    info_cache_.setBreakoutDetected(s_motion_shared->get_breakout_detected());
    info_cache_.setKnDetected(breakout_filter->is_kn_detected());
#endif

    info_cache_.main_mode = motion_state_machine_->main_mode();
    info_cache_.auto_state = motion_state_machine_->auto_state();

    // info_cache_.latency_data.curr_latency =
    // current_cycle_starttime_latency_ns_; info_cache_.latency_data.avg_latency
    // = avg_latency_ns_; info_cache_.latency_data.max_latency =
    // max_latency_ns_; info_cache_.latency_data.min_latency = min_latency_ns_;

    info_cache_.latency_data.curr_latency = latency_averager_.latest();
    info_cache_.latency_data.avg_latency = latency_averager_.average();
    info_cache_.latency_data.max_latency = latency_averager_.max();
    // info_cache_.latency_data.min_latency = latency_averager_.min();
    info_cache_.latency_data.warning_count = latency_warning_count_;

    info_cache_.time_use_data.total_time_use_avg =
        TIMEUSESTAT_AVG(total_time_statistic_);
    info_cache_.time_use_data.total_time_use_max =
        TIMEUSESTAT_MAX(total_time_statistic_);
    info_cache_.time_use_data.info_time_use_avg =
        TIMEUSESTAT_AVG(info_time_statistic_);
    info_cache_.time_use_data.info_time_use_max =
        TIMEUSESTAT_MAX(info_time_statistic_);
    info_cache_.time_use_data.ecat_time_use_avg =
        TIMEUSESTAT_AVG(ecat_time_statistic_);
    info_cache_.time_use_data.ecat_time_use_max =
        TIMEUSESTAT_MAX(ecat_time_statistic_);
    info_cache_.time_use_data.statemachine_time_use_avg =
        TIMEUSESTAT_AVG(statemachine_time_statistic_);
    info_cache_.time_use_data.statemachine_time_use_max =
        TIMEUSESTAT_MAX(statemachine_time_statistic_);

#ifdef EDM_OFFLINE_RUN_NO_ECAT
    info_cache_.setEcatConnected(ecat_state_ > EcatState::EcatConnecting);
#else  // EDM_OFFLINE_RUN_NO_ECAT
    info_cache_.setEcatConnected(ecat_manager_->is_ecat_connected());
#endif // EDM_OFFLINE_RUN_NO_ECAT
    info_cache_.setEcatAllEnabled(ecat_state_ == EcatState::EcatReady);
    info_cache_.setTouchDetectEnabled(
        motion_state_machine_->get_touch_detect_handler()->is_detect_enable());
    info_cache_.setTouchDetected(
        motion_state_machine_->get_touch_detect_handler()->physical_detected());
    info_cache_.setTouchWarning(
        motion_state_machine_->get_touch_detect_handler()->has_warning());

#ifdef EDM_MOTION_INFO_GET_USE_ATOMIC
    // Store To Atomic
    at_info_cache_.store(info_cache_);
#endif // EDM_MOTION_INFO_GET_USE_ATOMIC
}

void MotionThreadController::_handle_signal() {
    if (signal_buffer_->has_signal()) {
        const auto &signal_arr = signal_buffer_->get_signals_arr();
        for (int i = 0; i < signal_arr.size(); ++i) {
            if (signal_arr[i]) {
                MotionSignal signal{static_cast<MotionSignalType>(i),
                                    info_cache_};
                auto ret = motion_signal_queue_->push(signal);
                s_logger->trace("push signal: {}, success? {}", i, ret);
            }
        }
    }
    signal_buffer_->reset_all(); //! clear
}

void MotionThreadController::_ecat_sync_wrapper(
    const std::function<void(void)> &cb, bool run_check) {

    int wkc{-1};
    // 统计接收用时delay
    TIMEUSESTAT(
        ecat_time_statistic_,
        {
#ifdef EDM_ECAT_DRIVER_SOEM
            wkc = ecat_manager_->soem_master_receive();
#endif // EDM_ECAT_DRIVER_SOEM
#ifdef EDM_ECAT_DRIVER_IGH
            ecat_manager_->igh_master_receive();
            ecat_manager_->igh_domain_process();
#endif // EDM_ECAT_DRIVER_IGH

            if (run_check) [[likely]] {
                if (!ecat_manager_->receive_check(wkc)) {
                    return;
                }
            }

            if (cb) [[likely]] {
                cb();
            }

        // 发送的控制指令是上个周期的计算结果
#ifdef EDM_ECAT_DRIVER_IGH
            ecat_manager_->igh_domain_queue();
#endif // EDM_ECAT_DRIVER_IGH

            _sync_distributed_clocks();

#ifdef EDM_ECAT_DRIVER_SOEM
            ecat_manager_->soem_master_send();
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
            ecat_manager_->igh_master_send();
#endif // EDM_ECAT_DRIVER_IGH

            _update_master_clock();
        },
        true);
}

const char *MotionThreadController::GetThreadStateStr(ThreadState s) {
    switch (s) {
#define XX_(v__)           \
    case ThreadState::v__: \
        return #v__;
        XX_(Init)
        XX_(Running)
        XX_(Stopping)
        XX_(CanExit)
#undef XX_
    default:
        return "Unknow";
    }
}

const char *MotionThreadController::GetEcatStateStr(EcatState s) {
    switch (s) {
#define XX_(v__)         \
    case EcatState::v__: \
        return #v__;
        XX_(Init)
        XX_(EcatDisconnected)
        XX_(EcatConnecting)
        XX_(EcatConnectedWaitingForOP)
        XX_(EcatConnectedNotAllEnabled)
        XX_(EcatConnectedEnabling)
        XX_(EcatReady)
#undef XX_
    default:
        return "Unknow";
    }
}

void MotionThreadController::_switch_thread_state(
    ThreadState new_thread_state) {
    s_logger->trace("motion thread state: {} -> {}",
                    GetThreadStateStr(thread_state_),
                    GetThreadStateStr(new_thread_state));
    thread_state_ = new_thread_state;
}

void MotionThreadController::_switch_ecat_state(EcatState new_ecat_state) {
    s_logger->trace("motion thread ecat state: {} -> {}",
                    GetEcatStateStr(ecat_state_),
                    GetEcatStateStr(new_ecat_state));
    ecat_state_ = new_ecat_state;
}

int64_t MotionThreadController::_get_systime_ns() const {
    // get current monoclock abs time
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // convert to ns
    auto now_ns = static_cast<int64_t>(timespec2ns(&now));

    if (now_ns >= systime_base_ns_) [[likely]] {
        // system time is: mono time ref to system_base_time_ns_
        return now_ns - systime_base_ns_;
    } else {
        s_logger->warn("_get_system_time_ns: now_ns {} < systime_base_ns_ {}",
                       now_ns, systime_base_ns_);
        return 0;
    }
}

int64_t MotionThreadController::_system2mono(uint64_t systime_ns) const {
    if (systime_base_ns_ < 0 && ((uint64_t)(-systime_base_ns_) > systime_ns)) {
        s_logger->warn("_system2mono: systime_base_ns_ {}, systime_ns {}",
                       systime_base_ns_, systime_ns);
        return 0;
    } else [[likely]] {
        return systime_ns + systime_base_ns_;
    }
}

void MotionThreadController::_sync_distributed_clocks() {
#ifdef EDM_ECAT_DRIVER_SOEM
    /* calulate toff to get linux time and DC synced */
    //! 对指令速度突变的改善很有效果
    ecat_manager_->soem_dc_sync_time(cycletime_ns_, &toff_);
#endif // EDM_ECAT_DRIVER_IGH

#ifdef EDM_ECAT_DRIVER_IGH
#if (EDM_ECAT_DRIVER_IGH_DC_MODE == EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0)
    uint64_t prev_app_time = dc_systime_ns_;
#endif // EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0

    dc_systime_ns_ = _get_systime_ns();

#if (EDM_ECAT_DRIVER_IGH_DC_MODE == EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0)
    uint32_t ref_time = ecat_manager_->igh_get_reference_clock_time();
    dc_diff_ns_ = (uint32_t)dc_systime_ns_ - ref_time;

    // EDM_CYCLIC_LOG(s_logger->debug, 4000,
    //                "_sync_distributed_clocks: ref: {}, diff: {}", ref_time,
    //                dc_diff_ns_);
#endif // EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0

#if (EDM_ECAT_DRIVER_IGH_DC_MODE == EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_MASTER)
    // sync reference clock to master
    ecat_manager_->igh_sync_reference_clock_to(dc_systime_ns_);
#endif

    // call ecrt_master_sync_slave_clocks()
    ecat_manager_->igh_sync_slave_clocks();
#endif // EDM_ECAT_DRIVER_IGH
}

/** Return the sign of a number
 *
 * ie -1 for -ve value, 0 for 0, +1 for +ve value
 *
 * \retval the sign of the value
 */
#define edm_sign(val)              \
    ({                             \
        typeof(val) _val = (val);  \
        ((_val > 0) - (_val < 0)); \
    })

void MotionThreadController::_update_master_clock() {
#ifdef EDM_ECAT_DRIVER_SOEM
    systime_base_ns_ += toff_;
#endif // EDM_ECAT_DRIVER_SOEM

#ifdef EDM_ECAT_DRIVER_IGH
#if (EDM_ECAT_DRIVER_IGH_DC_MODE == EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0)
    int32_t delta = dc_diff_ns_ - prev_dc_diff_ns_;
    prev_dc_diff_ns_ = dc_diff_ns_;

    // normalise the time diff
    dc_diff_ns_ = ((dc_diff_ns_ + (cycletime_ns_ / 2)) % cycletime_ns_) -
                  (cycletime_ns_ / 2);

    {
        // add to totals
        dc_diff_total_ns_ += dc_diff_ns_;
        dc_delta_total_ns_ += delta;
        dc_filter_idx_++;

        if (dc_filter_idx_ >= dc_filter_cnt) {
            // add rounded delta average
            dc_adjust_ns_ +=
                ((dc_delta_total_ns_ + (dc_filter_cnt / 2)) / dc_filter_cnt);

            // and add adjustment for general diff (to pull in drift)
            dc_adjust_ns_ += edm_sign(dc_diff_total_ns_ / dc_filter_cnt);

            // limit crazy numbers (0.1% of std cycle time)
            if (dc_adjust_ns_ < -1000) {
                dc_adjust_ns_ = -1000;
            } else if (dc_adjust_ns_ > 1000) {
                dc_adjust_ns_ = 1000;
            }

            // reset
            dc_diff_total_ns_ = 0LL;
            dc_delta_total_ns_ = 0LL;
            dc_filter_idx_ = 0;
        }

        // add cycles adjustment to time base (including a spot adjustment)
        systime_base_ns_ += dc_adjust_ns_ + edm_sign(dc_diff_ns_);

        // EDM_CYCLIC_LOG(
        //     s_logger->debug, 4000,
        //     "_update_master_clock: systime_base_ns_: {}, dc_adjust_ns_: {}",
        //     systime_base_ns_, dc_adjust_ns_);
    }
#endif // EDM_ECAT_DRIVER_IGH_DC_SYNC_TO_SLAVE0
#endif // EDM_ECAT_DRIVER_IGH
}

void MotionThreadController::_wait_peroid() {
    // 获取新的 wakeup 时间 (systime不会变, 但是转换到mono时,
    // 由于调整了systime_base基准, 转换出的时间就是经过调整的linux时间了)
    auto wakeup_time_mono_ns = _system2mono(wakeup_systime_ns_);

    struct timespec wakeup_time_timespec, dummy_tleft;
    ns2timespec(wakeup_time_mono_ns, &wakeup_time_timespec);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup_time_timespec,
                    &dummy_tleft);

#ifndef EDM_OFFLINE_RUN_NO_ECAT
#ifdef EDM_ECAT_DRIVER_IGH
    // set master time in nano-seconds
    // call ecrt_master_application_time
    ecat_manager_->igh_tell_application_time(wakeup_systime_ns_);
#endif // EDM_ECAT_DRIVER_IGH
#endif // EDM_OFFLINE_RUN_NO_ECAT

    // 计算唤醒延迟
    {
        // 获取理论唤醒的monotime
        auto wakeup_time_ns = _system2mono(wakeup_systime_ns_);

        // 获取实际时间
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        auto now_ns = timespec2ns(&now);

        // 计算latency
        auto latency = now_ns - wakeup_time_ns;
        if (latency > 0) {
            latency_averager_.push(latency);

            if (latency > 100000) {
                if (thread_state_ == ThreadState::Running &&
                    ecat_state_ == EcatState::EcatReady) {
                    // 超过45us的延迟, 累加警告计数
                    ++latency_warning_count_;
                    s_logger->warn("latency: {}, last_ecat: {}", latency,
                                   TIMEUSESTAT_LATEST(ecat_time_statistic_));
                    s_logger->debug("wakeup_mono: {}, wakeup_sys: {}, "
                                    "sys_base: {}, now_mono: {}",
                                    wakeup_time_ns, wakeup_systime_ns_,
                                    systime_base_ns_, now_ns);
                }
            }

            if (latency > cycletime_ns_) {
                systime_base_ns_ += latency;
            }
        }
    }

    // calc next wake time (in sys time)
    // 下一个wakeup的systime
    wakeup_systime_ns_ += cycletime_ns_;
}

} // namespace move

} // namespace edm