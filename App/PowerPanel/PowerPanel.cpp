#include "PowerPanel.h"
#include "ui_PowerPanel.h"

namespace edm {
namespace app {

PowerPanel::PowerPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::PowerPanel),
      shared_core_data_(shared_core_data),
      power_ctrler_(shared_core_data->get_power_ctrler()) {
    ui->setupUi(this);

    power_database_ = new PowerDatabase(shared_core_data);

    power_database_->hide(); // 隐藏
    _init_button_slots();
}

PowerPanel::~PowerPanel() {
    power_database_->hide();
    delete power_database_;
    delete ui;
}

void PowerPanel::_init_button_slots()
{
    connect(ui->pb_database, &QPushButton::clicked, this, [this]() {this->power_database_->show();});

    // TODO
}

} // namespace app
} // namespace edm
