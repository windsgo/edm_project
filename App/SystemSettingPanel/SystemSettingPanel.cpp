#include "SystemSettingPanel.h"
#include "ui_SystemSettingPanel.h"

#include <QMessageBox>

namespace edm {
namespace app {

static auto& s_sys_setting = SystemSettings::instance();

SystemSettingPanel::SystemSettingPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemSettingPanel)
{
    ui->setupUi(this);

    _init_button_cb();
    _update_ui();
}

SystemSettingPanel::~SystemSettingPanel()
{
    delete ui;
}

void SystemSettingPanel::_init_button_cb()
{
    connect(ui->pb_update, &QPushButton::clicked, this, [this]() {
        _update_ui();
    });

    connect(ui->pb_save, &QPushButton::clicked, this, [this]() {
        auto ret = _save();
        if (!ret) {
            QMessageBox::critical(this, "save failed", "save to file failed");
        }

        _update_ui();
    });
}

void SystemSettingPanel::_update_ui()
{
    ui->sb_pb_maxacc->setValue(s_sys_setting.get_fmparam_max_acc_um_s2());
    ui->sb_pb_nacc->setValue(s_sys_setting.get_fmparam_nacc_ms());
    ui->sb_pb_mfr0->setValue(s_sys_setting.get_fmparam_speed_0_um_s());
    ui->sb_pb_mfr1->setValue(s_sys_setting.get_fmparam_speed_1_um_s());
    ui->sb_pb_mfr2->setValue(s_sys_setting.get_fmparam_speed_2_um_s());
    ui->sb_pb_mfr3->setValue(s_sys_setting.get_fmparam_speed_3_um_s());

    ui->sb_jump_maxacc->setValue(s_sys_setting.get_jump_param().max_acc_um_s2);
    ui->sb_jump_nacc->setValue(s_sys_setting.get_jump_param().nacc_ms);
    ui->sb_jump_buffer->setValue(s_sys_setting.get_jump_param().buffer_um);
}

bool SystemSettingPanel::_save()
{
    s_sys_setting.set_fastmove_max_acc_um_s2(ui->sb_pb_maxacc->value());
    s_sys_setting.set_fastmove_nacc_ms(ui->sb_pb_nacc->value());
    s_sys_setting.set_fastmove_speed_0_um_s(ui->sb_pb_mfr0->value());
    s_sys_setting.set_fastmove_speed_1_um_s(ui->sb_pb_mfr1->value());
    s_sys_setting.set_fastmove_speed_2_um_s(ui->sb_pb_mfr2->value());
    s_sys_setting.set_fastmove_speed_3_um_s(ui->sb_pb_mfr3->value());

    s_sys_setting.set_jumpparam_max_acc_um_s2(ui->sb_jump_maxacc->value());
    s_sys_setting.set_jumpparam_nacc_ms(ui->sb_jump_nacc->value());
    s_sys_setting.set_jumpparam_buffer_um(ui->sb_jump_buffer->value());

    auto save_ret = s_sys_setting.save_to_file();
    if (!save_ret) {
        s_sys_setting.reload_from_file();
    }

    return save_ret;
}

} // namespace app
} // namespace edm
