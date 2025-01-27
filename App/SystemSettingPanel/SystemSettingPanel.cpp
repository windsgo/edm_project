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
}

SystemSettingPanel::~SystemSettingPanel() {
    adc_calc_panel_->hide();
    delete adc_calc_panel_;

    delete ui;
}

void SystemSettingPanel::_init_button_cb() {
    connect(ui->pb_update, &QPushButton::clicked, this,
            [this]() { _update_ui(); });

    connect(ui->pb_save, &QPushButton::clicked, this, [this]() { _do_save(); });

    connect(ui->pb_enable_g01_half_closed_loop, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
    connect(ui->pb_enable_g01_run_each_servo_cmd, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
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
    ui->pb_enable_g01_run_each_servo_cmd->setChecked(
        s_sys_setting.get_enable_g01_run_each_servo_cmd());
    ui->pb_enable_g01_half_closed_loop->setChecked(
        s_sys_setting.get_enable_g01_half_closed_loop());

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
    s_sys_setting.set_enable_g01_run_each_servo_cmd(
        ui->pb_enable_g01_run_each_servo_cmd->isChecked()); // TODO
    s_sys_setting.set_enable_g01_half_closed_loop(
        ui->pb_enable_g01_half_closed_loop->isChecked()); // TODO

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
    drill_settings.auto_record_data = ui->pb_enable_auto_recorddata->isChecked();

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

    auto set_ret = _set_motion_settings_to_motion_thread();
    if (!set_ret) {
        emit shared_core_data_->sig_error_message(
            QString{"Set Motion Settings Failed!"});
    }

    auto save_ret = s_sys_setting.save_to_file();
    if (!save_ret) {
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

    move::MotionSettings motion_settings;
    motion_settings.enable_g01_half_closed_loop =
        s_sys_setting.get_enable_g01_half_closed_loop();
    motion_settings.enable_g01_run_each_servo_cmd =
        s_sys_setting.get_enable_g01_run_each_servo_cmd();

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
