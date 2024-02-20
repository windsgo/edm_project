#include "edm.h"

#include <QCanBus>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"
#include "QtDependComponents/PowerController/EleparamDecoder.h"
#include "QtDependComponents/PowerController/EleparamDefine.h"
#include "QtDependComponents/PowerController/PowerController.h"

#include "Utils/Format/edm_format.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

using namespace edm;

static void test_decode() {
    s_logger->debug("io1 mask: {:032B}",
                    power::EleparamDecodeResult::get_io_1_mask());
    s_logger->debug("io2 mask: {:032B}",
                    power::EleparamDecodeResult::get_io_2_mask());

    QByteArray a{8, 0x00};
    a[0] = 0b10110000;
    a[0] = a[0] | 0b01000001;

    uint8_t out = a[0];

    s_logger->debug("out = {:08B}", out);

    uint16_t s = 0x12F4;
    a[1] = s;
    a[2] = static_cast<uint8_t>(s);

    s_logger->debug("a[1] = {:#X}, a[2] = {:#X}", (uint8_t)a[1], (uint8_t)a[2]);

    auto eleparam = std::make_shared<power::EleParam_dkd_t>();

    auto print_bytearray = [](const QByteArray &ba) {
        std::stringstream ss;
        for (int i = 0; i < ba.size(); ++i) {
            ss << EDM_FMT::format("{:02X} ", (uint8_t)ba[i]);
        }

        s_logger->debug("{}", ss.str());
    };

    auto print_res = [&](power::EleparamDecodeResult::ptr res) {
        s_logger->debug("io1: {:032B}, masked io1: {:032B}", res->io_1(),
                        res->io_1() & res->get_io_1_mask());
        s_logger->debug("io2: {:032B}, masked io2: {:032B}", res->io_2(),
                        res->io_2() & res->get_io_2_mask());

        print_bytearray(res->can_buffer()[0]);
        print_bytearray(res->can_buffer()[1]);
    };

    auto input1 =
        std::make_shared<power::EleparamDecodeInput>(eleparam, 1, 1, 0x05);
    auto res1 = power::EleparamDecoder::decode(input1);
    print_res(res1);

    eleparam->c = 1;
    eleparam->pulse_on = 10;
    eleparam->pulse_off = 9;
    eleparam->pl = 1;
    eleparam->hp = 20;
    eleparam->pp = 10;
    auto input2 =
        std::make_shared<power::EleparamDecodeInput>(eleparam, 0, 0, 0x05);
    auto res2 = power::EleparamDecoder::decode(input2);
    print_res(res2);
}

static auto eleparam = std::make_shared<power::EleParam_dkd_t>();

static void test_power_control() {

    can::CanController::instance()->init();
    int index0 = can::CanController::instance()->add_device("can0", 115200);
    int index1 = can::CanController::instance()->add_device("can1", 115200);

    io::IOController::instance()->init(index0);
    power::PowerController::instance()->init(index0);

    while (!can::CanController::instance()->is_connected("can0") ||
           !can::CanController::instance()->is_connected("can1"))
        ;

    can::CanController::instance()->add_frame_received_listener(
        "can1",
        [&](const QCanBusFrame &frame) { qDebug() << frame.toString(); });
    
    // init its eleparam
    eleparam->sv = 0xEF;
    eleparam->ip = 0x10;

    power::PowerController::instance()->update_eleparam_and_send(eleparam);

    QTimer *t1 = new QTimer();
    QObject::connect(t1, &QTimer::timeout, [&]() {
        power::PowerController::instance()->trigger_send_ioboard_eleparam();
    });

    t1->start(1000);


    QTimer *t2 = new QTimer();
    QObject::connect(t2, &QTimer::timeout, [&]() {
        power::PowerController::instance()->trigger_send_eleparam();
    });

    t2->start(1000);
}

int main(int argc, char **argv) {

    s_logger->debug("test power");

    QCoreApplication app(argc, argv);

    can::CanController::instance()->init();

    // test_decode();

    test_power_control();

    return app.exec();
}