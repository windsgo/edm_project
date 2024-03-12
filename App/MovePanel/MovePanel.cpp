#include "MovePanel.h"
#include "ui_MovePanel.h"

namespace edm {
namespace app {
    
MovePanel::MovePanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::MovePanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);
}

MovePanel::~MovePanel() { delete ui; }

} // namespace app
} // namespace edm
