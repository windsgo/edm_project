#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QPalette>

// Panels
#include "CoordPanel/CoordPanel.h"
#include "InfoPanel/InfoPanel.h"
#include "MovePanel/MovePanel.h"
#include "IOPanel/IOPanel.h"
#include "PowerPanel/PowerPanel.h"
#include "GCodePanel/GCodePanel.h"
#include "TestPanel/TestPanel.h"
#include "CoordSettingPanel/CoordSettingPanel.h"
#include "SystemSettingPanel/SystemSettingPanel.h"

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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void _init_members();

    void _init_status_bar_palette_and_connection();

public:
    void slot_info_message(const QString& str, int timeout = 0);
    void slot_warn_message(const QString& str, int timeout = 0);
    void slot_error_message(const QString& str, int timeout = 0);

private:
    Ui::MainWindow *ui;

    SharedCoreData* shared_core_data_;

    task::TaskManager* task_manager_;

    CoordPanel* coord_panel_;
    InfoPanel* info_panel_;
    MovePanel* move_panel_;
    IOPanel* io_panel_;
    PowerPanel* power_panel_;
    GCodePanel* gcode_panel_;
    TestPanel* test_panel_;
    CoordSettingPanel* coord_setting_panel_;
    SystemSettingPanel* system_setting_panel_;

private:

    QTimer* test_latency_timer_;

private: // default palette of status bar, for message show
    QPalette status_bar_info_palette_;
    QPalette status_bar_warn_palette_;
    QPalette status_bar_error_palette_;
};

} // namespace app
} // namespace edm
