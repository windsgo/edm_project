#pragma once

#include "AxisRecorder.h"
#include "Motion/MoveDefines.h"
#include "Motion/PointMoveHandler/PointMoveHandler.h"
#include "Motion/SignalBuffer/SignalBuffer.h"
#include "Motion/TouchDetectHandler/TouchDetectHandler.h"

namespace edm {

namespace move {

class PauseMoveController final {
public:
    enum class State {
        NotInited, // 需要初始化 (当前指令绝对坐标)
        AllowManualPointMove,
        ManualPointMoving,
        Recovering,
        RecoveringPausing,
        RecoveringPaused,
        RecorverOver,

        OutSideStopping,       // auto stop
        OutSideOrErrorStopped, // auto stopped 或 出错
    };

public:
    using ptr = std::shared_ptr<PauseMoveController>;
    PauseMoveController(TouchDetectHandler::ptr touch_detect_handler,
                        SignalBuffer::ptr signal_buffer);
    ~PauseMoveController() noexcept = default;

    void init();

    auto state() const { return state_; }

    //! 绝对式坐标输入!
    bool start_manual_pointmove(const MoveRuntimePlanSpeedInput &speed_param,
                                const axis_t &start_pos,
                                const axis_t &target_pos);
    bool stop_manual_pointmove(bool immediate = false);

    // auto operate
    //! 操作的是recover过程中的快速移动的暂停、继续
    bool activate_recover();
    bool pause_recover();
    bool resume_recover();

    bool is_before_recover() const {
        return state_ == State::AllowManualPointMove ||
               state_ == State::ManualPointMoving;
    }
    bool is_recover_activated() const {
        return is_recovering() || is_recover_paused() || is_recover_over();
    }
    bool is_recover_paused() const { return state_ == State::RecoveringPaused; }
    bool is_recovering() const { return state_ == State::Recovering; }

    bool is_recover_over() const { return state_ == State::RecorverOver; }

    //! need to terminate
    bool stop(bool immediate = false);
    bool is_stopped() const { return state_ == State::OutSideOrErrorStopped; }

    void run_once();

    // const auto &get_cmd_axis() const { return curr_cmd_axis_; }

private:
    void _manual_pointmoving();
    void _recovering();
    void _recovering_pausing();
    void _outside_stopping();

    static const char* _state_str(State state);
    void _switch_state_to(State new_state);

private:
    TouchDetectHandler::ptr touch_detect_handler_;
    PointMoveHandler pm_handler_;
    AxisRecordStack axis_recorder_;

    // axis_t curr_cmd_axis_;

    State state_{State::NotInited};

    SignalBuffer::ptr signal_buffer_;
};

} // namespace move

} // namespace edm
