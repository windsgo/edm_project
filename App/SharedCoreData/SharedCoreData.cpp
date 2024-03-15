#include "SharedCoreData.h"

#include <QCoreApplication>

#include <random>

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace app {

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

SharedCoreData::SharedCoreData(QObject *parent)
    : QObject(parent), random_device_(), gen_(random_device_()),
      uniform_real_distribution_(-1.0, 1.0) {
    _init_data();
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

    // add can device
    const auto &can_device_name = sys_settings_.get_can_device_name();
    int can_device_index =
        can_ctrler_->add_device(QString::fromStdString(can_device_name),
                                sys_settings_.get_can_device_bitrate());

    if (can_device_index < 0) {
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
        if (this->can_ctrler_->is_connected(can_device_index)
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

    io_ctrler_ =
        std::make_shared<io::IOController>(can_ctrler_, can_device_index);

    power_ctrler_ = std::make_shared<power::PowerController>(
        can_ctrler_, io_ctrler_, can_device_index);

    // init servodata and adcinfo receive-holder
    can_recv_buffer_ =
        std::make_shared<CanReceiveBuffer>(can_ctrler_, can_device_index);

    _init_handbox_converter(can_device_index);

    // init move ...
    motion_signal_queue_ = std::make_shared<move::MotionSignalQueue>();
    motion_cmd_queue_ = std::make_shared<move::MotionCommandQueue>();

    // init cb used to init motion thread ctrler
    _init_motionthread_cb();

    motion_thread_ctrler_ = std::make_shared<move::MotionThreadController>(
        sys_settings_.get_ecat_netif_name(), motion_cmd_queue_,
        motion_signal_queue_, cb_enable_votalge_gate_, cb_get_servo_cmd_,
        cb_get_touch_physical_detected_, sys_settings_.get_ecat_iomap_size(),
        EDM_SERVO_NUM);

    // init info dispatcher
    info_dispatcher_ = new InfoDispatcher(motion_signal_queue_,
                                          motion_thread_ctrler_, this, 20);

    // init power manager
    power_manager_ = new PowerManager(io_ctrler_, power_ctrler_, 1000, this);
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
    cb_get_touch_physical_detected_ = [this]() -> bool {
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
        return this->manual_touch_detect_flag_;
#else // EDM_OFFLINE_MANUAL_TOUCH_DETECT
        Can1IOBoard407ServoData sd;
        this->can_recv_buffer_->load_servo_data(sd);

        return sd.touch_detected;

#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT
    };

    cb_get_servo_cmd_ = [this]() -> double {
#ifdef EDM_OFFLINE_MANUAL_SERVO_CMD
        double amplitude_um = this->manual_servo_cmd_feed_amplitude_um_;
        double probability_offset =
            (this->manual_servo_cmd_feed_probability_ - 0.50);

        double rd =
            uniform_real_distribution_(gen_); // rd 在 -1.0, 1.0之间正太分布

        // 根据 probability_offset 进行概率偏移
        rd += probability_offset * 2;
        if (rd > 1.0)
            rd = 1.0;
        else if (rd < -1.0)
            rd = -1.0;

        return util::UnitConverter::um2blu(rd * amplitude_um);
#else // EDM_OFFLINE_MANUAL_SERVO_CMD
        Can1IOBoard407ServoData sd;
        this->can_recv_buffer_->load_servo_data(sd);

        double um_ret = 0.0;
        if (sd.servo_direction > 0) {
            um_ret = (double)sd.servo_distance_0_01um / 100.0;
        } else {
            um_ret = (-1.0) * (double)sd.servo_distance_0_01um / 100.0;
        }

        return util::UnitConverter::um2blu(um_ret);

#endif // EDM_OFFLINE_MANUAL_SERVO_CMD
    };

    cb_enable_votalge_gate_ = [this](bool arg) -> void {
        s_logger->debug("push voltage gate command: {}", arg);

        auto run_cmd = global::CommandCommonFunctionFactory::bind(
            [this](bool _enable) {
                this->power_ctrler_->set_machbit_on(_enable);
            },
            arg);

        // thread safe call
        this->global_cmd_queue_->push_command(run_cmd);
    };
}

} // namespace app

} // namespace edm
