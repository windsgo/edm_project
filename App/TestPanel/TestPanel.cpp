#include "TestPanel.h"
#include "ui_TestPanel.h"

#include <QSlider>

namespace edm {
namespace app {

TestPanel::TestPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::TestPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    _update_phy_touchdetect();
    _update_servo();

    connect(
        ui->pb_phy_detected, &QPushButton::clicked, this,
        [this](bool checked [[maybe_unused]]) { _update_phy_touchdetect(); });

    connect(ui->horizontalSlider_servo, &QSlider::valueChanged, this,
            [this](int value [[maybe_unused]]) { _update_servo(); });
    connect(ui->horizontalSlider_servo_2, &QSlider::valueChanged, this,
            [this](int value [[maybe_unused]]) { _update_servo(); });
}

TestPanel::~TestPanel() { delete ui; }

void TestPanel::_update_phy_touchdetect() {
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    shared_core_data_->get_can_recv_buffer()->set_manual_touch_detect_flag(
        ui->pb_phy_detected->isChecked());
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT
}

void TestPanel::_update_servo() {
#ifdef EDM_OFFLINE_MANUAL_TOUCH_DETECT
    shared_core_data_->get_can_recv_buffer()->set_manual_servo_cmd(
        (double)ui->horizontalSlider_servo->value() / 100.0,
        util::UnitConverter::um_ms2blu_p(
            (double)ui->horizontalSlider_servo_2->value() / 100.0));
#endif // EDM_OFFLINE_MANUAL_TOUCH_DETECT
}

} // namespace app
} // namespace edm
