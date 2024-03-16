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

    const auto &last_error_str() const { return last_error_message_; }

signals: // 提供给GUI界面的信号, 告知一些重要状态变化
    // 通知信号 (整个GCodeList的运行信号)
    void sig_auto_started();
    void sig_auto_paused();
    void sig_auto_resumed();
    void sig_auto_stopped(bool err_occured);

    void sig_manual_pointmove_started();
    void sig_manual_pointmove_stopped();

    void sig_switch_coordindex(uint32_t coord_index);

private:
    void _init_state();
    void _init_connections();

private:
    void _autogcode_normal_end();
    void _autogcode_abort(std::string_view error_str);

    // 开始前检查gcodelist的一些合法性: 电参数序号是否存在
    bool _check_gcode_list_at_first();

    void _autogcode_init_curr_gcode();
    void _autogcode_check_to_next_gcode();

    bool _waitfor_cmd_tobe_accepted(move::MotionCommandBase::ptr cmd,
                                    int timeout_ms = 100);

private:
    bool _is_info_mainmode_idle() const;
    bool _is_info_mainmode_auto() const;
    bool _is_info_autostate_paused() const;
    bool _is_info_autostate_stopped() const;

    bool _check_posandneg_soft_limit(const move::axis_t &mach_target_pos) const;
    move::MoveRuntimePlanSpeedInput _get_default_speed_param() const;

private: // slots
    // 接收info的信号, 信号驱动
    void _slot_auto_stopped();
    void _slot_auto_started();
    void _slot_auto_paused();
    void _slot_auto_resumed();

private:
    void _cmd_start_pointmove(
        std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd);
    void _cmd_stop_pointmove();

    bool _cmd_auto_pause();
    bool _cmd_auto_resume();
    bool _cmd_auto_stop();

    bool _cmd_emergency_stop();

public:
    enum class State {
        Normal,
        AutoGCode,
    };

private:
    void _state_switchto(State new_state);

private:
    app::SharedCoreData *shared_core_data_;

    InfoDispatcher *info_dispatcher_; // 用于获取缓存的info状态

    std::vector<GCodeTaskBase::ptr> gcode_list_;
    int curr_gcode_num_; // 当前正在运行的gcode在gcode_list中的下标 (保证 >=0 且
                         // < size)
    bool gcodeauto_stop_flag_{false};

    State state_{State::Normal};

    std::string last_error_message_;
};

} // namespace task

} // namespace edm