#include "SystemSettingPanel.h"
#include "Motion/MotionThread/MotionCommand.h"
#include "QtDependComponents/ZynqConnection/TcpMessageDefine.h"
#include "QtDependComponents/ZynqConnection/ZynqConnectController.h"
#include "TaskManager/TaskHelper.h"
#include "ui_SystemSettingPanel.h"

#include "SystemSettings/SystemSettings.h"

#include <QMessageBox>
#include <memory>
#include <qpushbutton.h>

namespace edm {
namespace app {

static auto &s_sys_setting = SystemSettings::instance();

SystemSettingPanel::SystemSettingPanel(SharedCoreData *shared_core_data,
                                       QWidget *parent)
    : QWidget(parent), ui(new Ui::SystemSettingPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    _init_button_cb();
    _update_ui();
}

SystemSettingPanel::~SystemSettingPanel() { delete ui; }

void SystemSettingPanel::_init_button_cb() {
    connect(ui->pb_update, &QPushButton::clicked, this,
            [this]() { _update_ui(); });

    connect(ui->pb_save, &QPushButton::clicked, this, [this]() { _do_save(); });

    connect(ui->pb_enable_g01_half_closed_loop, &QPushButton::clicked, this,
            [this](bool checked [[maybe_unused]]) { _do_save(); });
    connect(ui->pb_enable_g01_run_each_servo_cmd, &QPushButton::clicked, this,
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

    adc_settings.adc_gain = (int16_t)(sys_adc_settings.adc_gain);
    adc_settings.adc_offset = (int16_t)(sys_adc_settings.adc_offset);
    adc_settings.voltage_filter_window_time_us =
        sys_adc_settings.voltage_filter_window_time_us;

    auto pkg = zynq::ZynqConnectController::MakeTcpPackage(
        zynq::COMM_FRAME_SET_ADC_SETTINGS, adc_settings);

    shared_core_data_->get_zynq_connect_ctrler()->send_tcp_bytearray(pkg);
}

} // namespace app
} // namespace edm
