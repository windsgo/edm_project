#include "EcatManager/EcatManager.h"
#include "Logger/LogMacro.h"
#include "Utils/Format/edm_format.h"

#include <functional>

#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "Utils/DataQueueRecorder/DataQueueRecorder.h"

#include "ethercat.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

static std::atomic_bool s_stop_flag{false};

pthread_t thread1;
#define stack64k (64 * 1024)
int dorun = 0;
// int64 toff;

#define NSEC_PER_SEC  1000000000
#define EC_TIMEOUTMON 500

edm::ecat::EcatManager::ptr em;

/* add ns to timespec */
void add_timespec(struct timespec *ts, int64 addtime) {
    int64 sec, nsec;

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
void ec_sync(int64 reftime, int64 cycletime, int64 *offsettime) {
    static int64 integral = 0;
    int64 delta;
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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>

static int latency_target_fd = -1;
static int32_t latency_target_value = 0;

/* 消除系统时钟偏移函数，取自cyclic_test */
static void set_latency_target(void) {
    struct stat s;
    int ret;

    if (stat("/dev/cpu_dma_latency", &s) == 0) {
        latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
        if (latency_target_fd == -1)
            return;
        ret = write(latency_target_fd, &latency_target_value, 4);
        if (ret == 0) {
            printf("# error setting cpu_dma_latency to %d!: %s\n",
                   latency_target_value, strerror(errno));
            close(latency_target_fd);
            return;
        }
        printf("# /dev/cpu_dma_latency set to %dus\n", latency_target_value);
    }
}

volatile int target_pos = 0;

#include <chrono>

static inline int64_t calcdiff_ns(const timespec &t1, const timespec &t2) {
    int64_t diff = NSEC_PER_SEC * (int64_t)((int)t1.tv_sec - (int)t2.tv_sec);
    diff += ((int)t1.tv_nsec - (int)t2.tv_nsec);
    return diff;
}

struct Data {
    int act_pos;
    int cmd_pos;
};

auto data_recorder = std::make_shared<edm::util::DataQueueRecorder<Data>>();

/* RT EtherCAT thread */
void *ecatthread(void *ptr) {
    struct timespec ts, tleft;
    int ht;
    int64 cycletime;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
    ts.tv_nsec = ht * 1000000;
    cycletime = *(int *)ptr * 1000; /* cycletime in ns */
                                    //    toff = 0;
    dorun = 0;
    ec_send_processdata();

    int64_t toff = 0;
    struct timespec last_curr_ts, curr_ts;
    int max_diff_ns = 0;
    uint32_t mmm = 0;

    uint32_t record_count = 0;

    int policy = -1;
    sched_param param;
    pthread_getschedparam(pthread_self(), &policy, &param);
    s_logger->info("*** thread policy: {}, pri: {}", (policy),
                   (int)param.sched_priority);

    while (1) {
        ++mmm;

        int64_t next_cycle_time = cycletime + toff;

        /* calculate next cycle start */
        add_timespec(&ts, next_cycle_time);
        // add_timespec(&ts, cycletime);

        /* wait to cycle start */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

        /* wake up here */
        /* this is the begin of this peroid */

        if (s_stop_flag && dorun == 0) {
            break;
        }

        if (dorun > 0) {

            if (!em->is_ecat_connected()) {
                s_logger->error("err: disconnected");
                dorun = 0;

                continue;
            }

            clock_gettime(CLOCK_MONOTONIC, &curr_ts);

            int diff =
                curr_ts.tv_nsec - (last_curr_ts.tv_nsec + next_cycle_time);
            if (diff > max_diff_ns && mmm > 1000)
                max_diff_ns = diff;
            if (diff > 5000) {
                s_logger->warn("diff: {}", diff);
            }

            struct timespec sync_begin_ts;
            clock_gettime(CLOCK_MONOTONIC, &sync_begin_ts);

            em->ecat_sync(); // 127us 左右

            struct timespec sync_end_ts;
            clock_gettime(CLOCK_MONOTONIC, &sync_end_ts);

            auto d = em->get_servo_device(0);
            auto sw = d->get_status_word();
            EDM_CYCLIC_LOG(s_logger->debug, 500, "sw: {0:016b}, {0:08X}", sw);

            if (s_stop_flag) {
                if (em->servo_all_disabled()) {
                    // s_logger->debug("servo_all_disabled: sw: {:016b}", sw);

                    em->disconnect_ecat();
                    break; // 退出主循环
                }

                em->disable_cycle_run_once();
            } else {

                if (em->servo_has_fault()) {
                    // s_logger->debug("has fault");
                    em->clear_fault_cycle_run_once();
                } else if (!em->servo_all_operation_enabled()) {
                    // s_logger->debug("not all op enabled");
                    em->clear_fault_cycle_run_once();
                } else {
                    em->set_servo_target_position(0, target_pos);
                    target_pos += 3;

                    if (data_recorder->is_running()) {
                        data_recorder->emplace(d->get_actual_position(),
                                               target_pos);
                    }
                }
            }

            if (ec_slave[0].hasdc) {
                /* calulate toff to get linux time and DC synced */
                ec_sync(ec_DCtime, cycletime,
                        &toff); //! 对指令速度突变的改善很有效果
            }

            struct timespec curr_end_ts;
            clock_gettime(CLOCK_MONOTONIC, &curr_end_ts); // 当前周期结束的时间

            EDM_CYCLIC_LOG(s_logger->debug, 500, "calc: sec: {}, nsec: {}",
                           ts.tv_sec, ts.tv_nsec);
            EDM_CYCLIC_LOG(
                s_logger->debug, 500,
                "real: sec: {}, nsec: {}; diff: {}ns, max_diff: {}ns",
                curr_ts.tv_sec, curr_ts.tv_nsec, diff, max_diff_ns);
            memcpy(&last_curr_ts, &curr_ts, sizeof(struct timespec));

            EDM_CYCLIC_LOG(s_logger->debug, 500, "耗时: {}s, {}ns, sync: {}ns",
                           curr_end_ts.tv_sec - curr_ts.tv_sec,
                           curr_end_ts.tv_nsec - curr_ts.tv_nsec,
                           sync_end_ts.tv_nsec - sync_begin_ts.tv_nsec);
        }
    }

    return NULL;
}

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
static int create_thread(void) {
    int ctime = 1000;

    set_latency_target(); // 有效消除本地周期不稳定warning

    /* create RT thread */
    // osal_thread_create_rt(&thread1, stack64k * 2, (void *)&ecatthread,
    //                       (void *)&ctime);

    /* use pthread, create self */
    struct sched_param param;
    pthread_attr_t attr;
    int ret;

    /* Lock memory */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }

    /* Initialize pthread attributes (default values) */
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&attr, stack64k * 2);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }

    /* Set scheduler policy and priority of pthread */
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    param.sched_priority = 99;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        goto out;
    }
    /* Use scheduling parameters of attr */
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        goto out;
    }

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    ret = pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &mask);
    if (ret) {
        printf("pthread_attr_setaffinity_np failed\n");
        goto out;
    }

    /* Create a pthread with specified attributes */
    ret = pthread_create(&thread1, &attr, ecatthread, (void *)&ctime);
    if (ret) {
        printf("create pthread failed\n");
        goto out;
    }

out:
    return ret;
}

static std::thread record_waiting_thread;
static std::thread ecat_reconnect_thread;
static void test_ecat(void) {

    if (create_thread()) {
        return;
    }

    // TODO
    // TODO
    // TODO
    //! ecat_sync dc同步未使用, 时间抖动未测量, motion thread中用async logger
    //! cpu dmalatency 见一个收藏的csdn博客
    //! 自己的motion thread, 再提高優先級
    //! 网卡驱动优化(之后再说)

    em = std::make_shared<edm::ecat::EcatManager>("enx34298f700ae1", 4096, 1);

    using namespace std::chrono_literals;
    ecat_reconnect_thread = std::thread([=]() {
        while (!s_stop_flag) {

            std::this_thread::sleep_for(1s);
            if (dorun > 0) {
                continue;
            }

            bool cret = em->connect_ecat(3);
            if (cret) {
                target_pos = em->get_servo_actual_position(0);
                s_logger->debug("dorun = 1, init target_pos = {}", target_pos);
                dorun = 1;
                // em->ecat_sync();
            }
        }
    });

    auto start_time =
        std::chrono::high_resolution_clock::now().time_since_epoch();
    data_recorder->start_record("output.bin");
    std::this_thread::sleep_for(100ms);

    record_waiting_thread = std::thread([=]() {
        while (1) {
            auto now =
                std::chrono::high_resolution_clock::now().time_since_epoch();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - start_time)
                    .count() >= 500) {
                data_recorder->stop_record();
                s_logger->info("record over, stop recorder");
                break;
            } else {
                std::this_thread::sleep_for(100ms);
                // s_logger->info("sleep_for");
            }

            if (s_stop_flag) {
                // data_recorder->stop_record();
                break;
            }
        }
    });

    /* Join the thread and wait until it is done */

    // // pthread_join(thread1);
    // while (1)
    //     ;
}

#include <signal.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QObject>

static void fun_sig(int sig) {
    s_stop_flag = true;
    s_logger->info("ctrl+c");

    signal(sig, SIG_DFL); // 防止多次ctrl+c退出不了

    QCoreApplication::quit();
}

#include "QtDependComponents/CanController/CanController.h"
// static auto s_can_ctrler = edm::can::CanController::instance();

// class CanControllerHolder {
// public:
//     CanControllerHolder() { edm::can::CanController::instance()->init(); }
//     ~CanControllerHolder() { edm::can::CanController::instance()->terminate(); }
// };

int main(int argc, char **argv) {

    QCoreApplication a(argc, argv);

    // CanControllerHolder holder;
    // s_can_ctrler->add_device("can0", 500000);

    signal(SIGINT, fun_sig);

    pid_t pid = getpid();
    s_logger->info("pid: {}", pid);

    test_ecat();

    int ret = a.exec();

    int join_ret = pthread_join(thread1, NULL);
    if (join_ret)
        printf("join pthread failed: %m\n");

    record_waiting_thread.join();
    ecat_reconnect_thread.join();

    s_logger->info("pid: {} exit", pid);
    return ret;
}