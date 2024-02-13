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

    edm::can::CanController can_ctrl("can0");

    return app.exec();
}