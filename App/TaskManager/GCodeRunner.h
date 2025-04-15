#pragma once

#include "GCodeTask.h"
#include "GCodeTaskConverter.h"

#include "TaskHelper.h"

#include "SharedCoreData/SharedCoreData.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include <QObject>
#include <QTimer>
#include <chrono>

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

    bool is_over() const { return state_ == State::Stopped; }

    bool start(const std::vector<GCodeTaskBase::ptr> &gcode_list);
    bool pause();
    bool resume();
    bool stop();

    bool estop();

    void reset() { _reset_state(); last_error_str_.clear(); }

    const auto& last_error_str() const { return last_error_str_; }

    // 用于显示
    int current_gcode_num() const { return curr_gcode_num_; }
    int total_gcode_num() const { return gcode_list_.size(); }

    auto current_node_elapsed_time() const {
        if (curr_gcode_num_ < gcode_list_.size()) {
            auto curr_gcode = gcode_list_[curr_gcode_num_];
            if (curr_gcode) {
                return curr_gcode->timer().elapsed_time();
            }
        }

        return std::chrono::system_clock::duration::zero();
    }

    auto total_elapsed_time() const {
        return total_elapsed_time_ + current_node_elapsed_time();
    }

    std::vector<std::string> get_time_report() const;

signals:
    void sig_auto_started();
    void sig_auto_paused();
    void sig_auto_resumed();
    void sig_auto_stopped(bool err_occured);

    void sig_autogcode_switched_to_line(uint32_t line_num);

    void sig_switch_coordindex(uint32_t coord_index);

    void sig_coord_offset_changed();

    void sig_record_data(int index, bool start);

private:
    bool _cmd_auto_pause();
    bool _cmd_auto_resume();
    bool _cmd_auto_stop();

    bool _cmd_emergency_stop();

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    void _drill_record_data(bool start);
#endif

private: 
    // 定时器触发, 或任意命令触发, 或任意信号到达也触发, 状态机完全由轮询状态完成, 不依赖信号
    void _run_once();

    bool _check_gcode_list_at_first();

    void _abort(std::string_view error_str);
    void _end(); // 正常结束

    void _check_to_next_gcode();

    // 重置状态
    void _reset_state();

    void _init_help_connections();

private:
    void _state_current_node_initing();
    void _state_running();
    void _state_running_non_motion(GCodeTaskBase::ptr curr_gcode);
    void _state_waiting_for_paused();
    void _state_waiting_for_resumed();
    void _state_waiting_for_stopped();

public:
    enum State {
        ReadyToStart,
        CurrentNodeIniting, // 当前node启动前准备, 不支持在此状态下暂停
        Running,
        WaitingForPaused,
        Paused,
        WaitingForResumed,
        WaitingForStopped,
        Stopped
    };

    static const char* GetStateStr(State s);

private:
    void _switch_to_state(State new_state);

private:
    State state_{State::Stopped};

    app::SharedCoreData *shared_core_data_;

    // 每次状态机运行一开始获取状态
    move::MotionInfo local_info_cache_;

    QTimer* update_timer_;
    const int update_timer_fast_peroid_ms_ = 100; // 快速周期
    const int update_timer_regular_peroid_ms_ = 1000; // 普通周期 

    std::vector<GCodeTaskBase::ptr> gcode_list_;
    int curr_gcode_num_{0};

    std::chrono::system_clock::duration total_elapsed_time_{0};

    std::string last_error_str_;

    // solve pause fail.
    bool delay_pause_flag_ {false}; // 延迟暂停(下一个状态暂停)

    // 记录时间
};

} // namespace task

} // namespace edm