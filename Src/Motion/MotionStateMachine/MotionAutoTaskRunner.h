#pragma once

#include <memory>

#include "MotionAutoTask.h"
#include "G00AutoTask.h"
#include "G01AutoTask.h"
#include "G04AutoTask.h"
#include "M00FakeAutoTask.h"

#include "Motion/SignalBuffer/SignalBuffer.h"

#include "Motion/PauseMoveController/PauseMoveController.h"
#include "Motion/TouchDetectHandler/TouchDetectHandler.h"

namespace edm
{

namespace move
{

// 作为中间层，维护一个autotask，一个暂停点动的控制类
// 并负责信号的发出，和向外层展示状态机
//! 出于简单化, 暂停点动虽然支持暂停,停止, 但是其暂停和继续不作为向外展示的状态
class AutoTaskRunner final {
public:
    using ptr = std::shared_ptr<AutoTaskRunner>;
    AutoTaskRunner(TouchDetectHandler::ptr touch_detect_handler, SignalBuffer::ptr signal_buffer);
    ~AutoTaskRunner() noexcept = default;

    // 重置状态机, 删除当前任务
    void reset(const axis_t& init_axis);

    // 重新启动一个任务 (无论任务是否结束, 如果未结束, 会打印警告)
    bool restart_task(AutoTask::ptr task);

    // 当前是否存在任务 (可能已经结束)
    bool has_task() const { return !!(curr_task_); }

    // 周期性执行接口, 执行, 并进行状态转换
    // 传入一个signal buffer, 用于信号输出
    void run_once();

    // auto运动操作命令
    bool pause();
    bool resume();
    bool stop(bool immediate = false);

    // 暂停点动时可以调用的手动点动接口, 其余时候调用均返回false且无效
    //! 绝对式坐标输入!
    bool start_manual_pointmove(const MoveRuntimePlanSpeedInput &speed_param,
                                const axis_t &start_pos, const axis_t &target_pos);
    bool stop_manual_pointmove(bool immediate = false);

    // 当前状态机状态
    auto state() const { return state_; }

    const auto& get_curr_cmd_axis() const { return curr_cmd_axis_; }

private:
    void _autostate_switch_to(MotionAutoState new_state);

    // 切换到暂停需要初始化 PauseMoveController, 特殊操作
    void _autostate_switch_to_paused();

    void _normal_moving();
    void _pausing();
    void _paused();
    void _resuming();
    void _stopping();

    void _dominated_state_pmrecovering_run_once();

private:
    // 主导状态, 描述当前的Auto状态描述的行为
    enum class DominatedState {
        //! 暂停点动的手动点动部分描述为 AutoTaskRunning中的 Paused状态
        //! PauseMoveRecoverRunning的Paused状态特指暂停恢复中
        AutoTaskRunning, // 正常AutoTask的执行
        PauseMoveRecoverRunning // 暂停点动(恢复过程)进行中 
        // PauseMoveRecoverRunning 中只应该存在的AutoState
        // 1. NormalRunning: 激活后(或被暂停后继续恢复), 正常恢复中 
        // 2. Pausing: 恢复中, 被暂停, 处于暂停中
        // 3. Paused: 恢复中被暂停, 已暂停
        // 5. Stopping: 恢复中被停止, 停止中, 停止完成后转移至AutoTaskRunning的Stopped
    };

    void _dominated_state_switch_to(DominatedState new_domin_state);
public:
    static const char* GetAutoStateStr(MotionAutoState state);
    static const char* GetDominStateStr(DominatedState domin_state);

private:
    AutoTask::ptr curr_task_ {nullptr};

    MotionAutoState state_ {MotionAutoState::Stopped};
    DominatedState domin_state_ {DominatedState::AutoTaskRunning};

    PauseMoveController::ptr pausemove_controller_;
    SignalBuffer::ptr signal_buffer_;

    axis_t curr_cmd_axis_;
};



} // namespace move

} // namespace edm
