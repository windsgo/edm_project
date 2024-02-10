#include "edm.h"

#include <QCoreApplication>
#include <QTimer>
#include <QCanBus>

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());


int main (int argc, char** argv) {

    s_root_logger->debug("test qt");

    QCoreApplication app(argc, argv);

    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        s_root_logger->debug("hi");
    });

    t.start(1000);

    auto plugins = QCanBus::instance()->plugins();
    for (const auto& plugin: plugins) {
        s_root_logger->debug("plugin: {}", plugin.toStdString());
    }

    auto devices = QCanBus::instance()->availableDevices("socketcan");

    for (const auto& dev: devices) {
        s_root_logger->debug("{}", dev.name().toStdString());
    }

    int a = 0x1234;
    int b = 0xF;
    s_root_logger->debug("{:08x}, {:02x}", a, b);

    return app.exec();
}