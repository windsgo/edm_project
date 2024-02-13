#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "Utils/Netif/netif_utils.h"

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

bool exist_can0() {
    auto devices = QCanBus::instance()->availableDevices("socketcan");

    for (const auto &dev : devices) {
        if (dev.name() == "can0") {
            return true;
        }
    }

    return false;
}

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
        // qDebug() << "*** " << device->state() << device->errorString()
        //          << device->busStatus();

        if (device->state() != QCanBusDevice::ConnectedState) {
            s_root_logger->debug("--- reconnecting");

            // if (!exist_can0()) return; 
            // 需要系统层面进行检查是否存在can0设备, qt无法进行,
            // 要么就不停调用下面的代码对can0尝试up和connect
            if (!edm::util::is_netdev_exist("can0")) {
                return;
            }

            int a = system("sudo ip link set can0 down");
            int b = system("sudo ip link set can0 up type can bitrate 115200");
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