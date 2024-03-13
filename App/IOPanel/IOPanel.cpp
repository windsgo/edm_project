#include "IOPanel.h"
#include "ui_IOPanel.h"

namespace edm {
namespace app {

IOPanel::IOPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::IOPanel), shared_core_data_(shared_core_data),
      io_ctrler_(shared_core_data->get_io_ctrler()) {
    ui->setupUi(this);
}

IOPanel::~IOPanel() { delete ui; }

} // namespace app
} // namespace edm
