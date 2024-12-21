#pragma once

#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"
#include "Motion/Trajectory/TrajectoryList.h"
#include "MotionAutoTask.h"

#include "Utils/UnitConverter/UnitConverter.h"

namespace edm {
namespace move {

class G01GroupAutoTask : public AutoTask {
public:
    G01GroupAutoTask(const G01GroupStartParam &start_param,
                     const MotionCallbacks &cbs);

    bool pause() override;
    bool resume() override;
    bool stop(bool immediate = false) override;

    bool is_normal_running() const override;
    bool is_pausing() const override;
    bool is_paused() const override;
    bool is_resuming() const override;
    bool is_stopping() const override;
    bool is_stopped() const override;
    bool is_over() const override;

    void run_once() override;

public:
    enum class State {
        NormalRunning, // 正常运行
        Pausing, // 
        Paused,
        Resuming, // 如果有暂停回到加工起点, 这里就是恢复加工位置
        Stopping, // 如果在抬刀, 等待抬刀完成
        Stopped
    };
    static const char *GetStateStr(State s);

private:
    void _state_changeto(State new_s);

private:
    void _state_normal_running();
    void _state_pausing();
    void _state_paused();
    void _state_resuming();
    void _state_stopping();
    void _state_stopped();

    void _mach_on(bool enable);

private:
    MotionCallbacks cbs_;
    G01GroupStartParam start_param_;

    TrajectoryList::ptr traj_list_;

    State state_{State::Stopped};
};

} // namespace move
} // namespace edm