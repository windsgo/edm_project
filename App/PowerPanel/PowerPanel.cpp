#include "PowerPanel.h"
#include "ui_PowerPanel.h"

namespace edm {
namespace app {

PowerPanel::PowerPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::PowerPanel),
      shared_core_data_(shared_core_data),
      power_ctrler_(shared_core_data->get_power_ctrler()) {
    ui->setupUi(this);
}

PowerPanel::~PowerPanel() { delete ui; }

} // namespace app
} // namespace edm
