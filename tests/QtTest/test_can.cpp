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

    edm::can::CanController can_ctrl("can0", 115200);

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        QByteArray array{8, 0x16};

        array.resize(8);
        // array.append(0x11)
        QCanBusFrame frame(0x123, array);
        can_ctrl.send_frame(frame);
    });

    t.start(1000);

    return app.exec();
}