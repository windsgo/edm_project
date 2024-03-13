#pragma once

#include <QWidget>

// Panels
#include "CoordPanel/CoordPanel.h"
#include "InfoPanel/InfoPanel.h"
#include "MovePanel/MovePanel.h"
#include "IOPanel/IOPanel.h"

// SharedData
#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class MainWindow;
}

namespace edm {
namespace app {

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    SharedCoreData* shared_core_data_;

    CoordPanel* coord_panel_;
    InfoPanel* info_panel_;
    MovePanel* move_panel_;
    IOPanel* io_panel_;
};

} // namespace app
} // namespace edm
