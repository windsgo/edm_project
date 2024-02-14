#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "Utils/Netif/netif_utils.h"

#include "QtDependComponents/CanController/CanController.h"

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

int main(int argc, char **argv) {

    s_root_logger->debug("test can");

    QCoreApplication app(argc, argv);

    qDebug().noquote() << "main thread: " << QThread::currentThreadId();

    edm::can::CanController::instance()->add_device("can0", 115200);
    edm::can::CanController::instance()->add_device("can1", 115200);

    edm::can::CanController::instance()->add_frame_received_listener(
        "can1", [](const QCanBusFrame &frame) {
            qDebug().noquote() << "thread: " << QThread::currentThreadId() << frame.toString();
        });

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        QByteArray array{8, 0x16};
        array.resize(8);
        QCanBusFrame frame0(0x147, array);
        edm::can::CanController::instance()->send_frame("can0", frame0);

        array.fill(0xEF);
        QCanBusFrame frame1(0x258, array);
        edm::can::CanController::instance()->send_frame("can1", frame1);

        static int i = 0;
        ++i;
        if (i == 4) {
            QCoreApplication::exit(0);
        }
    });

    t.start(1000);

    return app.exec();
}