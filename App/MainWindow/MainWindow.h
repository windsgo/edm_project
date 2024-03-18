#pragma once

#include <QWidget>

// Panels
#include "CoordPanel/CoordPanel.h"
#include "InfoPanel/InfoPanel.h"
#include "MovePanel/MovePanel.h"
#include "IOPanel/IOPanel.h"
#include "PowerPanel/PowerPanel.h"

#include "TaskManager/TaskManager.h"
#include "TaskManager/GCodeTaskConverter.h"
#include "TaskManager/GCodeTask.h"

#include "TaskManager/GCodeRunner.h"

#include "codeeditor/codeeditor.h"

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

    void _init_test_gcode_buttons();

private:
    Ui::MainWindow *ui;

    SharedCoreData* shared_core_data_;

    task::TaskManager* task_manager_;
    task::GCodeRunner* test_gcode_runner_;

    CoordPanel* coord_panel_;
    InfoPanel* info_panel_;
    MovePanel* move_panel_;
    IOPanel* io_panel_;
    PowerPanel* power_panel_;

    CodeEditor* test_codeeditor_;
};

} // namespace app
} // namespace edm
