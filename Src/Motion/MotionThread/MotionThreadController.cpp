#include "MotionThreadController.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {
MotionThreadController::MotionThreadController(
    std::string_view ifname, MotionCommandQueue::ptr motion_cmd_queue,
    uint32_t servo_num, uint32_t io_num = 0)
    : motion_cmd_queue_(motion_cmd_queue) {
    //! 需要注意的是, 构造函数中的代码允许在Caller线程, 不运行在新的线程
    //! 所以线程要最后创建, 防止数据竞争, 和使用未初始化成员变量的问题

    ecat_manager_ =
        std::make_shared<ecat::EcatManager>(ifname, servo_num, io_num);
    if (!ecat_manager_) {
        s_logger->critical("MotionThreadController ecat_manager create failed");
        throw exception("MotionThreadController ecat_manager create failed");
    }

    //! 线程最后创建, 在成员变量都初始化完毕后再创建
    if (!_create_thread()) {
        s_logger->critical("Motion Thread Create Failed");
        throw exception("Motion Thread Create Failed");
    }
}

MotionThreadController::~MotionThreadController() {
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

        switch (thread_state_) {
        default:
        case ThreadState::Init:
            thread_state_ = ThreadState::Running;
            break;
        case ThreadState::Running: {
            _threadstate_running();

            if (thread_stop_flag_) {
                thread_state_ = ThreadState::Stopping; //! 准备退出线程
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
    }

    return NULL;
}

void MotionThreadController::_threadstate_stopping() {
    ++stopping_count_;
    if (stopping_count_ > stopping_count_max_) {
        thread_state_ = ThreadState::CanExit;
        return;
    }

    if (!ecat_manager_->is_ecat_connected()) {
        thread_state_ = ThreadState::CanExit;
        return;
    }

    if (ecat_manager_->servo_all_disabled()) {
        thread_state_ = ThreadState::CanExit;
        return;
    }

    ecat_manager_->disable_cycle_run_once();
}

void MotionThreadController::_threadstate_running() {
    switch (ecat_state_) {
    default:
    case EcatState::Init:
        ecat_state_ = EcatState::EcatDisconnected;
        break;
    case EcatState::EcatDisconnected:
        if (ecat_connect_flag_) {
            ecat_state_ = EcatState::EcatConnecting;
            ecat_connect_flag_ = false;
        }
        break;
    case EcatState::EcatConnecting: {
        ecat_connect_flag_ = false;

        bool ret = ecat_manager_->connect_ecat(3);
        if (ret) {
            ecat_state_ = EcatState::EcatConnectedNotAllEnabled;
        }

        break;
    }
    case EcatState::EcatConnectedNotAllEnabled: {
        if (ecat_manager_->servo_all_operation_enabled()) {
            ecat_state_ = EcatState::EcatReady;
            break;
        }

        if (ecat_clear_fault_reenable_flag_) {
            ecat_state_ = EcatState::EcatConnectedEnabling;
            ecat_clear_fault_reenable_flag_ = false;
            break;
        }
    }
    case EcatState::EcatConnectedEnabling: {
        ecat_clear_fault_reenable_flag_ = false;
        if (!ecat_manager_->servo_all_operation_enabled()) {
            ecat_manager_->clear_fault_cycle_run_once();
        }
        break;
    }
    case EcatState::EcatReady:

        // TODO StateMachine Operate Cycle

        _dc_sync(); //! 只在正常的情况下进行DC同步
        break;
    }
}

bool MotionThreadController::_set_cpu_dma_latency() { return false; }

void MotionThreadController::_fetch_command_and_handle() {}

void MotionThreadController::_dc_sync() {
    /* calulate toff to get linux time and DC synced */
    ecat_manager_->dc_sync_time(cycletime_ns_,
                                &toff_); //! 对指令速度突变的改善很有效果
}

} // namespace move

} // namespace edm