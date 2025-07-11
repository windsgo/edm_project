#pragma once

#include <QWidget>
#include <QMainWindow>
#include <QPalette>

// Panels
#include "CoordPanel/CoordPanel.h"
#include "InfoPanel/InfoPanel.h"
#include "Motion/MoveDefines.h"
#include "MovePanel/MovePanel.h"
#include "IOPanel/IOPanel.h"
#include "PowerPanel/PowerPanel.h"
#include "GCodePanel/GCodePanel.h"
#include "TestPanel/TestPanel.h"
#include "CoordSettingPanel/CoordSettingPanel.h"
#include "SystemSettingPanel/SystemSettingPanel.h"
#include "DataQueueRecordPanel/DataQueueRecordPanel.h"
#include "LogListPanel/LogListPanel.h"

#include "DataDisplayer/DataDisplayer.h"

#include "TaskManager/TaskManager.h"

// SharedData
#include "SharedCoreData/SharedCoreData.h"
#include "qwt_dial_needle.h"

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

private:
    void _init_members();
    void _init_tab_monitor();

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    void _init_tab_breakout_monitor();
#endif

    void _init_status_bar_palette_and_connection();

private:
    void _slot_monitor_timer_doit();
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    void _slot_breakout_monitor_timer_doit(const move::MotionInfo& info);
#endif

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
    DataQueueRecordPanel* dqr_panel_;
    LogListPanel* loglist_panel_;

    // DataDisplayer* test_data_displayer_;
    // int data_index0_;
    // int data_index1_;
    // QTimer *display_timer_;

private:

    QTimer* test_latency_timer_;

private: // default palette of status bar, for message show
    QPalette status_bar_info_palette_;
    QPalette status_bar_warn_palette_;
    QPalette status_bar_error_palette_;

private: // monitor 
    DataDisplayer* vol_cur_displayer_;
    int monitor_voltage_index_;
    int monitor_current_index_;
    DataDisplayer* mach_rate_displayer_;
    int monitor_normal_rate_index_;
    int monitor_short_rate_index_;
    int monitor_open_rate_index_;
    int sv_speed_index_;

    QwtDialSimpleNeedle* dial_needle_;

    QTimer* monitor_timer_;
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
private:
    DataDisplayer* bo_displayer_1_;
    DataDisplayer* bo_displayer_2_;

    int bo_v_index_;
    int bo_av_index_;

    int bo_kn_index_;
    int bo_knr_index_;
    int bo_kn_cnt_index_;
#endif
};

} // namespace app
} // namespace edm
