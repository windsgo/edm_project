#pragma once

#include <QObject>
#include <QMetaObject>
#include <QTimer>

#include <chrono>
#include <memory>
#include <mutex>

#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"

#include "SharedCoreData/SharedCoreData.h"

#include "GCodeTask.h"
#include "GCodeTaskBase.h"

namespace edm {

namespace task
{

class TaskManager final : public QObject {
    Q_OBJECT 
    // need to emit signals ...
public:
    using ptr = std::shared_ptr<TaskManager>;
    TaskManager(app::SharedCoreData* shared_core_data);
    ~TaskManager() = default;

public: // GUI命令接口
    bool operation_start_pointmove(std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd);
    bool operation_stop_pointmove();

    bool operation_gcode_start(const std::vector<GCodeTaskBase::ptr>& gcode_list);
    bool operation_gcode_pause();
    bool operation_gcode_resume();
    bool operation_gcode_stop();


signals: // 提供给GUI界面的信号, 告知一些重要状态变化
    // 通知信号
    void sig_auto_started();
    void sig_auto_paused();
    void sig_auto_resumed();
    void sig_auto_stopped();

    void sig_manual_pointmove_started();
    void sig_manual_pointmove_stopped();

    // 持续状态信号 (只有坐标刷新需要用), 也可以让GUI自己轮训
    void sig_update_axis(); // TODO 参数定义, 信号参数类型注册

private:
    void _init_state();

private:
    void _timer_slot();

    void _autogcode_state_run_once();

    // 开始前检查gcodelist的一些合法性: 电参数序号是否存在
    bool _check_gcode_list_at_first();

    void _autogcode_check_to_gcode(int next_gcode_num);

private:
    bool _is_info_mainmode_idle() const;
    bool _is_info_mainmode_auto() const;
    bool _is_info_autostate_paused() const;
    bool _is_info_autostate_stopped() const;

private:
    void _cmd_start_pointmove(std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd);

public:
    enum class State {
        Normal,
        AutoGCode,
    };

private:
    void _state_switchto(State new_state);

private: 
    app::SharedCoreData* shared_core_data_;

    InfoDispatcher* info_dispatcher_; // 用于获取缓存的info状态

    std::vector<GCodeTaskBase::ptr> gcode_list_;
    int curr_gcode_num_; // 当前正在运行的gcode在gcode_list中的下标 (保证 >=0 且 < size)
    bool gcodeauto_stop_flag_ {false};

    State state_ { State::Normal };

    QTimer* timer_; // 周期性运行计时器
};

} // namespace task

} // namespace edm