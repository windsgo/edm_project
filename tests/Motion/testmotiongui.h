#ifndef TESTMOTIONGUI_H
#define TESTMOTIONGUI_H

#include <QWidget>

#include <QApplication>
#include <QDebug>

#include "GlobalCommandQueue/GlobalCommandQueue.h"
#include "Motion/MotionThread/MotionThreadController.h"

#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"

namespace Ui {
class TestMotionGui;
}

class TestMotionGui : public QWidget
{
    Q_OBJECT

public:
    explicit TestMotionGui(QWidget *parent = nullptr);

    ~TestMotionGui();

private:
    // 0
    void _init_system();

    // 0.1
    void _init_queues();
    // 0.2
    void _init_motion_controller();

    // 0.3
    void _init_button_slots();

    // 0.3.1
    void _init_ecat_button();
    // 0.3.2
    void _init_pm_buttons();

private: // motion cmds
    void _cmd_ecat_trigger_connect();

    void _cmd_start_pointmove(const edm::move::axis_t& target_pos);
    void _cmd_stop_pointmove(bool immediate = false);

private: // info dispatcher slot
    void _info_slot(const edm::move::MotionInfo& info);

private:
    Ui::TestMotionGui *ui;
    edm::global::GlobalCommandQueue::ptr global_command_queue_;

    edm::move::MotionCommandQueue::ptr motion_cmd_queue_;
    edm::move::MotionSignalQueue::ptr motion_signal_queue_;
    edm::move::MotionThreadController::ptr motion_controller_;

    edm::InfoDispatcher* info_dispatcher_ {nullptr};
};

#endif // TESTMOTIONGUI_H
