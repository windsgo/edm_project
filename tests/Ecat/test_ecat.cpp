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

#include "ethercat.h"

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

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
    if (ts->tv_nsec > NSEC_PER_SEC) {
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

int target_pos = 0;

#include <chrono>

/* RT EtherCAT thread */
OSAL_THREAD_FUNC_RT ecatthread(void *ptr) {
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

    while (1) {
        ++mmm;

        /* calculate next cycle start */
          add_timespec(&ts, cycletime + toff);
        // add_timespec(&ts, cycletime);

        /* wait to cycle start */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

        clock_gettime(CLOCK_MONOTONIC, &curr_ts);

        int diff = curr_ts.tv_nsec - (last_curr_ts.tv_nsec + cycletime + toff);
        if (diff > max_diff_ns && mmm > 1000) max_diff_ns = diff;
        if (diff > 5000) {
            s_logger->warn("diff: {}", diff);
        }

        EDM_CYCLIC_LOG(s_logger->debug, 500, "calc: sec: {}, nsec: {}", ts.tv_sec, ts.tv_nsec);
        EDM_CYCLIC_LOG(s_logger->debug, 500, "real: sec: {}, nsec: {}; diff: {}ns, max_diff: {}ns", curr_ts.tv_sec, curr_ts.tv_nsec, diff, max_diff_ns);
        memcpy(&last_curr_ts, &curr_ts, sizeof(struct timespec));

        if (dorun > 0) {
            em->ecat_sync();

            // ec_receive_processdata(200);
            auto d = em->get_servo_device(0);

            em->set_servo_target_position(0, target_pos);

            target_pos += 3;

            // EDM_CYCLIC_LOG(
            //     s_logger->debug, 500,
            //     "sw: {:016b}, cmd: {} ,pos: {}; {}, {}, {}, {}, {};; {}, {}; toff: {}",
            //     d->get_status_word(), target_pos, d->get_actual_position(),
            //     d->sw_fault(), d->sw_ready_to_switch_on(),
            //     d->sw_switch_on_disabled(), d->sw_switched_on(),
            //     d->sw_operational_enabled(), em->servo_has_fault(),
            //     em->servo_all_operation_enabled(), toff);

            if (em->servo_has_fault()) {
                // s_logger->debug("has fault");
                em->clear_fault_cycle_run_once();
            } else if (!em->servo_all_operation_enabled()) {
                // s_logger->debug("not all op enabled");
                em->clear_fault_cycle_run_once();
            }


            //  int wkc = ec_receive_processdata(EC_TIMEOUTRET);

            //  dorun++;
            //  /* if we have some digital output, cycle */
            //  if( digout ) *digout = (uint8) ((dorun / 16) & 0xff);

             if (ec_slave[0].hasdc)
             {
                /* calulate toff to get linux time and DC synced */
                ec_sync(ec_DCtime, cycletime, &toff); //! 对指令速度突变的改善很有效果
             }
            //  ec_send_processdata();

        }
    }
}

static void test_ecat(void) {
    int ctime = 1000;

    /* create RT thread */
    osal_thread_create_rt(&thread1, stack64k * 2, (void *)&ecatthread,
                          (void *)&ctime);

    // TODO
    // TODO
    // TODO
    //! ecat_sync dc同步未使用, 时间抖动未测量, motion thread中用async logger
    //! cpu dmalatency 见一个收藏的csdn博客
    //! 自己的motion thread, 再提高優先級
    //! 网卡驱动优化(之后再说)

    em = std::make_shared<edm::ecat::EcatManager>("enx34298f700ae1", 4096, 1);
    bool ret = em->connect_ecat(3);
    if (ret) {
        target_pos = em->get_servo_actual_position(0);
        s_logger->debug("dorun = 1, init target_pos = {}", target_pos);
        dorun = 1;
        // em->ecat_sync();
    }

    // pthread_join(thread1);
    while (1)
        ;
}

int main(int argc, char **argv) {
    test_ecat();
    return 0;
}