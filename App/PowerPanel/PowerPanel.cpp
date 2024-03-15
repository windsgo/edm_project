#include "PowerPanel.h"
#include "ui_PowerPanel.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

PowerPanel::PowerPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::PowerPanel),
      shared_core_data_(shared_core_data),
      pm_(shared_core_data->get_power_manager()) {
    ui->setupUi(this);

    update_io_timer_ = new QTimer(this);
    connect(update_io_timer_, &QTimer::timeout, this, &PowerPanel::slot_update_io_display);
    update_io_timer_->start(350);

    _init_update_slots();
    _init_button_slots();

    _update_eleparam_display(pm_->get_current_eleparam());
}

PowerPanel::~PowerPanel() { delete ui; }

void PowerPanel::slot_update_eleparam_display(const edm::power::EleParam_dkd_t &eleparam) {
    _update_io_display(); // io 也可能会变化, 也要刷新一下
    _update_eleparam_display(eleparam);
}

void PowerPanel::slot_update_io_display() {
    // 只刷新io, 连接到其他地方会设置machon这些东西的地方
    _update_io_display();
}

void PowerPanel::_update_io_display() {
    ui->pb_power_on->setChecked(pm_->is_power_on());
    ui->pb_mach_on->setChecked(pm_->is_highpower_on());
    ui->pb_mach_bit->setChecked(pm_->is_machbit_on());
}

void PowerPanel::_update_eleparam_display(
    const edm::power::EleParam_dkd_t &eleparam) {

#define XX_(sp_name__, ele_name__) \
    ui->spinBox_ele_##sp_name__->setValue(eleparam.ele_name__);

    XX_(on, pulse_on)
    XX_(off, pulse_off)
    XX_(up, up)
    XX_(dn, dn)
    XX_(ip, ip)
    XX_(hp, hp)
    XX_(ma, ma)
    XX_(sv, sv)
    XX_(al, al)
    XX_(ld, ld)
    XX_(oc, oc)
    XX_(pp, pp)
    XX_(s, servo_speed)
    ui->comboBox_ele_pl->setCurrentIndex(eleparam.pl); // pl = 0 -> index = 0 -> "+"
    XX_(c, c)
    XX_(js, jump_js)
    XX_(sensitivity, servo_sensitivity)
    XX_(voltage1, UpperThreshold)
    XX_(voltage2, LowerThreshold)
    XX_(index, upper_index)

#undef XX_    

}

void PowerPanel::_set_param_from_ui() {
    edm::power::EleParam_dkd_t param;

#define XX_(sp_name__, ele_name__) \
    param.ele_name__ = ui->spinBox_ele_##sp_name__->value();

    XX_(on, pulse_on)
    XX_(off, pulse_off)
    XX_(up, up)
    XX_(dn, dn)
    XX_(ip, ip)
    XX_(hp, hp)
    XX_(ma, ma)
    XX_(sv, sv)
    XX_(al, al)
    XX_(ld, ld)
    XX_(oc, oc)
    XX_(pp, pp)
    XX_(s, servo_speed)
    param.pl = ui->comboBox_ele_pl->currentIndex(); // pl = 0 -> index = 0 -> "+"
    XX_(c, c)
    XX_(js, jump_js)
    XX_(sensitivity, servo_sensitivity)
    XX_(voltage1, UpperThreshold)
    XX_(voltage2, LowerThreshold)
    XX_(index, upper_index)

#undef XX_

    pm_->set_current_eleparam(param);
}

void PowerPanel::_init_update_slots() {
    connect(pm_, &PowerManager::sig_power_flag_changed, this, &PowerPanel::slot_update_io_display);
    connect(pm_, &PowerManager::sig_ele_changed, this, &PowerPanel::slot_update_eleparam_display);
}

void PowerPanel::_init_button_slots() {
    connect(ui->pb_database, &QPushButton::clicked, this, [this]() {
        this->shared_core_data_->get_power_manager()->show_database_ui();
    });

    connect(ui->pb_mach_bit, &QPushButton::clicked, this, [this](bool checked) {
        pm_->set_machbit_on(checked);
    });

    connect(ui->pb_mach_on, &QPushButton::clicked, this, [this](bool checked) {
        pm_->set_highpower_on(checked);
    });

    connect(ui->pb_power_on, &QPushButton::clicked, this, [this](bool checked) {
        pm_->set_power_on(checked);
    });

    connect(ui->pb_set_param, &QPushButton::clicked, this, &PowerPanel::_set_param_from_ui);
    // TODO
}

} // namespace app
} // namespace edm
