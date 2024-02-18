#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "Utils/Netif/netif_utils.h"

#include "QtDependComponents/CanController/CanController.h"

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

using namespace std::chrono_literals;

static bool stop_flag = false;
static void thread_func() {
    s_root_logger->info("thread_func start");

    while (!stop_flag) {
        std::this_thread::sleep_for(2000us);

        static int aa = 0;
        if (!edm::can::CanController::instance()->is_connected("can0")) {
            ++aa;
            continue;;
        }

        QByteArray array{8, 0x16};
        array.resize(8);
        QCanBusFrame frame0(0x147, array);
        edm::can::CanController::instance()->send_frame("can0", frame0);
        edm::can::CanController::instance()->send_frame("can0", frame0);

        static int x = 0, j = 0;
        ++x;
        if (x >= 500) {
            x = 0;
            ++j;
            qDebug() << "sent" << j << edm::can::CanController::instance()->is_connected("can0") << aa;
        }
    }
}

#include <stdio.h>

int main(int argc, char **argv) {

    s_root_logger->debug("test can");

    QCoreApplication app(argc, argv);

    qDebug().noquote() << "main thread: " << QThread::currentThreadId();

    edm::can::CanController::instance()->add_device("can0", 115200);
    edm::can::CanController::instance()->add_device("can1", 115200);

    edm::can::CanController::instance()->add_frame_received_listener(
        "can1", [](const QCanBusFrame &frame) {
            // qDebug().noquote() << "thread: " << QThread::currentThreadId() << frame.toString();
            static int i = 0;
            static int j = 0;
            ++i;
            // fprintf(stderr, "%d,", i);
            
            if (i >= 500) {
                i = 0;
                ++j;
                s_root_logger->debug("recv: {}", j);
            }
        });


    // ! 测试非Qt线程调用QController
    // std::thread thread(thread_func);

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        // static int i = 0;
        // ++i;
        // if (i == 10000) {
        //     // edm::can::CanController::instance()->terminate();
        //     t.stop();
        //     // stop_flag = true;
        //     // thread.join();

        //     // QCoreApplication::exit(0);
        //     return;
        // }

        // static int x = 0, j = 0;
        // ++x;
        // if (x >= 500) {
        //     x = 0;
        //     ++j;
        //     qDebug() << "sent" << j;
        // }

        QByteArray array{8, 0x16};
        array.resize(8);
        QCanBusFrame frame0(0x147, array);
        edm::can::CanController::instance()->send_frame("can0", frame0);
        edm::can::CanController::instance()->send_frame("can0", frame0);

        // array.fill(0xEF);
        // QCanBusFrame frame1(0x258, array);
        // edm::can::CanController::instance()->send_frame("can1", frame1);

    });

    t.start(2);
    int ret = app.exec();

    return ret;
}