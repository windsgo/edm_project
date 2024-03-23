#include "TaskManager.h"
#include "Logger/LogMacro.h"

#include <QCoreApplication>
#include <QDateTime>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace task {

TaskManager::TaskManager(edm::app::SharedCoreData *shared_core_data,
                         QObject *parent)
    : QObject(parent), shared_core_data_(shared_core_data),
      info_dispatcher_(shared_core_data->get_info_dispatcher()) {

    gcode_runner_ = new GCodeRunner(shared_core_data, this);

    _init_state();
    _init_connections();
}

bool TaskManager::operation_start_pointmove(
    std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd) {
    if (gcode_runner_->is_over()) {
        return _cmd_start_pointmove(cmd);
    } else {
        // GCode Runner Working
        if (gcode_runner_->state() != GCodeRunner::Paused) {
            s_logger->error(
                "TaskManager: start pm failed, GCodeRunner not paused");
            return false;
        }

        // 处于auto暂停情况
        return _cmd_start_pointmove(cmd);
    }
}

bool TaskManager::operation_stop_pointmove() { return _cmd_stop_pointmove(); }

bool TaskManager::operation_gcode_start(
    const std::vector<GCodeTaskBase::ptr> &gcode_list) {
    return gcode_runner_->start(gcode_list);
}

bool TaskManager::operation_gcode_pause() { return gcode_runner_->pause(); }

bool TaskManager::operation_gcode_resume() { return gcode_runner_->resume(); }

bool TaskManager::operation_gcode_stop() { return gcode_runner_->stop(); }

bool TaskManager::operation_emergency_stop() { return gcode_runner_->estop(); }

void TaskManager::_init_state() { gcode_runner_->reset(); }

void TaskManager::_init_connections() {
    // info直接点动信号
    connect(info_dispatcher_, &InfoDispatcher::sig_manual_pointmove_started,
            this, &TaskManager::sig_manual_pointmove_started);
    connect(info_dispatcher_, &InfoDispatcher::sig_manual_pointmove_stopped,
            this, &TaskManager::sig_manual_pointmove_stopped);

    // 信号转发
    connect(gcode_runner_, &GCodeRunner::sig_auto_started, this,
            &TaskManager::sig_auto_started);
    connect(gcode_runner_, &GCodeRunner::sig_auto_paused, this,
            &TaskManager::sig_auto_paused);
    connect(gcode_runner_, &GCodeRunner::sig_auto_resumed, this,
            &TaskManager::sig_auto_resumed);
    connect(gcode_runner_, &GCodeRunner::sig_auto_stopped, this,
            &TaskManager::sig_auto_stopped);
    connect(gcode_runner_, &GCodeRunner::sig_autogcode_switched_to_line, this,
            &TaskManager::sig_autogcode_switched_to_line);
    connect(gcode_runner_, &GCodeRunner::sig_switch_coordindex, this,
            &TaskManager::sig_switch_coordindex);
}

bool TaskManager::_cmd_start_pointmove(
    std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd) {
    this->shared_core_data_->get_motion_cmd_queue()->push_command(cmd);

    return TaskHelper::WaitforCmdTobeAccepted(cmd, 200);
}

bool TaskManager::_cmd_stop_pointmove() {
    // create motion cmd
    auto stop_pointmove_command =
        std::make_shared<edm::move::MotionCommandManualStopPointMove>(false);
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        stop_pointmove_command);

    return TaskHelper::WaitforCmdTobeAccepted(stop_pointmove_command, 200);
}

} // namespace task

} // namespace edm
