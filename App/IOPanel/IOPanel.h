#pragma once

#include <QWidget>

#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class IOPanel;
}

namespace edm {
namespace app {

class IOPanel : public QWidget {
    Q_OBJECT

public:
    explicit IOPanel(SharedCoreData *shared_core_data,
                     QWidget *parent = nullptr);
    ~IOPanel();

private:
    Ui::IOPanel *ui;

    SharedCoreData *shared_core_data_;

    io::IOController::ptr io_ctrler_;
};

} // namespace app
} // namespace edm
