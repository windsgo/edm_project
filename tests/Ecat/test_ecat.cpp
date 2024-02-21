#include "EcatManager/EcatManager.h"
#include "Logger/LogMacro.h"

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

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

/* 调试打印宏 */
#define CYCLIC_DO(__func, __peroid, __fmt, ...) \
    {                                           \
        static int __cyclic_i = 0;              \
        ++__cyclic_i;                           \
        if (__cyclic_i >= (__peroid)) {         \
            __func(__fmt, ##__VA_ARGS__);       \
            __cyclic_i = 0;                     \
        }                                       \
    }

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
int target_pos = 0;
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



    while (1) {
        /* calculate next cycle start */
        //   add_timespec(&ts, cycletime + toff);
        add_timespec(&ts, cycletime);
        /* wait to cycle start */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

        
        if (dorun > 0) {
            auto d = em->get_servo_device(0);

            em->set_servo_target_position(0, target_pos);

            ++target_pos;

            CYCLIC_DO(s_logger->debug, 500,
                      "sw: {:016b}, cmd: {} ,pos: {}; {}, {}, {}, {}, {};; {}, {}",
                      d->get_status_word(), target_pos, d->get_actual_position(), d->sw_fault(),
                      d->sw_ready_to_switch_on(), d->sw_switch_on_disabled(),
                      d->sw_switched_on(), d->sw_operational_enabled(),
                      em->servo_has_fault(), em->servo_all_operation_enabled());

            if (em->servo_has_fault()) {
                // s_logger->debug("has fault");
                em->clear_fault_cycle_run_once();
            } else if (!em->servo_all_operation_enabled()) {
                // s_logger->debug("not all op enabled");
                em->clear_fault_cycle_run_once();
            }

            em->ecat_sync();

            //  int wkc = ec_receive_processdata(EC_TIMEOUTRET);

            //  dorun++;
            //  /* if we have some digital output, cycle */
            //  if( digout ) *digout = (uint8) ((dorun / 16) & 0xff);

            //  if (ec_slave[0].hasdc)
            //  {
            //     /* calulate toff to get linux time and DC synced */
            //     ec_sync(ec_DCtime, cycletime, &toff);
            //  }
            //  ec_send_processdata();
        }
    }
}

static void test_ecat(void) {
    int ctime = 2000;

    /* create RT thread */
    osal_thread_create_rt(&thread1, stack64k * 2, (void *)&ecatthread,
                          (void *)&ctime);

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