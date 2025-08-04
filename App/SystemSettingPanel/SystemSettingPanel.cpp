#include "SystemSettingPanel.h"
#include "ADCCalcPanel/ADCCalcPanel.h"
#include "Motion/MotionThread/MotionCommand.h"
#include "Motion/MoveDefines.h"
#include "QtDependComponents/ZynqConnection/TcpMessageDefine.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
#include "TaskManager/TaskHelper.h"
#include "ui_SystemSettingPanel.h"

#include "SystemSettings/SystemSettings.h"

#include <QMessageBox>
#include <memory>
#include <qpushbutton.h>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

static auto &s_sys_setting = SystemSettings::instance();

SystemSettingPanel::SystemSettingPanel(SharedCoreData *shared_core_data,
                                       QWidget *parent)
    : QWidget(parent), ui(new Ui::SystemSettingPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    adc_calc_panel_ = new ADCCalcPanel(nullptr);
    adc_calc_panel_->setWindowFlags(adc_calc_panel_->windowFlags() | Qt::Tool);
    connect(adc_calc_panel_, &ADCCalcPanel::sig_result_applied, this,
            [this](const ADCCalcPanel::ADCCalcResult &result) {
                ui->dsb_adc_gain->setValue(result.new_k);
                ui->dsb_adc_offset->setValue(result.new_b);
            });

    connect(
        shared_core_data_->get_zynq_connect_ctrler().get(),
        &zynq::ZynqConnectController::sig_zynq_tcp_connected, this, [this]() {
            this->_set_adc_settings_to_zynq();
            s_logger->info("zynq reconnected, send adc settings immediately");
        });

    connect(ui->pb_open_adccalc, &QPushButton::clicked, this, [this]() {
        adc_calc_panel_->slot_set_current_k_and_b_to_ui(
            ui->dsb_adc_gain->value(), ui->dsb_adc_offset->value());

        adc_calc_panel_->show();
    });

#if !(EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    ui->tab_drill->hide();
#endif

    _init_button_cb();
    _update_ui();

    _do_save(); // set once

    _init_voffset_page_buttons();
    _init_voffset_page_infosignals();
}

SystemSettingPanel::~SystemSettingPanel() {
    adc_calc_panel_->hide();
    delete adc_calc_panel_;

    delete ui;
}

void SystemSettingPanel::_init_voffset_page_buttons() {

#define XX_SEND_CMD__(cmd__, desc__)                                      \
    do {                                                                  \
        shared_core_data_->get_motion_cmd_queue()->push_command(cmd__);   \
        bool ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd__, 200);  \
        if (!ret) {                                                       \
            QMessageBox::critical(this, desc__,                           \
                                  QString("send ") + desc__ + " failed"); \
            emit this->shared_core_data_->sig_error_message(              \
                QString("send ") + desc__ + " failed", 2000);             \
        } else {                                                          \
            emit this->shared_core_data_->sig_info_message(               \
                QString("send ") + desc__ + " success", 2000);            \
        }                                                                 \
    } while (0)

#define XX_CONNECT_VOFF_ADD_N__(index__)                                       \
    do {                                                                       \
        connect(                                                               \
            ui->pb_voff_add_##index__, &QPushButton::clicked, this, [this]() { \
                auto cmd =                                                     \
                    std::make_shared<move::MotionCommandSettingTestVOffset>(   \
                        move::MotionCommandSettingTestVOffset::VOffsetSetCmd{  \
                            index__,                                           \
                            move::MotionCommandSettingTestVOffset::            \
                                VOffsetSetCmdType::IncValue,                   \
                            util::UnitConverter::mm2blu(0.01)});               \
                XX_SEND_CMD__(cmd, "voff add");                                \
            });                                                                \
    } while (0)

#define XX_CONNECT_VOFF_DEC_N__(index__)                                       \
    do {                                                                       \
        connect(                                                               \
            ui->pb_voff_dec_##index__, &QPushButton::clicked, this, [this]() { \
                auto cmd =                                                     \
                    std::make_shared<move::MotionCommandSettingTestVOffset>(   \
                        move::MotionCommandSettingTestVOffset::VOffsetSetCmd{  \
                            index__,                                           \
                            move::MotionCommandSettingTestVOffset::            \
                                VOffsetSetCmdType::IncValue,                   \
                            -util::UnitConverter::mm2blu(0.01)});              \
                XX_SEND_CMD__(cmd, "voff dec");                                \
            });                                                                \
    } while (0)

#define XX_CONNECT_VOFF_CLEAR_N__(index__)                                    \
    do {                                                                      \
        connect(                                                              \
            ui->pb_voff_clear_##index__, &QPushButton::clicked, this,         \
            [this]() {                                                        \
                auto cmd =                                                    \
                    std::make_shared<move::MotionCommandSettingTestVOffset>(  \
                        move::MotionCommandSettingTestVOffset::VOffsetSetCmd{ \
                            index__,                                          \
                            move::MotionCommandSettingTestVOffset::           \
                                VOffsetSetCmdType::SetValue,                  \
                            0});                                              \
                XX_SEND_CMD__(cmd, "voff clear");                             \
            });                                                               \
    } while (0)

#define XX_CONNECT_VOFF_SET_VALUE_N__(index__)                                \
    do {                                                                      \
        connect(                                                              \
            ui->pb_voff_set_value_##index__, &QPushButton::clicked, this,     \
            [this]() {                                                        \
                auto value = ui->dsb_set_voff_value_##index__->value();       \
                auto cmd =                                                    \
                    std::make_shared<move::MotionCommandSettingTestVOffset>(  \
                        move::MotionCommandSettingTestVOffset::VOffsetSetCmd{ \
                            index__,                                          \
                            move::MotionCommandSettingTestVOffset::           \
                                VOffsetSetCmdType::SetValue,                  \
                            util::UnitConverter::mm2blu(value)});             \
                XX_SEND_CMD__(cmd, "voff set value");                         \
            });                                                               \
    } while (0)

#define XX_CONNECT_VOFF_FORCED_ZERO_N__(index__)                              \
    do {                                                                      \
        connect(                                                              \
            ui->pb_voff_forced_zero_##index__, &QPushButton::clicked, this,   \
            [this](bool checked) {                                            \
                auto cmd =                                                    \
                    std::make_shared<move::MotionCommandSettingTestVOffset>(  \
                        move::MotionCommandSettingTestVOffset::VOffsetSetCmd{ \
                            index__,                                          \
                            move::MotionCommandSettingTestVOffset::           \
                                VOffsetSetCmdType::ForcedZero,                \
                            checked ? 1.0 : 0.0});                            \
                XX_SEND_CMD__(cmd, "voff forced zero");                       \
            });                                                               \
    } while (0)

    XX_CONNECT_VOFF_ADD_N__(0);
    XX_CONNECT_VOFF_ADD_N__(1);
    XX_CONNECT_VOFF_ADD_N__(2);
    XX_CONNECT_VOFF_ADD_N__(3);
    XX_CONNECT_VOFF_ADD_N__(4);
    XX_CONNECT_VOFF_ADD_N__(5);

    XX_CONNECT_VOFF_DEC_N__(0);
    XX_CONNECT_VOFF_DEC_N__(1);
    XX_CONNECT_VOFF_DEC_N__(2);
    XX_CONNECT_VOFF_DEC_N__(3);
    XX_CONNECT_VOFF_DEC_N__(4);
    XX_CONNECT_VOFF_DEC_N__(5);

    XX_CONNECT_VOFF_CLEAR_N__(0);
    XX_CONNECT_VOFF_CLEAR_N__(1);
    XX_CONNECT_VOFF_CLEAR_N__(2);
    XX_CONNECT_VOFF_CLEAR_N__(3);
    XX_CONNECT_VOFF_CLEAR_N__(4);
    XX_CONNECT_VOFF_CLEAR_N__(5);

    XX_CONNECT_VOFF_SET_VALUE_N__(0);
    XX_CONNECT_VOFF_SET_VALUE_N__(1);
    XX_CONNECT_VOFF_SET_VALUE_N__(2);
    XX_CONNECT_VOFF_SET_VALUE_N__(3);
    XX_CONNECT_VOFF_SET_VALUE_N__(4);
    XX_CONNECT_VOFF_SET_VALUE_N__(5);

    XX_CONNECT_VOFF_FORCED_ZERO_N__(0);
    XX_CONNECT_VOFF_FORCED_ZERO_N__(1);
    XX_CONNECT_VOFF_FORCED_ZERO_N__(2);
    XX_CONNECT_VOFF_FORCED_ZERO_N__(3);
    XX_CONNECT_VOFF_FORCED_ZERO_N__(4);
    XX_CONNECT_VOFF_FORCED_ZERO_N__(5);
#if 0
    connect(ui->pb_voff_add_0, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSettingTestVOffset>(
            move::MotionCommandSettingTestVOffset::VOffsetSetCmd{
                0,
                move::MotionCommandSettingTestVOffset::VOffsetSetCmdType::
                    IncValue,
                util::UnitConverter::mm2blu(0.01)});

        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);

        bool ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 200);

        if (!ret) {
            QMessageBox::critical(this, "send voff add failed",
                                  QString("send voff add failed"));
            emit this->shared_core_data_->sig_error_message(
                "send voff add failed", 2000);
        } else {
            emit this->shared_core_data_->sig_info_message(
                "send voff add success", 2000);
        }
    });

    connect(ui->pb_voff_dec_0, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSettingTestVOffset>(
            move::MotionCommandSettingTestVOffset::VOffsetSetCmd{
                0,
                move::MotionCommandSettingTestVOffset::VOffsetSetCmdType::
                    IncValue,
                -util::UnitConverter::mm2blu(0.01)});

        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
        bool ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 200);

        if (!ret) {
            QMessageBox::critical(this, "send voff dec failed",
                                  QString("send voff dec failed"));
            emit this->shared_core_data_->sig_error_message(
                "send voff dec failed", 2000);
        } else {
            emit this->shared_core_data_->sig_info_message(
                "send voff dec success", 2000);
        }
    });

    connect(ui->pb_voff_clear_0, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSettingTestVOffset>(
            move::MotionCommandSettingTestVOffset::VOffsetSetCmd{
                0,
                move::MotionCommandSettingTestVOffset::VOffsetSetCmdType::
                    SetValue,
                0});
        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
        bool ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 200);

        if (!ret) {
            QMessageBox::critical(this, "send voff clear failed",
                                  QString("send voff clear failed"));
            emit this->shared_core_data_->sig_error_message(
                "send voff clear failed", 2000);
        } else {
            emit this->shared_core_data_->sig_info_message(
                "send voff clear success", 2000);
        }
    });

    connect(ui->pb_voff_set_value_0, &QPushButton::clicked, this, [this]() {
        auto value = ui->dsb_set_voff_value_0->value();
        auto cmd = std::make_shared<move::MotionCommandSettingTestVOffset>(
            move::MotionCommandSettingTestVOffset::VOffsetSetCmd{
                0,
                move::MotionCommandSettingTestVOffset::VOffsetSetCmdType::
                    SetValue,
                util::UnitConverter::mm2blu(value)});

        shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
        bool ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 200);

        if (!ret) {
            QMessageBox::critical(this, "send voff set value failed",
                                  QString("send voff set value failed"));
            emit this->shared_core_data_->sig_error_message(
                "send voff set value failed", 2000);
        } else {
            emit this->shared_core_data_->sig_info_message(
                "send voff set value success", 2000);
        }
    });

    connect(
        ui->pb_voff_forced_zero_0, &QPushButton::clicked, this,
        [this](bool checked) {
            auto cmd = std::make_shared<move::MotionCommandSettingTestVOffset>(
                move::MotionCommandSettingTestVOffset::VOffsetSetCmd{
                    0,
                    move::MotionCommandSettingTestVOffset::VOffsetSetCmdType::
                        ForcedZero,
                    checked ? 1.0 : 0.0}); // 强制为0

            shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
            bool ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 200);

            if (!ret) {
                QMessageBox::critical(this, "send voff forced zero failed",
                                      QString("send voff forced zero failed"));
                emit this->shared_core_data_->sig_error_message(
                    "send voff forced zero failed", 2000);
            } else {
                emit this->shared_core_data_->sig_info_message(
                    "send voff forced zero success", 2000);
            }
        });
#endif
}

void SystemSettingPanel::_init_voffset_page_infosignals() {
    connect(shared_core_data_->get_info_dispatcher(),
            &InfoDispatcher::info_updated, this,
            [this](const move::MotionInfo &info) {
                if (!this->isVisible()) {
                    return;
                }

                const auto &v_offsets = info.curr_v_offsets_blu;
                ui->lb_current_voff_0->setText(
                    InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                        util::UnitConverter::blu2mm(v_offsets[0])));
                ui->lb_current_voff_1->setText(
                    InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                        util::UnitConverter::blu2mm(v_offsets[1])));
                ui->lb_current_voff_2->setText(
                    InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                        util::UnitConverter::blu2mm(v_offsets[2])));
                ui->lb_current_voff_3->setText(
                    InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                        util::UnitConverter::blu2mm(v_offsets[3])));
                ui->lb_current_voff_4->setText(
                    InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                        util::UnitConverter::blu2mm(v_offsets[4])));
                ui->lb_current_voff_5->setText(
                    InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                        util::UnitConverter::blu2mm(v_offsets[5])));
            });
}

void SystemSettingPanel::_init_button_cb() {
    connect(ui->pb_update, &QPushButton::clicked, this,
            [this]() { _update_ui(); });

    connect(ui->pb_save, &QPushButton::clicked, this, [this]() { _do_save(); });

    connect(ui->pb_enable_g01_half_closed_loop, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
    connect(ui->pb_enable_g01_run_each_servo_cmd, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
    connect(ui->pb_enable_g01_servo_dynamic_strategy, &QPushButton::clicked,
            this, [this](bool checked [[maybe_unused]]) { _do_save(); });
    // connect(ui->sb_g01_servo_dynamic_strategy_type,
    //         QOverload<int>::of(&QSpinBox::valueChanged), this,
    //         [this](int value [[maybe_unused]]) { _do_save(); });
    // 这里value changed信号会在updateui时触发，产生bug

    connect(ui->pb_enable_auto_opump, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
    connect(ui->pb_enable_auto_ipump, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
    connect(ui->pb_enable_auto_recorddata, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
}

void SystemSettingPanel::_do_save() {
    auto ret = _save();
    if (!ret) {
        QMessageBox::critical(this, "save failed", "save to file failed");
    } else {
        emit shared_core_data_->sig_info_message("Save System Settings Success",
                                                 5000);
    }

    _update_ui();
}

void SystemSettingPanel::_update_ui() {
    ui->sb_pb_maxacc->setValue(s_sys_setting.get_fmparam_max_acc_um_s2());
    ui->sb_pb_nacc->setValue(s_sys_setting.get_fmparam_nacc_ms());
    ui->sb_pb_mfr0->setValue(s_sys_setting.get_fmparam_speed_0_um_s());
    ui->sb_pb_mfr1->setValue(s_sys_setting.get_fmparam_speed_1_um_s());
    ui->sb_pb_mfr2->setValue(s_sys_setting.get_fmparam_speed_2_um_s());
    ui->sb_pb_mfr3->setValue(s_sys_setting.get_fmparam_speed_3_um_s());

    ui->sb_jump_maxacc->setValue(s_sys_setting.get_jump_param().max_acc_um_s2);
    ui->sb_jump_nacc->setValue(s_sys_setting.get_jump_param().nacc_ms);
    ui->sb_jump_buffer->setValue(s_sys_setting.get_jump_param().buffer_um);

    // Motion Settings
    const auto &motion_settings = s_sys_setting.get_motion_settings();
    ui->pb_enable_g01_run_each_servo_cmd->setChecked(
        motion_settings.enable_g01_run_each_servo_cmd);
    ui->pb_enable_g01_half_closed_loop->setChecked(
        motion_settings.enable_g01_half_closed_loop);
    ui->pb_enable_g01_servo_dynamic_strategy->setChecked(
        motion_settings.enable_g01_servo_with_dynamic_strategy);
    ui->sb_g01_servo_dynamic_strategy_type->setValue(
        motion_settings.g01_servo_dynamic_strategy_type);

    // adc
    ui->dsb_adc_gain->setValue(s_sys_setting.get_zynq_adc_settings().adc_gain);
    ui->dsb_adc_offset->setValue(
        s_sys_setting.get_zynq_adc_settings().adc_offset);
    ui->sb_adc_filter_time_us->setValue(
        s_sys_setting.get_zynq_adc_settings().voltage_filter_window_time_us);

    //#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // drill
    const auto &drill_settings = s_sys_setting.get_drill_settings();
    ui->dsb_drill_touch_return_length->setValue(drill_settings.touch_return_um);
    ui->dsb_drill_touch_speed->setValue(drill_settings.touch_speed_um_ms);

    ui->pb_enable_auto_opump->setChecked(drill_settings.auto_switch_opump);
    ui->pb_enable_auto_ipump->setChecked(drill_settings.auto_switch_ipump);
    ui->pb_enable_auto_recorddata->setChecked(drill_settings.auto_record_data);

    const auto &bo_settings = drill_settings.breakout_params;
    ui->sb_drill_voltage_average_window->setValue(
        bo_settings.voltage_average_filter_window_size);
    ui->sb_drill_kn_calc_window->setValue(
        bo_settings.stderr_filter_window_size);
    ui->dsb_drill_kn_valid_threshold->setValue(bo_settings.kn_valid_threshold);
    ui->sb_drill_kn_sc_window->setValue(bo_settings.kn_sc_window_size);
    ui->dsb_drill_kn_valid_rate_threshold->setValue(
        bo_settings.kn_valid_rate_threshold);
    ui->sb_drill_kn_cnt_threshold->setValue(
        bo_settings.kn_valid_rate_ok_cnt_threshold);
    ui->sb_drill_kc_cnt_maximum->setValue(
        bo_settings.kn_valid_rate_ok_cnt_maximum);

    ui->dsb_drill_max_move_length_after_breakout_start_detected->setValue(
        bo_settings.max_move_um_after_breakout_start_detected);
    ui->dsb_drill_breakout_detect_enable_percent->setValue(
        bo_settings.breakout_start_detect_length_percent);
    ui->dsb_drill_speed_rate_after_breakout_start_detected->setValue(
        bo_settings.speed_rate_after_breakout_start_detected);
    ui->sb_drill_wait_time_ms_after_breakout_end_judged->setValue(
        bo_settings.wait_time_ms_after_breakout_end_judged);

    ui->sb_drill_ctrl_flags->setValue(bo_settings.ctrl_flags);
    //#endif

    // axis params
    const auto &axis_params = s_sys_setting.get_axis_params().axis_params_vec;

    ui->sb_maxspeed_0->setValue(axis_params[0].max_speed_um_s);
    ui->sb_maxspeed_1->setValue(axis_params[1].max_speed_um_s);
    ui->sb_maxspeed_2->setValue(axis_params[2].max_speed_um_s);
    ui->sb_maxspeed_3->setValue(axis_params[3].max_speed_um_s);
    ui->sb_maxspeed_4->setValue(axis_params[4].max_speed_um_s);
    ui->sb_maxspeed_5->setValue(axis_params[5].max_speed_um_s);

    ui->sb_maxacc_0->setValue(axis_params[0].max_acc_um_s2);
    ui->sb_maxacc_1->setValue(axis_params[1].max_acc_um_s2);
    ui->sb_maxacc_2->setValue(axis_params[2].max_acc_um_s2);
    ui->sb_maxacc_3->setValue(axis_params[3].max_acc_um_s2);
    ui->sb_maxacc_4->setValue(axis_params[4].max_acc_um_s2);
    ui->sb_maxacc_5->setValue(axis_params[5].max_acc_um_s2);
}

bool SystemSettingPanel::_save() {
    s_sys_setting.set_fastmove_max_acc_um_s2(ui->sb_pb_maxacc->value());
    s_sys_setting.set_fastmove_nacc_ms(ui->sb_pb_nacc->value());
    s_sys_setting.set_fastmove_speed_0_um_s(ui->sb_pb_mfr0->value());
    s_sys_setting.set_fastmove_speed_1_um_s(ui->sb_pb_mfr1->value());
    s_sys_setting.set_fastmove_speed_2_um_s(ui->sb_pb_mfr2->value());
    s_sys_setting.set_fastmove_speed_3_um_s(ui->sb_pb_mfr3->value());

    s_sys_setting.set_jumpparam_max_acc_um_s2(ui->sb_jump_maxacc->value());
    s_sys_setting.set_jumpparam_nacc_ms(ui->sb_jump_nacc->value());
    s_sys_setting.set_jumpparam_buffer_um(ui->sb_jump_buffer->value());

    // Motion Settings (set s_sys_setting and set to motion thread)
    s_sys_setting.get_motion_settings().enable_g01_run_each_servo_cmd =
        ui->pb_enable_g01_run_each_servo_cmd->isChecked();
    s_sys_setting.get_motion_settings().enable_g01_half_closed_loop =
        ui->pb_enable_g01_half_closed_loop->isChecked();
    s_sys_setting.get_motion_settings().enable_g01_servo_with_dynamic_strategy =
        ui->pb_enable_g01_servo_dynamic_strategy->isChecked();
    s_sys_setting.get_motion_settings().g01_servo_dynamic_strategy_type =
        ui->sb_g01_servo_dynamic_strategy_type->value();

    struct _sys::_zynq_adc_settings adc_s;
    adc_s.adc_offset = ui->dsb_adc_offset->value();
    adc_s.adc_gain = ui->dsb_adc_gain->value();
    adc_s.voltage_filter_window_time_us =
        (uint32_t)(ui->sb_adc_filter_time_us->value());

    s_sys_setting.set_zynq_adc_settings(adc_s);

    _set_adc_settings_to_zynq();

    //#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // drill
    struct _sys::_drill_settings drill_settings;

    drill_settings.touch_return_um = ui->dsb_drill_touch_return_length->value();
    drill_settings.touch_speed_um_ms = ui->dsb_drill_touch_speed->value();

    drill_settings.auto_switch_opump = ui->pb_enable_auto_opump->isChecked();
    drill_settings.auto_switch_ipump = ui->pb_enable_auto_ipump->isChecked();
    drill_settings.auto_record_data =
        ui->pb_enable_auto_recorddata->isChecked();

    struct _sys::_breakout_settings bo_settings;
    bo_settings.voltage_average_filter_window_size =
        ui->sb_drill_voltage_average_window->value();
    bo_settings.stderr_filter_window_size =
        ui->sb_drill_kn_calc_window->value();
    bo_settings.kn_valid_threshold = ui->dsb_drill_kn_valid_threshold->value();
    bo_settings.kn_sc_window_size = ui->sb_drill_kn_sc_window->value();
    bo_settings.kn_valid_rate_threshold =
        ui->dsb_drill_kn_valid_rate_threshold->value();
    bo_settings.kn_valid_rate_ok_cnt_threshold =
        ui->sb_drill_kn_cnt_threshold->value();
    bo_settings.kn_valid_rate_ok_cnt_maximum =
        ui->sb_drill_kc_cnt_maximum->value();

    bo_settings.max_move_um_after_breakout_start_detected =
        ui->dsb_drill_max_move_length_after_breakout_start_detected->value();
    bo_settings.breakout_start_detect_length_percent =
        ui->dsb_drill_breakout_detect_enable_percent->value();
    bo_settings.speed_rate_after_breakout_start_detected =
        ui->dsb_drill_speed_rate_after_breakout_start_detected->value();
    bo_settings.wait_time_ms_after_breakout_end_judged =
        ui->sb_drill_wait_time_ms_after_breakout_end_judged->value();

    bo_settings.ctrl_flags = ui->sb_drill_ctrl_flags->value();

    drill_settings.breakout_params = bo_settings;

    s_sys_setting.set_drill_settings(drill_settings);

    auto drill_set_ret = _set_drill_settings_to_motion_thread();
    if (!drill_set_ret) {
        emit shared_core_data_->sig_error_message(
            QString{"Set Drill Settings Failed!"});
    }
    //#endif

    // axis params
    auto &axis_params = s_sys_setting.get_axis_params().axis_params_vec;
    axis_params.at(0).max_speed_um_s = ui->sb_maxspeed_0->value();
    axis_params.at(1).max_speed_um_s = ui->sb_maxspeed_1->value();
    axis_params.at(2).max_speed_um_s = ui->sb_maxspeed_2->value();
    axis_params.at(3).max_speed_um_s = ui->sb_maxspeed_3->value();
    axis_params.at(4).max_speed_um_s = ui->sb_maxspeed_4->value();
    axis_params.at(5).max_speed_um_s = ui->sb_maxspeed_5->value();

    axis_params.at(0).max_acc_um_s2 = ui->sb_maxacc_0->value();
    axis_params.at(1).max_acc_um_s2 = ui->sb_maxacc_1->value();
    axis_params.at(2).max_acc_um_s2 = ui->sb_maxacc_2->value();
    axis_params.at(3).max_acc_um_s2 = ui->sb_maxacc_3->value();
    axis_params.at(4).max_acc_um_s2 = ui->sb_maxacc_4->value();
    axis_params.at(5).max_acc_um_s2 = ui->sb_maxacc_5->value();

    auto set_ret = _set_motion_settings_to_motion_thread();
    if (!set_ret) {
        emit shared_core_data_->sig_error_message(
            QString{"Set Motion Settings Failed!"});
    }

    auto save_ret = s_sys_setting.save_to_file();
    if (!save_ret) {
        s_logger->error("save to file failed");
        s_sys_setting.reload_from_file();
    }

    return save_ret;
}

//#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
bool SystemSettingPanel::_set_drill_settings_to_motion_thread() {
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    auto mcq = shared_core_data_->get_motion_cmd_queue();

    move::DrillParams drill_params;

    const auto &sys_drill_settings = s_sys_setting.get_drill_settings();

    drill_params.touch_return_um = sys_drill_settings.touch_return_um;
    drill_params.touch_speed_um_ms = sys_drill_settings.touch_speed_um_ms;

    drill_params.breakout_params.voltage_average_filter_window_size =
        sys_drill_settings.breakout_params.voltage_average_filter_window_size;
    drill_params.breakout_params.stderr_filter_window_size =
        sys_drill_settings.breakout_params.stderr_filter_window_size;

    drill_params.breakout_params.kn_valid_threshold =
        sys_drill_settings.breakout_params.kn_valid_threshold;
    drill_params.breakout_params.kn_sc_window_size =
        sys_drill_settings.breakout_params.kn_sc_window_size;
    drill_params.breakout_params.kn_valid_rate_threshold =
        sys_drill_settings.breakout_params.kn_valid_rate_threshold;
    drill_params.breakout_params.kn_valid_rate_ok_cnt_threshold =
        sys_drill_settings.breakout_params.kn_valid_rate_ok_cnt_threshold;
    drill_params.breakout_params.kn_valid_rate_ok_cnt_maximum =
        sys_drill_settings.breakout_params.kn_valid_rate_ok_cnt_maximum;

    drill_params.breakout_params.max_move_um_after_breakout_start_detected =
        sys_drill_settings.breakout_params
            .max_move_um_after_breakout_start_detected;
    drill_params.breakout_params.breakout_start_detect_length_percent =
        sys_drill_settings.breakout_params.breakout_start_detect_length_percent;
    drill_params.breakout_params.speed_rate_after_breakout_start_detected =
        sys_drill_settings.breakout_params
            .speed_rate_after_breakout_start_detected;
    drill_params.breakout_params.wait_time_ms_after_breakout_end_judged =
        sys_drill_settings.breakout_params
            .wait_time_ms_after_breakout_end_judged;

    drill_params.breakout_params.ctrl_flags =
        sys_drill_settings.breakout_params.ctrl_flags;

    drill_params.auto_switch_opump = sys_drill_settings.auto_switch_opump;
    drill_params.auto_switch_ipump = sys_drill_settings.auto_switch_ipump;

    auto drill_settings_cmd =
        std::make_shared<move::MotionCommandSetDrillParams>(drill_params);

    mcq->push_command(drill_settings_cmd);

    return task::TaskHelper::WaitforCmdTobeAccepted(drill_settings_cmd, 200);
#else
    return true;
#endif
}
//#endif

bool SystemSettingPanel::_set_motion_settings_to_motion_thread() {
    auto mcq = shared_core_data_->get_motion_cmd_queue();

    const auto &sys_motion_settings = s_sys_setting.get_motion_settings();

    move::MotionSettings motion_settings;
    motion_settings.enable_g01_half_closed_loop =
        sys_motion_settings.enable_g01_half_closed_loop;
    motion_settings.enable_g01_run_each_servo_cmd =
        sys_motion_settings.enable_g01_run_each_servo_cmd;

    motion_settings.enable_g01_servo_with_dynamic_strategy =
        sys_motion_settings.enable_g01_servo_with_dynamic_strategy;
    motion_settings.g01_servo_dynamic_strategy_type =
        sys_motion_settings.g01_servo_dynamic_strategy_type;

    auto motion_settings_cmd =
        std::make_shared<move::MotionCommandSettingMotionSettings>(
            motion_settings);

    mcq->push_command(motion_settings_cmd);

    return task::TaskHelper::WaitforCmdTobeAccepted(motion_settings_cmd, 200);
}

void SystemSettingPanel::_set_adc_settings_to_zynq() {
    zynq::upper_adc_settings_t adc_settings;

    const auto &sys_adc_settings = s_sys_setting.get_zynq_adc_settings();

    adc_settings.adc_gain_times_1000 =
        (int32_t)round(sys_adc_settings.adc_gain * 1000.0);
    adc_settings.adc_offset_times_1000 =
        (int32_t)round(sys_adc_settings.adc_offset * 1000.0);
    adc_settings.voltage_filter_window_time_us =
        sys_adc_settings.voltage_filter_window_time_us;

    auto pkg = zynq::ZynqConnectController::MakeTcpPackage(
        zynq::COMM_FRAME_SET_ADC_SETTINGS, adc_settings);

    shared_core_data_->get_zynq_connect_ctrler()->send_tcp_bytearray(pkg);
}

} // namespace app
} // namespace edm
