#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

int main(int argc, char **argv) {

    s_root_logger->debug("test can");

    QCoreApplication app(argc, argv);

    qDebug().noquote() << "main thread: " << QThread::currentThreadId();
    
    edm::can::CanController::instance()->init();

    int can0_index =
        edm::can::CanController::instance()->add_device("can0", 115200);
    edm::can::CanController::instance()->add_device("can1", 115200);

    edm::can::CanController::instance()->add_frame_received_listener(
        "can1", [](const QCanBusFrame &frame) {
            qDebug().noquote()
                << "thread: " << QThread::currentThreadId() << frame.toString();
        });

    edm::io::IOController::instance()->init(can0_index);

    while (!edm::can::CanController::instance()->is_connected("can0") &&
           !edm::can::CanController::instance()->is_connected("can1"))
        ;

    uint32_t tmp = 0x00000000;

    // uint32_t mask = 0x0000000F;
    uint32_t mask = 0b0011;

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        tmp += 0x01;

        edm::io::IOController::instance()->set_can_machineio_1_withmask(tmp,
                                                                        mask);
    });

    t.start(500);

    int ret = app.exec();

    return ret;
}