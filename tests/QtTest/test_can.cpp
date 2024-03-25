#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "Utils/Netif/netif_utils.h"

#include "QtDependComponents/CanController/CanController.h"

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

static edm::can::CanController::ptr can_ctrler;

using namespace std::chrono_literals;

static bool stop_flag = false;
static void thread_func() {
    s_root_logger->info("thread_func start");
    QByteArray array{8, 0x16};
    array.resize(8);
    QCanBusFrame frame0(0x147, array);

    while (!stop_flag) {
        std::this_thread::sleep_for(1000us);

        can_ctrler->send_frame("can0", frame0);

        static int x = 0, j = 0;
        ++x;
        if (x >= 1000) {
            x = 0;
            ++j;
            s_root_logger->debug("sent: {}", j);
        }
    }
}

#include <stdio.h>

std::atomic_uint8_t dummy[8];

struct dummy_canfetcher {
    uint8_t data[8];
};
std::atomic<dummy_canfetcher> ddd;

static_assert(sizeof(dummy_canfetcher) == sizeof(ddd));

int main(int argc, char **argv) {

    s_root_logger->debug("test can");

    QCoreApplication app(argc, argv);

    qDebug().noquote() << "main thread: " << QThread::currentThreadId();

    can_ctrler = std::make_shared<edm::can::CanController>();

    auto can0 = can_ctrler->add_device("can0", 500000);
    can_ctrler->add_device("can1", 500000);

    while (!can_ctrler->is_connected("can0"))
        ;
    while (!can_ctrler->is_connected("can1"))
        ;

    can_ctrler->add_frame_received_listener(
        "can1", [](const QCanBusFrame &frame) {
            // qDebug().noquote() << "thread: " << QThread::currentThreadId() <<
            // frame.toString();
            static int i = 0;
            static int j = 0;
            ++i;
            // fprintf(stderr, "%d,", i);

            // for (int b = 0; b < 8; ++b) {
            //     dummy[b] = frame.payload()[b];
            // }

            ddd = *(dummy_canfetcher *)frame.payload().data();

            frame.frameType();

            if (i >= 500 * 2) {
                i = 0;
                ++j;
                s_root_logger->debug("recv: {}", j);
            }
        });

    // ! 测试非Qt线程调用QController
    // std::thread thread(thread_func);

    QByteArray array{8, 0x16};
    array.resize(8);
    QCanBusFrame frame0(0x147, array);
    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        can_ctrler->send_frame(can0, frame0);
        // can_ctrler->send_frame("can0", frame0);
        static int i = 0;
        static int j = 0;
        ++i;
        // fprintf(stderr, "%d,", i);

        // for (int b = 0; b < 8; ++b) {
        //     dummy[b] = frame.payload()[b];
        // }

        if (i >= 500 * 2) {
            i = 0;
            ++j;
            s_root_logger->debug("sent: {}", j);
        }
    });

    t.start(1);

    QTimer t2;
    QObject::connect(&t2, &QTimer::timeout, [&]() {
        QByteArray array{8, 0x23};
        array.resize(8);
        QCanBusFrame frame0(0x148, array);
        can_ctrler->send_frame("can1", frame0);
        // can_ctrler->send_frame("can1", frame0);
    });

    // t2.start(200);

    QTimer t3;
    QObject::connect(&t3, &QTimer::timeout, [&]() {
        dummy_canfetcher c = ddd;
    });

    t3.start(1);

    int ret = app.exec();

    return ret;
}