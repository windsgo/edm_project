#include "SharedCoreData.h"

#include <QCoreApplication>

#include <qhostaddress.h>
#include <random>
#include <thread>

#include "Logger/LogMacro.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace app {

// 用于在G01抬刀时的电压关断, 向主线程发送一个事件, 让主线程来执行电源操作,
// 因为PowerController不是线程安全的
class MotionEventVoltageEnable : public QEvent {
public:
    MotionEventVoltageEnable(bool voltage_enable)
        : QEvent(type), voltage_enable_(voltage_enable) {}
    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_MotionVoltageEnable);

    auto voltage_enable() const { return voltage_enable_; }

private:
    bool voltage_enable_;
};

class MotionEventMachOn : public QEvent {
public:
    MotionEventMachOn(bool mach_on) : QEvent(type), mach_on_(mach_on) {}
    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_MotionMachOn);

    auto mach_on() const { return mach_on_; }

private:
    bool mach_on_;
};

class HandBoxEventStartPointMove : public QEvent {
public:
    HandBoxEventStartPointMove(const move::axis_t &dir, uint32_t speed_level,
                               bool touch_detect_enable)
        : QEvent(type), dir_(dir), speed_level_(speed_level),
          touch_detect_enable_(touch_detect_enable) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxStartPM);

    inline const auto &dir() const { return dir_; }
    inline auto speed_level() const { return speed_level_; }
    inline auto touch_detect_enable() const { return touch_detect_enable_; }

private:
    move::axis_t dir_;
    uint32_t speed_level_;
    bool touch_detect_enable_;
};

class HandBoxEventStopPointMove : public QEvent {
public:
    HandBoxEventStopPointMove() : QEvent(type) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxStopPM);
};

class HandBoxEventPump : public QEvent {
public:
    HandBoxEventPump(bool pump_on) : QEvent(type), pump_on_(pump_on) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxPump);

    auto pump_on() const { return pump_on_; }

private:
    bool pump_on_;
};

class HandBoxEventEntAuto : public QEvent {
public:
    HandBoxEventEntAuto() : QEvent(type) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxEntAuto);
};

class HandBoxEventPauseAuto : public QEvent {
public:
    HandBoxEventPauseAuto() : QEvent(type) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxPauseAuto);
};

class HandBoxEventStopAuto : public QEvent {
public:
    HandBoxEventStopAuto() : QEvent(type) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxStopAuto);
};

class HandBoxEventAck : public QEvent {
public:
    HandBoxEventAck() : QEvent(type) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(EDM_CUSTOM_QTEVENT_TYPE_HandboxAck);
};

struct metatype_register__ {
    metatype_register__() {
        qRegisterMetaType<edm::move::axis_t>("edm::move::axis_t");
        qRegisterMetaType<edm::power::EleParam_dkd_t>(
            "edm::power::EleParam_dkd_t");
    }
};
static struct metatype_register__ mt_register__;

struct eventtype_register__ {
    eventtype_register__() {
        QEvent::registerEventType(HandBoxEventStartPointMove::type);
        QEvent::registerEventType(HandBoxEventStopPointMove::type);
        QEvent::registerEventType(HandBoxEventPump::type);
        QEvent::registerEventType(HandBoxEventEntAuto::type);
        QEvent::registerEventType(HandBoxEventPauseAuto::type);
        QEvent::registerEventType(HandBoxEventStopAuto::type);
        QEvent::registerEventType(HandBoxEventAck::type);
    }
};
static struct eventtype_register__ et_register;

SharedCoreData::SharedCoreData(QObject *parent) : QObject(parent) {
    _init_data();
}

SharedCoreData::~SharedCoreData() {
#ifdef EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
    helper_can_simulate_thread_stop_flag_ = true;
    helper_can_simulate_thread_.join();
#endif // EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
}

void SharedCoreData::send_ioboard_bz_once() const {
    power::CanIOBoardCommonMessageStrc ms;
    ms.message_type =
        power::CommonMessageType_t::CommomMessageType_BZOnceNotify;

#ifndef EDM_OFFLINE_RUN_NO_CAN
    QCanBusFrame frame(EDM_CAN_TXID_IOBOARD_COMMONMESSAGE,
                       QByteArray((const char *)(&ms), 8));
    can_ctrler_->send_frame(can_device_index_, frame);
#endif // EDM_OFFLINE_RUN_NO_CAN
}

void SharedCoreData::customEvent(QEvent *e) {
    auto type = e->type();

    s_logger->debug("{}, event: {}", __PRETTY_FUNCTION__, (int)type);

    switch (type) {
    case HandBoxEventStartPointMove::type: {
        auto he = static_cast<HandBoxEventStartPointMove *>(e);
        emit sig_handbox_start_pointmove(he->dir(), he->speed_level(),
                                         he->touch_detect_enable());
        e->accept();
        break;
    }
    case HandBoxEventStopPointMove::type: {
        emit sig_handbox_stop_pointmove();
        e->accept();
        break;
    }
    case HandBoxEventPump::type: {
        auto he = static_cast<HandBoxEventPump *>(e);
        emit sig_handbox_pump(he->pump_on());
        e->accept();
        break;
    }
    case HandBoxEventEntAuto::type: {
        emit sig_handbox_ent_auto();
        e->accept();
        break;
    }
    case HandBoxEventPauseAuto::type: {
        emit sig_handbox_pause_auto();
        e->accept();
        break;
    }
    case HandBoxEventStopAuto::type: {
        emit sig_handbox_stop_auto();
        e->accept();
        break;
    }
    case HandBoxEventAck::type: {
        emit sig_handbox_ack();
        e->accept();
        break;
    }

    case MotionEventVoltageEnable::type: {
        auto vol_enable_event = static_cast<MotionEventVoltageEnable *>(e);
        this->power_manager_->set_machbit_on(
            vol_enable_event->voltage_enable());
        e->accept();
        break;
    };

    case MotionEventMachOn::type: {
        auto mach_on_event = static_cast<MotionEventMachOn *>(e);
        this->power_manager_->set_highpower_on(mach_on_event->mach_on());
        this->power_ctrler_->update_eleparam_and_send();
        e->accept();
        break;
    }

    default:
        break;
    }
}

void SharedCoreData::_init_data() {
    // init coord
    coord_system_ = std::make_shared<coord::CoordinateSystem>(
        EDM_CONFIG_DIR + sys_settings_.get_coord_config_file());

    // init global cmd queue
    global_cmd_queue_ = std::make_shared<global::GlobalCommandQueue>();

    // init can ...
    can_ctrler_ = std::make_shared<can::CanController>();

#ifndef EDM_OFFLINE_RUN_NO_CAN
    // add can device
    const auto &can_device_name = sys_settings_.get_can_device_name();
    can_device_index_ =
        can_ctrler_->add_device(QString::fromStdString(can_device_name),
                                sys_settings_.get_can_device_bitrate());

    if (can_device_index_ < 0) {
        throw exception(
            EDM_FMT::format("add can device failed: {}", can_device_name));
    }

#ifdef EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
    const auto &helper_can_device_name =
        sys_settings_.get_helper_can_device_name();
    int helper_can_device_index =
        can_ctrler_->add_device(QString::fromStdString(helper_can_device_name),
                                sys_settings_.get_can_device_bitrate());

    if (helper_can_device_index < 0) {
        throw exception(EDM_FMT::format("add helper_can device failed: {}",
                                        helper_can_device_name));
    }
#endif // EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE

    QEventLoop el;
    QTimer t;
    connect(&t, &QTimer::timeout, this, [&]() {
        if (this->can_ctrler_->is_connected(can_device_index_)
#ifdef EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
            && this->can_ctrler_->is_connected(helper_can_device_index)
#endif // EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
        ) {
            t.stop();
            el.quit();
            s_logger->info("** can all connected");
        } else {
            s_logger->debug("** can connecting...");
        }
    });
    t.start(1000);
    el.exec();

#ifdef EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE
    helper_can_simulate_thread_ = std::thread(
        [this](int helper_can_index) {
            QByteArray ba1{8, 0x0c};
            QCanBusFrame frame1{EDM_CAN_RXID_IOBOARD_SERVODATA, ba1};
            while (!helper_can_simulate_thread_stop_flag_) {
                can_ctrler_->send_frame(helper_can_index, frame1);
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }
        },
        helper_can_device_index);
#endif // EDM_OFFLINE_RUN_MANUAL_TWO_CAN_DEVICE

#else  // EDM_OFFLINE_RUN_NO_CAN
    int can_device_index = -1;
#endif // EDM_OFFLINE_RUN_NO_CAN

    io_ctrler_ =
        std::make_shared<io::IOController>(can_ctrler_, can_device_index_);

    power_ctrler_ = std::make_shared<power::PowerController>(
        can_ctrler_, io_ctrler_, can_device_index_);

    // init servodata and adcinfo receive-holder
    can_recv_buffer_ =
        std::make_shared<CanReceiveBuffer>(can_ctrler_, can_device_index_);

    _init_handbox_converter(can_device_index_);

    // init move ...
    motion_signal_queue_ = std::make_shared<move::MotionSignalQueue>();
    motion_cmd_queue_ = std::make_shared<move::MotionCommandQueue>();

    // init cb used to init motion thread ctrler
    _init_motionthread_cb();

    motion_thread_ctrler_ = std::make_shared<move::MotionThreadController>(
        sys_settings_.get_ecat_netif_name(), motion_cmd_queue_,
        motion_signal_queue_, cb_enable_votalge_gate_, cb_mach_on_,
        can_recv_buffer_, sys_settings_.get_ecat_iomap_size(), EDM_SERVO_NUM);

    // init info dispatcher
    info_dispatcher_ =
        new InfoDispatcher(motion_signal_queue_, motion_thread_ctrler_, this,
                           sys_settings_.get_info_dispatcher_peroid_ms());

    // init power manager
    power_manager_ = new PowerManager(io_ctrler_, power_ctrler_,
                                      motion_cmd_queue_, 1000, this);
    
    // TODO
    // FIXME
    // test
    zynq_connect_ctrler_ = std::make_shared<zynq::ZynqConnectController>(QHostAddress::LocalHost, 12345, 22222);

    // 获取运动线程中的记录器指针, 用于操作开始结束
    record_data1_queuerecorder_ =
        move::MotionSharedData::instance()->get_record_data1_queuerecorder();
}

void SharedCoreData::_init_handbox_converter(uint32_t can_index) {

    auto cb_start_pm = [this](const move::axis_t &dir, uint32_t speed_level,
                              bool touch_detect_enable) {
        // send an event to this object, then emit a signal, to trigger move
        // slot
        auto e = new HandBoxEventStartPointMove(dir, speed_level,
                                                touch_detect_enable);
        QCoreApplication::postEvent(this, e);
    };

    auto cb_stop_pm = [this]() {
        // send an event to this object, then emit a signal, to trigger move
        // slot
        auto e = new HandBoxEventStopPointMove();
        QCoreApplication::postEvent(this, e);
    };

    auto cb_pause_auto = [this]() {
        auto e = new HandBoxEventPauseAuto();
        QCoreApplication::postEvent(this, e);
    };
    auto cb_ent_auto = [this]() {
        auto e = new HandBoxEventEntAuto();
        QCoreApplication::postEvent(this, e);
    };
    auto cb_stop_auto = [this]() {
        auto e = new HandBoxEventStopAuto();
        QCoreApplication::postEvent(this, e);
    };

    auto cb_ack = [this]() {
        auto e = new HandBoxEventAck();
        QCoreApplication::postEvent(this, e);
    };

    auto cb_pump_on = [this](bool on) {
        auto e = new HandBoxEventPump(on);
        QCoreApplication::postEvent(this, e);
    };

    handbox_converter_ = std::make_shared<HandboxConverter>(
        can_ctrler_, can_index, cb_start_pm, cb_stop_pm, cb_pause_auto,
        cb_ent_auto, cb_stop_auto, cb_ack, cb_pump_on);
}

void SharedCoreData::_init_motionthread_cb() {
    cb_enable_votalge_gate_ = [this](bool arg) -> void {
        s_logger->debug("push voltage gate command: {}", arg);

        // postevent本身可能会耗时较多但是线程安全,
        // 而global_cmd_queue_这个队列是相当空闲的 操作电压的操作无须特别实时,
        // 所以优先保证motion操作电压的函数可以快速返回
        auto run_cmd = global::CommandCommonFunctionFactory::bind(
            [this](bool _enable) {
                // send 3 times
                QCoreApplication::postEvent(
                    this, new MotionEventVoltageEnable(_enable));
            },
            arg);

        // thread safe call
        this->global_cmd_queue_->push_command(run_cmd);
    };

    cb_mach_on_ = [this](bool arg) -> void {
        auto run_cmd = global::CommandCommonFunctionFactory::bind(
            [this](bool _mach_on) {
                QCoreApplication::postEvent(this,
                                            new MotionEventMachOn(_mach_on));
            },
            arg);

        // thread safe call
        this->global_cmd_queue_->push_command(run_cmd);
    };
}

} // namespace app

} // namespace edm
