#include "CoordSetToGivenValueDialog.h"
#include "ui_CoordSetToGivenValueDialog.h"

namespace edm {
namespace app {

CoordSetToGivenValueDialog::CoordSetToGivenValueDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CoordSetToGivenValueDialog) {
    ui->setupUi(this);

#if (EDM_BLU_PER_UM == 10)
    ui->doubleSpinBox->setDecimals(4);
#elif (EDM_BLU_PER_UM == 1)
    ui->doubleSpinBox->setDecimals(3);
#else
#error "EDM_BLU_PER_UM not valid"
#endif
}

CoordSetToGivenValueDialog::~CoordSetToGivenValueDialog() { delete ui; }

move::unit_t CoordSetToGivenValueDialog::value() const {
    return ui->doubleSpinBox->value();
}

} // namespace app
} // namespace edm
