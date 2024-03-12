#include "InfoPanel.h"
#include "ui_InfoPanel.h"

namespace edm {
namespace app {
InfoPanel::InfoPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::InfoPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    _init_info_slot();
}

InfoPanel::~InfoPanel() { delete ui; }

void InfoPanel::_init_info_slot() {
    connect(shared_core_data_->get_info_dispatcher(),
            &InfoDispatcher::info_updated, this, &InfoPanel::_update_info);
    
    // TODO signal connection
}

void InfoPanel::_update_info(const move::MotionInfo &info) {
    _do_update_ui_by_info(info);
}

void InfoPanel::_do_update_ui_by_info(const move::MotionInfo &info) {
    ui->pb_display_ecat_connected->setChecked(info.EcatConnected());
    ui->pb_display_ecat_enabled->setChecked(info.EcatConnected());

    ui->pb_display_touch_enabled->setChecked(info.TouchDetectEnabled());
    ui->pb_display_touch_detected->setChecked(info.TouchDetected());
    ui->pb_display_touch_warning->setChecked(info.TouchWarning());

    switch (info.main_mode) {
    case move::MotionMainMode::Idle:
        ui->pb_display_mainmode_idle->setChecked(true);
        break;
    case move::MotionMainMode::Manual:
        ui->pb_display_mainmode_manual->setChecked(true);
        break;
    case move::MotionMainMode::Auto:
        ui->pb_display_mainmode_auto->setChecked(true);
        break;
    }

    switch (info.auto_state)
    {
    case move::MotionAutoState::NormalMoving:
        ui->pb_display_autostate_running->setChecked(true);
        break;
    case move::MotionAutoState::Pausing:
        ui->pb_display_autostate_pausing->setChecked(true);
        break;
    case move::MotionAutoState::Paused:
        ui->pb_display_autostate_paused->setChecked(true);
        break;
    case move::MotionAutoState::Resuming:
        ui->pb_display_autostate_resuming->setChecked(true);
        break;
    case move::MotionAutoState::Stopping:
        ui->pb_display_autostate_stopping->setChecked(true);
        break;
    case move::MotionAutoState::Stopped:
        ui->pb_display_autostate_stopped->setChecked(true);
        break;
    default:
        break;
    }
}

} // namespace app
} // namespace edm