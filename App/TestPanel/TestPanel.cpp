#include "TestPanel.h"
#include "Coordinate/Coordinate.h"
#include "Coordinate/CoordinateManager.h"
#include "Coordinate/CoordinateSystem.h"
#include "Motion/MoveDefines.h"
#include "TaskManager/TaskHelper.h"
#include "ui_TestPanel.h"

#include <QSlider>
#include <qaudiodeviceinfo.h>
#include <qpushbutton.h>

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

TestPanel::TestPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::TestPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    _update_phy_touchdetect();
    _update_servo();
    _update_manual_voltage();

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    connect(shared_core_data_->get_info_dispatcher(),
            &InfoDispatcher::info_updated, this,
            [this](const edm::move::MotionInfo &info) {
                ui->dsb_show_spindle_blu->setValue(info.spindle_axis_blu);
            });

    connect(ui->pb_start_spindle, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSetSpindleState>(true);
        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);

        auto ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 1000);
        if (!ret) {
            s_logger->error("start spindle failed");
        }
    });

    connect(ui->pb_stop_spindle, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSetSpindleState>(false);
        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);

        auto ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 1000);
        if (!ret) {
            s_logger->error("stop spindle failed");
        }
    });

    connect(ui->pb_set_spindle_param, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSetSpindleParam>(
            ui->dsb_spindle_speed->value(), ui->dsb_spindle_acc->value());
        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);

        auto ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 1000);
        if (!ret) {
            s_logger->error("set spindle param failed");
        }
    });
#endif

    connect(
        ui->pb_phy_detected, &QPushButton::clicked, this,
        [this](bool checked [[maybe_unused]]) { _update_phy_touchdetect(); });

    connect(ui->horizontalSlider_servo, &QSlider::valueChanged, this,
            [this](int value [[maybe_unused]]) { _update_servo(); });
    connect(ui->horizontalSlider_servo_2, &QSlider::valueChanged, this,
            [this](int value [[maybe_unused]]) { _update_servo(); });
    connect(ui->horizontalSlider_voltage, &QSlider::valueChanged, this,
            [this](int value [[maybe_unused]]) { _update_manual_voltage(); });

    connect(ui->pb_test_xy_posmove, &QPushButton::pressed, this, [this]() {
#ifdef EDM_OFFLINE_RUN
        if (coord::Coordinate::Size < 3)
            return;

        move::axis_t dir;
        dir[0] = 1.0;
        dir[1] = 1.0;
        dir[2] = 0.0;
        for (std::size_t i = 3; i < dir.size(); ++i) {
            dir[i] = 0.0;
        }

        uint32_t speed_level = 0;
        bool touch_detect_enable = true;

        emit shared_core_data_->sig_handbox_start_pointmove(
            dir, speed_level, touch_detect_enable);
#endif
    });
    connect(ui->pb_test_xy_posmove, &QPushButton::released, this, [this]() {
#ifdef EDM_OFFLINE_RUN
        emit shared_core_data_->sig_handbox_stop_pointmove();
#endif
    });

    connect(ui->pb_test_yz_negmove, &QPushButton::pressed, this, [this]() {
#ifdef EDM_OFFLINE_RUN
        if (coord::Coordinate::Size < 3)
            return;

        move::axis_t dir;
        dir[0] = 0.0;
        dir[1] = -1.0;
        dir[2] = -1.0;
        for (std::size_t i = 3; i < dir.size(); ++i) {
            dir[i] = 0.0;
        }

        uint32_t speed_level = 0;
        bool touch_detect_enable = true;

        emit shared_core_data_->sig_handbox_start_pointmove(
            dir, speed_level, touch_detect_enable);
#endif
    });
    connect(ui->pb_test_yz_negmove, &QPushButton::released, this, [this]() {
#ifdef EDM_OFFLINE_RUN
        emit shared_core_data_->sig_handbox_stop_pointmove();
#endif
    });

    _init_test_director();

#if 0
    _init_audio_test();
#endif
}

#if 0
void TestPanel::_init_audio_test() {
    connect(
        ui->pb_test_start_audio_record, &QPushButton::clicked, this, [this]() {
            // auto ad = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
            // for (const auto &d : ad) {
            //     s_logger->debug("available audio device: {}, realm: {}",
            //                     d.deviceName().toStdString(),
            //                     d.realm().toStdString());
            // }

            audio::AudioStartRecordParam param;
            param.file_path = "test_audio.pcm";
            param.audio_device_info = QAudioDeviceInfo::defaultInputDevice();
            param.audio_format.setSampleRate(192000);
            param.audio_format.setChannelCount(1);
            param.audio_format.setSampleSize(16);
            param.audio_format.setCodec("audio/pcm");
            param.audio_format.setByteOrder(QAudioFormat::LittleEndian);
            param.audio_format.setSampleType(QAudioFormat::SignedInt);

            s_logger->debug(
                "test start record: filename: {}, sample rate: {}, "
                "channel count: {}, sample size: {}, codec: {}, "
                "byte order: {}, sample type: {}",
                param.file_path.toStdString(), param.audio_format.sampleRate(),
                param.audio_format.channelCount(),
                param.audio_format.sampleSize(),
                param.audio_format.codec().toStdString(),
                param.audio_format.byteOrder() == QAudioFormat::LittleEndian
                    ? "LittleEndian"
                    : "BigEndian",
                [param]() {
                    switch (param.audio_format.sampleType()) {
                    case QAudioFormat::SignedInt:
                        return "SignedInt";
                    case QAudioFormat::UnSignedInt:
                        return "UnSignedInt";
                    case QAudioFormat::Float:
                        return "Float";
                    default:
                        return "Unknown";
                    }
                }());
            s_logger->debug("default audio device: {}",
                            param.audio_device_info.deviceName().toStdString());

            shared_core_data_->get_audio_recorder()->start_record(param);
        });

    connect(ui->pb_test_stop_audio_record, &QPushButton::clicked, this, [this]() {
        s_logger->debug("test stop record");
        shared_core_data_->get_audio_recorder()->stop_record();
    });
}
#endif

void TestPanel::_init_test_director() {
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    connect(
        ui->pb_director_start_pointmove, &QPushButton::clicked, this, [this]() {
            auto io_ctrler = this->shared_core_data_->get_io_ctrler();

            io_ctrler->director_start_pointmove(ui->sb_director_inc->value(),
                                                ui->sb_director_speed->value());
        });
    connect(ui->pb_director_stop_pointmove, &QPushButton::clicked, this,
            [this]() {
                auto io_ctrler = this->shared_core_data_->get_io_ctrler();

                io_ctrler->director_stop_pointmove();
            });

    connect(ui->pb_director_move_pos, &QPushButton::pressed, this, [this]() {
        auto io_ctrler = this->shared_core_data_->get_io_ctrler();

        io_ctrler->director_start_pointmove(10000,
                                            ui->sb_director_speed->value());
    });
    connect(ui->pb_director_move_neg, &QPushButton::pressed, this, [this]() {
        auto io_ctrler = this->shared_core_data_->get_io_ctrler();

        io_ctrler->director_start_pointmove(-10000,
                                            ui->sb_director_speed->value());
    });
    connect(ui->pb_director_move_pos, &QPushButton::released, this, [this]() {
        auto io_ctrler = this->shared_core_data_->get_io_ctrler();

        io_ctrler->director_stop_pointmove();
    });
    connect(ui->pb_director_move_neg, &QPushButton::released, this, [this]() {
        auto io_ctrler = this->shared_core_data_->get_io_ctrler();

        io_ctrler->director_stop_pointmove();
    });

    connect(ui->pb_director_start_homemove, &QPushButton::clicked, this,
            [this]() {
                auto io_ctrler = this->shared_core_data_->get_io_ctrler();

                io_ctrler->director_start_homemove(
                    ui->sb_director_home_back_speed->value(),
                    ui->sb_director_home_forward_speed->value());
            });

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (this->isVisible() == false)
            return;

        auto d = shared_core_data_->get_io_ctrler()->get_director_state();

        ui->sb_director_curr_pos->setValue(d.curr_step);
        ui->sb_director_curr_state->setValue(d.director_state);
    });

    timer->start(250);
#endif
}

TestPanel::~TestPanel() { delete ui; }

void TestPanel::_update_phy_touchdetect() {
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    shared_core_data_->get_zynq_udpmessage_holder()
        ->set_manual_touch_detect_flag(
#else
    shared_core_data_->get_can_recv_buffer()->set_manual_touch_detect_flag(
#endif // !EDM_USE_ZYNQ_SERVOBOARD
            ui->pb_phy_detected->isChecked());
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT
}

void TestPanel::_update_servo() {
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    shared_core_data_->get_zynq_udpmessage_holder()->set_manual_servo_cmd(
#else
    shared_core_data_->get_can_recv_buffer()->set_manual_servo_cmd(
#endif // EDM_USE_ZYNQ_SERVOBOARD
        (double)ui->horizontalSlider_servo->value() / 100.0,
        util::UnitConverter::um_ms2blu_p(
            (double)ui->horizontalSlider_servo_2->value() / 10.0));
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT
}

void TestPanel::_update_manual_voltage() {
#ifdef EDM_OFFLINE_MANUAL_VOLTAGE
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    shared_core_data_->get_zynq_udpmessage_holder()->set_manual_voltage(
#else
    shared_core_data_->get_can_recv_buffer()->set_manual_voltage(
#endif // EDM_USE_ZYNQ_SERVOBOARD
        ui->horizontalSlider_voltage->value());
#endif // EDM_OFFLINE_MANUAL_VOLTAGE
}

} // namespace app
} // namespace edm
