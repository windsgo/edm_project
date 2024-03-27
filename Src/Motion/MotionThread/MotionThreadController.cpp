#include "MotionThreadController.h"
#include "EcatManager/ServoDevice.h"

#include "ethercat.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

MotionThreadController::MotionThreadController(
    std::string_view ifname, MotionCommandQueue::ptr motion_cmd_queue,
    MotionSignalQueue::ptr motion_signal_queue,
    const std::function<void(bool)> &cb_enable_voltage_gate,
    const std::function<void(bool)> &cb_mach_on,
    CanReceiveBuffer::ptr can_recv_buffer, uint32_t iomap_size,
    uint32_t servo_num, uint32_t io_num)
    : motion_cmd_queue_(motion_cmd_queue),
      motion_signal_queue_(motion_signal_queue),
      cb_enable_votalge_gate_(cb_enable_voltage_gate), cb_mach_on_(cb_mach_on),
      can_recv_buffer_(can_recv_buffer) {
    //! 需要注意的是, 构造函数中的代码运行在Caller线程, 不运行在新的线程
    //! 所以线程要最后创建, 防止数据竞争, 和使用未初始化成员变量的问题

    ecat_manager_ = std::make_shared<ecat::EcatManager>(ifname, iomap_size,
                                                        servo_num, io_num);
    if (!ecat_manager_) {
        s_logger->critical("MotionThreadController ecat_manager create failed");
        throw exception("MotionThreadController ecat_manager create failed");
    }

    signal_buffer_ = std::make_shared<SignalBuffer>();

    //! 初始化公共数据的can buffer
    s_motion_shared->set_can_recv_buffer(can_recv_buffer);

    //! 创建motion状态机
    auto get_act_pos_cb =
        std::bind_front(&MotionThreadController::_get_act_pos, this);

    motion_state_machine_ = std::make_shared<MotionStateMachine>(
        get_act_pos_cb, signal_buffer_, cb_enable_votalge_gate_, cb_mach_on_);
    if (!motion_state_machine_) {
        s_logger->critical("MotionStateMachine create failed");
        throw exception("MotionStateMachine create failed");
    }

    latency_averager_ = std::make_shared<util::LongPeroidAverager<int32_t>>(
        [](int max) { s_logger->warn("latency_averager_ max: {}", max); });

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

#define NSEC_PER_SEC 1000000000

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
    //! 下一个周期开始的位置
    // // 这一周期理论的周期开始时间: ts
    // // 这一周期实际的开始时间: temp_curr_ts > ts (os保证)
    // if (thread_state_ == ThreadState::Running &&
    //     ecat_state_ == EcatState::EcatReady) {
    //     // 统计时防止上一周期因为在连接而导致时长超时
    //     struct timespec temp_curr_ts;
    //     clock_gettime(CLOCK_MONOTONIC, &temp_curr_ts);
    //     auto t = calcdiff_ns(temp_curr_ts, this->next_);
    //     if (t > 0)
    //         latency_averager_->push(t);

    //     if (t > cycletime_ns_) {
    //         s_logger->warn("t {} > cycletime_ns_; toff {}", t, toff_);
    //         // add_timespec(&this->next_, cycletime_ns_);
    //         this->next_ = temp_curr_ts;
    //     }
    // }

#ifndef EDM_OFFLINE_RUN_NO_ECAT
    // if (ecat_manager_->is_ecat_connected()) {
    //     TIMEUSESTAT(ecat_time_statistic_, ecat_manager_->ecat_sync(),
    //                 thread_state_ == ThreadState::Running &&
    //                     ecat_state_ == EcatState::EcatReady);
    // }
#endif // EDM_OFFLINE_RUN_NO_ECAT

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
    // TIMEUSESTAT(info_time_statistic_,
    //             _fetch_command_and_handle_and_copy_info_cache(),
    //             thread_state_ == ThreadState::Running &&
    //                 ecat_state_ == EcatState::EcatReady)
    _fetch_command_and_handle_and_copy_info_cache();

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
    // CPU_SET(3, &mask);
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

    // cpu_set_t get;
    // CPU_ZERO(&get);
    // pthread_getaffinity_np(pthread_self(), sizeof(get), &get);
    // auto isset = CPU_ISSET(2, &get);
    // s_logger->debug("isset: {}", isset);

    clock_gettime(CLOCK_MONOTONIC, &next_);
    int ht = (next_.tv_nsec / 1000000) + 1; /* round to nearest ms */
    next_.tv_nsec = ht * 1000000;

    ec_send_processdata();

    while (!thread_exit_) {

#ifndef EDM_OFFLINE_RUN_NO_ECAT
        if (!ecat_manager_->is_ecat_connected()) {
            toff_ = 0; // 未连接, toff是不对的值
        }
#endif // EDM_OFFLINE_RUN_NO_ECAT

        static struct timespec test_last_next;
        test_last_next = next_;

        /* calculate next cycle start */
        add_timespec(&next_, cycletime_ns_ + toff_);
        // add_timespec(&next_, cycletime_ns_);

        /* wait to cycle start */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_, &tleft_);

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

        // 统计时防止上一周期因为在连接而导致时长超时
        struct timespec temp_curr_ts;
        clock_gettime(CLOCK_MONOTONIC, &temp_curr_ts);
        auto t = calcdiff_ns(temp_curr_ts, this->next_);
        if (t > 0 && ecat_time_statistic_.averager().latest() < cycletime_ns_)
            latency_averager_->push(t);

        //! 防止固定间隔的时间戳跟不上 // TODO 后续细看DC时间同步
        if (t > cycletime_ns_) {
            s_logger->warn(
                "t {} > cycletime_ns_; toff {}, last ecat: {}, last total: {}, last info: {}, last sm: {}",
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

    ecat_manager_->ecat_sync(
        [this]() { ecat_manager_->disable_cycle_run_once(); });
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
        _switch_ecat_state(EcatState::EcatConnectedNotAllEnabled);
#else  // EDM_OFFLINE_RUN_NO_ECAT
        bool ret = ecat_manager_->connect_ecat(3);
        if (ret) {
            _switch_ecat_state(EcatState::EcatConnectedNotAllEnabled);
        }

        ec_send_processdata();
#endif // EDM_OFFLINE_RUN_NO_ECAT

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

        ecat_manager_->ecat_sync([this]() {
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

        TIMEUSESTAT(ecat_time_statistic_, ecat_manager_->ecat_sync([this]() {
            // set to motor axis
            const auto &cmd_axis = motion_state_machine_->get_cmd_axis();

            for (int i = 0; i < cmd_axis.size(); ++i) {
                ecat_manager_->set_servo_target_position(
                    i,
                    static_cast<int32_t>(std::lround(
                        cmd_axis[i])) // TODO, gear ratio, default 1.0
                );
            }

            // test

            // static int j = 0;
            // ++j;
            // auto pana_d =
            // std::static_pointer_cast<ecat::PanasonicServoDevice>(
            //     ecat_manager_->get_servo_device(0));
            // if (j < 100) {
            //     pana_d->ctrl_->v_offset = 10000;
            // } else if (j < 200) {
            //     pana_d->ctrl_->v_offset = -10000;
            // } else {
            //     j = 0;
            // }

            _dc_sync(); //! 只在正常的情况下进行DC同步
            // TODO 后续细看DC时间同步
        }),
                    true);

        // 先检查驱动器情况
        if (!ecat_manager_->is_ecat_connected()) {
            s_logger->warn("in EcatReady, ecat disconnected");
            _switch_ecat_state(EcatState::EcatDisconnected);
            // 重置 运动状态机
            motion_state_machine_->reset();
            break;
        }

        if (!ecat_manager_->servo_all_operation_enabled()) {
            s_logger->debug(
                "{:08b}",
                ecat_manager_->get_servo_device(0)->get_status_word());
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
    motion_state_machine_->refresh_axis_using_actpos();
    motion_state_machine_->set_enable(true);

    struct timespec temp_curr_ts;
    clock_gettime(CLOCK_MONOTONIC, &temp_curr_ts);
    add_timespec(&temp_curr_ts, cycletime_ns_);

    //! 主要用于防止连接Ecat的过程太耗时，导致固定间隔的时间戳跟不上
    // 在这里重置一下,
    this->next_ = temp_curr_ts;
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

    // struct stat s;
    // int err;

    // errno = 0;
    // err = stat("/dev/cpu_dma_latency", &s);
    // if (err == -1) {
    // 	s_logger->error("WARN: stat /dev/cpu_dma_latency failed");
    // 	return false;
    // }

    // errno = 0;
    // latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    // if (latency_target_fd == -1) {
    // 	s_logger->error("WARN: open /dev/cpu_dma_latency");
    // 	return false;
    // }

    // errno = 0;
    // err = write(latency_target_fd, &latency_target_value, 4);
    // if (err < 1) {
    // 	s_logger->error("# error setting cpu_dma_latency to {}!",
    // latency_target_value); 	close(latency_target_fd); 	return false;
    // }
    // printf("# /dev/cpu_dma_latency set to %dus\n", latency_target_value);
    // return true;
}

void MotionThreadController::_fetch_command_and_handle_and_copy_info_cache() {
    auto cmd_opt = motion_cmd_queue_->try_get_command();
    if (!cmd_opt) {
        // 没有命令, 只是拷贝info cache
        TIMEUSESTAT(info_time_statistic_, _copy_info_cache(), true);

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

        latency_averager_->clear();

        accept_cmd_flag = true;

        break;
    }
    case MotionCommandSetting_SetJumpParam: {
        s_logger->trace("Handle MotionCmd: Setting_SetJumpParam");

        auto set_jump_param_cmd =
            std::static_pointer_cast<MotionCommandSettingSetJumpParam>(cmd);

        motion_state_machine_->set_jump_param(set_jump_param_cmd->jump_param());

        s_logger->debug("***** JumpParam Received");

        accept_cmd_flag = true;
        break;
    }
    default:
        s_logger->warn("Unsupported MotionCommandType: {}", (int)cmd->type());
        cmd->ignore();
        break;
    }

    TIMEUSESTAT(info_time_statistic_, _copy_info_cache(),
                true); // 在处理完命令之后, accept/ignore之前, 缓存info,
                       // 保证外界在发送并等待完指令被accept/ignore后,
                       // 直接查询的info一定是正确的

    if (accept_cmd_flag) {
        cmd->accept();
    } else {
        cmd->ignore();
    }
}

void MotionThreadController::_dc_sync() {
    /* calulate toff to get linux time and DC synced */
    ecat_manager_->dc_sync_time(cycletime_ns_,
                                &toff_); //! 对指令速度突变的改善很有效果
}

bool MotionThreadController::_get_act_pos(axis_t &axis) {
#ifdef EDM_OFFLINE_RUN_NO_ECAT
    axis = motion_state_machine_->get_cmd_axis(); // 离线, 返回指令位置
#else                                             // EDM_OFFLINE_RUN_NO_ECAT
    if (!this->ecat_manager_->is_ecat_connected()) {
        // MotionUtils::ClearAxis(axis);
        return false;
    }

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] = (double)this->ecat_manager_->get_servo_actual_position(i);
    }
#endif                                            // EDM_OFFLINE_RUN_NO_ECAT

    return true;
}

void MotionThreadController::_copy_info_cache() {
#ifndef EDM_MOTION_INFO_GET_USE_ATOMIC
    std::lock_guard guard(info_cache_mutex_);
#endif // EDM_MOTION_INFO_GET_USE_ATOMIC

    info_cache_.curr_cmd_axis_blu = motion_state_machine_->get_cmd_axis();
    _get_act_pos(info_cache_.curr_act_axis_blu);

    info_cache_.main_mode = motion_state_machine_->main_mode();
    info_cache_.auto_state = motion_state_machine_->auto_state();

    // info_cache_.latency_data.curr_latency =
    // current_cycle_starttime_latency_ns_; info_cache_.latency_data.avg_latency
    // = avg_latency_ns_; info_cache_.latency_data.max_latency =
    // max_latency_ns_; info_cache_.latency_data.min_latency = min_latency_ns_;

    info_cache_.latency_data.curr_latency = latency_averager_->latest();
    info_cache_.latency_data.avg_latency = latency_averager_->average();
    info_cache_.latency_data.max_latency = latency_averager_->max();
    info_cache_.latency_data.min_latency = latency_averager_->min();

    info_cache_.time_use_data.total_time_use_avg =
        TIMEUSESTAT_AVG(total_time_statistic_);
    info_cache_.time_use_data.info_time_use_avg =
        TIMEUSESTAT_AVG(info_time_statistic_);
    info_cache_.time_use_data.ecat_time_use_avg =
        TIMEUSESTAT_AVG(ecat_time_statistic_);
    info_cache_.time_use_data.statemachine_time_use_avg =
        TIMEUSESTAT_AVG(statemachine_time_statistic_);

    // bit_state1
    info_cache_.bit_state1 = 0;

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

} // namespace move

} // namespace edm