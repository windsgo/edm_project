#include "PowerPanel.h"
#include "ui_PowerPanel.h"

namespace edm {
namespace app {

PowerPanel::PowerPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::PowerPanel),
      shared_core_data_(shared_core_data),
      power_ctrler_(shared_core_data->get_power_ctrler()) {
    ui->setupUi(this);
    _init_button_slots();
}

PowerPanel::~PowerPanel() {
    delete ui;
}

void PowerPanel::_init_button_slots()
{
    connect(ui->pb_database, &QPushButton::clicked, this, [this]() {this->shared_core_data_->get_power_manager()->show_database_ui();});

    // TODO
}

} // namespace app
} // namespace edm
