#pragma once

#include <QDialog>
#include <optional>

#include "InputHelper/InputHelper.h"
#include "Motion/MoveDefines.h"

namespace Ui {
class CoordSetToGivenValueDialog;
}

namespace edm {
namespace app {

class CoordSetToGivenValueDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoordSetToGivenValueDialog(QWidget *parent = nullptr);
    ~CoordSetToGivenValueDialog();

    move::unit_t value() const;

private:
    Ui::CoordSetToGivenValueDialog *ui;
};

} // namespace app
} // namespace edm
