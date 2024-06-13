#pragma once

#include <QMetaObject>
#include <QObject>
#include <QTimer>

#include <chrono>
#include <memory>
#include <mutex>

#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"

#include "SharedCoreData/SharedCoreData.h"

#include "GCodeTask.h"
#include "GCodeTaskBase.h"

#include "GCodeRunner.h"

namespace edm {

namespace task {

class TaskManager final : public QObject {
    Q_OBJECT
    // need to emit signals ...
public:
    TaskManager(app::SharedCoreData *shared_core_data,
                QObject *parent = nullptr);
    ~TaskManager() = default;

public: // GUI命令接口
    bool operation_start_pointmove(
        std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd);
    bool operation_stop_pointmove();

    bool
    operation_gcode_start(const std::vector<GCodeTaskBase::ptr> &gcode_list);
    bool operation_gcode_pause();
    bool operation_gcode_resume();
    bool operation_gcode_stop();

    bool operation_emergency_stop();

    // 重置
    void reset() { _init_state(); }

    const auto &autogcode_last_error_str() const { return gcode_runner_->last_error_str(); }

signals: // 提供给GUI界面的信号, 告知一些重要状态变化
    // 通知信号 (整个GCodeList的运行信号)
    void sig_auto_started();
    void sig_auto_paused();
    void sig_auto_resumed();
    void sig_auto_stopped(bool err_occured);

    void sig_manual_pointmove_started();
    void sig_manual_pointmove_stopped();

    void sig_autogcode_switched_to_line(uint32_t line_num);

    void sig_switch_coordindex(uint32_t coord_index);
    
    void sig_coord_offset_changed();

private:
    void _init_state();
    void _init_connections();

private:
    bool _cmd_start_pointmove(
        std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd);
    bool _cmd_stop_pointmove();

private:
    app::SharedCoreData *shared_core_data_;

    GCodeRunner* gcode_runner_;

    InfoDispatcher *info_dispatcher_; // 用于获取缓存的info状态
};

} // namespace task

} // namespace edm