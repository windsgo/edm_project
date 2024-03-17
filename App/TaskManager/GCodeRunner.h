#pragma once

#include "GCodeTask.h"
#include "GCodeTaskConverter.h"

#include "SharedCoreData/SharedCoreData.h"

#include <QObject>
#include <QTimer>

namespace edm
{

namespace task
{

class GCodeRunner final : public QObject {
    Q_OBJECT
public:
    GCodeRunner(app::SharedCoreData* shared_core_data, QObject* parent = nullptr);
    ~GCodeRunner() = default;

    auto state() const { return state_; }

    bool pause();
    bool resume();
    bool stop();

    void reset() { _reset_state(); }

private:
    bool _cmd_auto_pause();
    bool _cmd_auto_resume();
    bool _cmd_auto_stop();

private: 
    // 定时器触发, 或任意命令触发, 或任意信号到达也触发, 状态机完全由轮询状态完成, 不依赖信号
    void _run_once();

    // 直接向motion controler获取实时的info信息
    void _direct_update_info_from_motion_ctrler();

    // 重置状态
    void _reset_state();

public:
    enum State {
        Running,
        WaitingForPaused,
        Paused,
        WaitingForResumed,
        WaitingForStopped,
        Stopped
    };

private:
    State state_{State::Stopped};

    move::MotionInfo info_;

    QTimer* update_timer_;

    std::vector<GCodeTaskBase::ptr> gcode_list_;
    int curr_gcode_num_;
};

} // namespace task

} // namespace edm