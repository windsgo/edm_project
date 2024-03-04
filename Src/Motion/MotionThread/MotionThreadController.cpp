#include "MotionThreadController.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {
MotionThreadController::MotionThreadController(
    std::string_view ifname, MotionCommandQueue::ptr motion_cmd_queue,
    MotionSignalQueue::ptr motion_signal_queue, uint32_t iomap_size,
    uint32_t servo_num, uint32_t io_num)
    : motion_cmd_queue_(motion_cmd_queue),
      motion_signal_queue_(motion_signal_queue) {
    //! 需要注意的是, 构造函数中的代码允许在Caller线程, 不运行在新的线程
    //! 所以线程要最后创建, 防止数据竞争, 和使用未初始化成员变量的问题

    ecat_manager_ = std::make_shared<ecat::EcatManager>(ifname, iomap_size,
                                                        servo_num, io_num);
    if (!ecat_manager_) {
        s_logger->critical("MotionThreadController ecat_manager create failed");
        throw exception("MotionThreadController ecat_manager create failed");
    }

    signal_buffer_ = std::make_shared<SignalBuffer>();

    auto get_act_pos_cb =
        std::bind_front(&MotionThreadController::_get_act_pos, this);
    auto touch_detect_cb = [this]() -> bool {
        return false; // TODO
    };
    motion_state_machine_ = std::make_shared<MotionStateMachine>(
        get_act_pos_cb, touch_detect_cb, signal_buffer_);
    if (!motion_state_machine_) {
        s_logger->critical("MotionStateMachine create failed");
        throw exception("MotionStateMachine create failed");
    }

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
}

void *MotionThreadController::_ThreadEntry(void *mtc) {
    auto controller = static_cast<MotionThreadController *>(mtc);

    s_logger->trace("MotionThreadController _ThreadEntry enter.");

    void *ret = controller->_run();

    s_logger->trace("MotionThreadController _ThreadEntry exit.");

    return ret;
}

bool MotionThreadController::_create_thread() {

    // TODO set_latency_target
    if (!_set_cpu_dma_latency()) {
        s_logger->warn("MotionThreadController _set_cpu_dma_latency failed.");
    }

    struct sched_param param;
    pthread_attr_t attr;
    int ret;

    /* Lock memory */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        s_logger->critical("MotionThreadController mlockall failed: {}",
                           strerror(errno));
        // exit(-2);
        return false;
    }

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
    CPU_SET(1, &mask);
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
    if (ret) {
        s_logger->critical("pthread_attr_setaffinity_np failed: {}", ret);
        return false;
    }

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

#define NSEC_PER_SEC 1000000000

/* add ns to timespec */
static void add_timespec(struct timespec *ts, int64_t addtime) {
    int64_t sec, nsec;

    nsec = addtime % NSEC_PER_SEC;
    sec = (addtime - nsec) / NSEC_PER_SEC;
    ts->tv_sec += sec;
    ts->tv_nsec += nsec;
    if (ts->tv_nsec > NSEC_PER_SEC) {
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

void *MotionThreadController::_run() {

    struct timespec ts, tleft;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    int ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
    ts.tv_nsec = ht * 1000000;

    bool thread_exit = false;
    while (!thread_exit) {
        if (!ecat_manager_->is_ecat_connected()) {
            toff_ = 0; // 未连接, toff是不对的值
        }

        /* calculate next cycle start */
        add_timespec(&ts, cycletime_ns_ + toff_);

        /* wait to cycle start */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

        //! 下一个周期开始的位置

        if (ecat_manager_->is_ecat_connected()) {
            ecat_manager_->ecat_sync();
        }

        _fetch_command_and_handle();

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
            thread_exit = true; // 退出循环
            break;
        }
        }

        //! 在最外层 返回info, 以及处理要返回的信号

        _copy_info_cache();

        // handle signal
        _handle_signal();
    }

    return NULL;
}

void MotionThreadController::_threadstate_stopping() {
    ++stopping_count_;
    if (stopping_count_ > stopping_count_max_) {
        s_logger->warn("stopping count overflow: {} > {}", stopping_count_,
                       stopping_count_max_);
        ecat_manager_->disconnect_ecat();
        _switch_thread_state(ThreadState::CanExit);
        return;
    }

    if (!ecat_manager_->is_ecat_connected()) {
        ecat_manager_->disconnect_ecat();
        _switch_thread_state(ThreadState::CanExit);
        return;
    }

    if (ecat_manager_->servo_all_disabled()) {
        ecat_manager_->disconnect_ecat(); //! 一定要调用, 不然驱动器报警
        _switch_thread_state(ThreadState::CanExit);
        return;
    }

    ecat_manager_->disable_cycle_run_once();
}

void MotionThreadController::_threadstate_running() {
    switch (ecat_state_) {
    default:
    case EcatState::Init:
        _switch_ecat_state(EcatState::EcatDisconnected);
        break;
    case EcatState::EcatDisconnected:
        if (ecat_connect_flag_) {
            _switch_ecat_state(EcatState::EcatConnecting);
            ecat_connect_flag_ = false;
        }
        break;
    case EcatState::EcatConnecting: {
        ecat_connect_flag_ = false;

        bool ret = ecat_manager_->connect_ecat(3);
        if (ret) {
            _switch_ecat_state(EcatState::EcatConnectedNotAllEnabled);
        }

        break;
    }
    case EcatState::EcatConnectedNotAllEnabled: {
        if (ecat_manager_->servo_all_operation_enabled()) {
            s_logger->debug(
                "{:08b}",
                ecat_manager_->get_servo_device(0)->get_status_word());
            _ecat_state_switch_to_ready();
            ecat_clear_fault_reenable_flag_ = false;
            break;
        }

        if (ecat_clear_fault_reenable_flag_) {
            _switch_ecat_state(EcatState::EcatConnectedEnabling);
            ecat_clear_fault_reenable_flag_ = false;
            break;
        }

        break;
    }
    case EcatState::EcatConnectedEnabling: {
        ecat_clear_fault_reenable_flag_ = false;
        if (!ecat_manager_->servo_all_operation_enabled()) {
            ecat_manager_->clear_fault_cycle_run_once();
        } else {
            _ecat_state_switch_to_ready();
        }
        break;
    }
    case EcatState::EcatReady: {

        signal_buffer_->reset_all();

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

        // StateMachine Operate Cycle
        motion_state_machine_->run_once();

        // set to motor axis
        const auto &cmd_axis = motion_state_machine_->get_cmd_axis();
        for (int i = 0; i < cmd_axis.size(); ++i) {
            ecat_manager_->set_servo_target_position(
                i,
                static_cast<int32_t>(
                    std::lround(cmd_axis[i])) // TODO, gear ratio, default 1.0
            );
        }

        // handle info buffer

        _dc_sync(); //! 只在正常的情况下进行DC同步
        break;
    }
    }
}

void MotionThreadController::_ecat_state_switch_to_ready() {
    _switch_ecat_state(EcatState::EcatReady);
    motion_state_machine_->reset();
    motion_state_machine_->refresh_axis_using_actpos();
    motion_state_machine_->set_enable(true);
}

bool MotionThreadController::_set_cpu_dma_latency() {
    /* 消除系统时钟偏移函数，取自cyclic_test */
    struct stat s;

    if (stat("/dev/cpu_dma_latency", &s) == 0) {
        int latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
        if (latency_target_fd == -1) {
            s_logger->error("open /dev/cpu_dma_latency failed");
            return false;
        }

        int ret = write(latency_target_fd, &latency_target_value_, 4);
        if (ret <= 0) {
            s_logger->error("# error setting cpu_dma_latency to {}!: {}\n",
                            latency_target_value_, strerror(errno));
            close(latency_target_fd);
            return false;
        }

        close(latency_target_fd);
        s_logger->info("# /dev/cpu_dma_latency set to {}us\n",
                       latency_target_value_);
        return true;
    } else {
        return false;
    }
}

void MotionThreadController::_fetch_command_and_handle() {
    // TODO
    auto cmd_opt = motion_cmd_queue_->try_get_command();
    if (!cmd_opt) {
        return;
    }

    auto cmd = *cmd_opt;

    // TODO handle cmd
    switch (cmd->type()) {
    case MotionCommandManual_StartPointMove: {
        s_logger->trace("Handle MotionCmd: Manual_StartPointMove");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            cmd->ignore();
            break;
        }

        auto spm_cmd =
            std::static_pointer_cast<MotionCommandManualStartPointMove>(cmd);

        auto ret = motion_state_machine_->start_manual_pointmove(
            spm_cmd->speed_param(), spm_cmd->end_pos());

        if (ret) {
            cmd->accept();
        } else {
            cmd->ignore();
        }
        break;
    }
    case MotionCommandManual_StopPointMove: {
        s_logger->trace("Handle MotionCmd: Manual_StopPointMove");
        if (ecat_state_ != EcatState::EcatReady ||
            thread_state_ != ThreadState::Running) {
            cmd->ignore();
            break;
        }

        auto spm_cmd =
            std::static_pointer_cast<MotionCommandManualStopPointMove>(cmd);

        auto ret =
            motion_state_machine_->stop_manual_pointmove(spm_cmd->immediate());

        if (ret) {
            cmd->accept();
        } else {
            cmd->ignore();
        }
        break;
    }
    case MotionCommandSetting_TriggerEcatConnect: {
        s_logger->trace("Handle MotionCmd: Setting_TriggerEcatConnect");
        cmd->accept();
        ecat_connect_flag_ = true;
        ecat_clear_fault_reenable_flag_ = true;
        break;
    }
    default:
        s_logger->warn("Unsupported MotionCommandType: {}", (int)cmd->type());
        cmd->ignore();
        break;
    }
}

void MotionThreadController::_dc_sync() {
    /* calulate toff to get linux time and DC synced */
    ecat_manager_->dc_sync_time(cycletime_ns_,
                                &toff_); //! 对指令速度突变的改善很有效果
}

bool MotionThreadController::_get_act_pos(axis_t &axis) {
    if (!this->ecat_manager_->is_ecat_connected()) {
        // MotionUtils::ClearAxis(axis);
        return false;
    }

    for (int i = 0; i < axis.size(); ++i) {
        axis[i] = (double)this->ecat_manager_->get_servo_actual_position(i);
    }

    return true;
}

void MotionThreadController::_copy_info_cache() {
    std::lock_guard guard(info_cache_mutex_);

    info_cache_.curr_cmd_axis_blu = motion_state_machine_->get_cmd_axis();
    _get_act_pos(info_cache_.curr_act_axis_blu);

    info_cache_.main_mode = motion_state_machine_->main_mode();
    info_cache_.auto_state = motion_state_machine_->auto_state();

    // bit_state1
    info_cache_.bit_state1 = 0;

    info_cache_.setEcatConnected(ecat_manager_->is_ecat_connected());
    info_cache_.setEcatAllEnabled(ecat_state_ == EcatState::EcatReady);
    info_cache_.setTouchDetectEnabled(false); // TODO
    info_cache_.setTouchDetected(false);      // TODO
    info_cache_.setTouchWarning(false);       // TODO
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