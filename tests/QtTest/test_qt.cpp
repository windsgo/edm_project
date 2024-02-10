#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

int main(int argc, char **argv) {

    s_root_logger->debug("test qt");

    QCoreApplication app(argc, argv);

    auto plugins = QCanBus::instance()->plugins();
    for (const auto &plugin : plugins) {
        s_root_logger->debug("plugin: {}", plugin.toStdString());
    }

    auto devices = QCanBus::instance()->availableDevices("socketcan");

    for (const auto &dev : devices) {
        s_root_logger->debug("{}", dev.name().toStdString());
    }

    qDebug() << system("sudo ip link set can0 down");
    qDebug() << system("sudo ip link set can0 up type can bitrate 115200");

    QString err;
    auto device = QCanBus::instance()->createDevice("socketcan", "can0", &err);
    // auto device = new QCanBusDevice();
    if (!device) {
        s_root_logger->debug("create failed: {}");
        return 1;
    } else {
        s_root_logger->debug("create success");
    }

    device->setConfigurationParameter(QCanBusDevice::BitRateKey, QVariant());
    device->connectDevice();

    QObject::connect(device, &QCanBusDevice::framesReceived,
                     [&]() { qDebug() << "Frame received"; });

    QObject::connect(device, &QCanBusDevice::errorOccurred,
                     [&](QCanBusDevice::CanBusError err) {
                         qDebug() << "--- error: " << err;
                         device->disconnectDevice();
                     });


    QTimer t;
    QObject::connect(&t, &QTimer::timeout, [&]() {
        qDebug() << "*** " << device->state() << device->errorString()
                 << device->busStatus();

        if (device->state() != QCanBusDevice::ConnectedState) {
            qDebug() << system("sudo ip link set can0 down");
            qDebug() << system("sudo ip link set can0 up type can bitrate 115200");
            bool ret = device->connectDevice();
            s_root_logger->debug("--- reconnect: {}", ret);
        } else {

            QByteArray array{8, 0x16};
            // array.append(0x11)
            QCanBusFrame frame(0x123, array);
            device->writeFrame(frame);
        }
    });

    t.start(1000);

    // int a = 0x1234;
    // int b = 0xF;
    // s_root_logger->debug("{:08x}, {:02x}", a, b);

    return app.exec();
}