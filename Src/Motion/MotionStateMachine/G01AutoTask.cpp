#include "G01AutoTask.h"

#include "Logger/LogMacro.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include <chrono>

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

// static bool s_g01_run_each_servo_cmd =
//     edm::SystemSettings::instance().get_enable_g01_run_each_servo_cmd();

#define MULTI_SEND_INTERVAL 30 // (ms)

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

G01AutoTask::G01AutoTask(
    TrajectoryLinearSegement::ptr line_traj, unit_t max_jump_height_from_begin,
    const std::function<void(bool)> &cb_enable_votalge_gate,
    const std::function<void(bool)> &cb_mach_on)
    : AutoTask(AutoTaskType::G01), line_traj_(line_traj),
      max_jump_height_from_begin_(max_jump_height_from_begin),
      cb_enable_votalge_gate_(cb_enable_votalge_gate), cb_mach_on_(cb_mach_on) {

    assert(line_traj_->at_start());

    // 开始时获取一次抬刀参数
    jumping_param_ = s_motion_shared->get_jump_param();

    s_logger->debug("dn ms: {}, buffer_blu: {}", jumping_param_.dn_ms,
                    jumping_param_.buffer_blu);

    // 设定一个"上一次抬刀结束时间" = "上一次开始放电的时间"
    // 用于根据DN值, 开始下一次抬刀
    last_jump_end_time_ms_ = GetCurrentTimeMs();

    // 使能电压gate
    cb_enable_votalge_gate_(true);
    last_send_enable_votalge_gate_time_ =
        std::chrono::high_resolution_clock::now();

    cb_mach_on_(true);
}

bool G01AutoTask::pause() {
    switch (state_) {
    case State::Stopping:
    case State::Stopped:
    case State::Resuming:
    default:
        return false;
    case State::Pausing:
    case State::Paused:
        return true;
    case State::NormalRunning: {
        _state_changeto(State::Pausing);
        _pauseorstop_substate_changeto(PauseOrStopSubState::WaitingForJumpEnd);
        return true;
    }
    }
}

bool G01AutoTask::resume() {
    switch (state_) {
    case State::Stopping:
    case State::Stopped:
    case State::Pausing:
    case State::NormalRunning:
    default:
        return false;
    case State::Resuming:
        return true;
    case State::Paused: {
        _state_changeto(State::Resuming);
        _resume_substate_changeto(ResumeSubState::RecoveringToLastMachingPos);
        return true;
    }
    }
}

bool G01AutoTask::stop(bool immediate [[maybe_unused]]) {
    // TODO 不处理 immediate

    switch (state_) {
    case State::Pausing: {
        _state_changeto(State::Stopping); // 沿用Pausing时Substate做的事情
        return true;
    }
    case State::Paused: {
        _state_changeto(State::Stopped);
        return true;
    }
    case State::Stopping:
    case State::Stopped:
        return true;
    case State::Resuming: {
        s_logger->warn("G01 Stopped When Resuming");
        // TODO 如果支持了暂停回原点, 这个地方要考虑速度大的时候Stop会不会振动
        _state_changeto(State::Stopped);
        return true;
    }
    case State::NormalRunning: {
        _state_changeto(State::Stopping);
        _pauseorstop_substate_changeto(PauseOrStopSubState::WaitingForJumpEnd);
        return true;
    }
    default:
        return false;
    }
}

bool G01AutoTask::is_normal_running() const {
    return state_ == State::NormalRunning;
}

bool G01AutoTask::is_pausing() const { return state_ == State::Pausing; }

bool G01AutoTask::is_paused() const { return state_ == State::Paused; }

bool G01AutoTask::is_resuming() const { return state_ == State::Resuming; }

bool G01AutoTask::is_stopping() const { return state_ == State::Stopping; }

bool G01AutoTask::is_stopped() const { return state_ == State::Stopped; }

bool G01AutoTask::is_over() const { return is_stopped(); }

void G01AutoTask::run_once() {
    switch (state_) {
    case State::NormalRunning:
        _state_normal_running();
        break;
    case State::Pausing:
        _state_pausing();
        break;
    case State::Paused:
        _state_paused();
        break;
    case State::Resuming:
        _state_resuming();
        break;
    case State::Stopping:
        _state_stopping();
        break;
    case State::Stopped:
        _state_stopped();
        break;
    default:
        break;
    }
}

const char *G01AutoTask::GetStateStr(State s) {
    switch (s) {
#define XX_(s__)     \
    case State::s__: \
        return #s__;
        XX_(NormalRunning)
        XX_(Pausing)
        XX_(Paused)
        XX_(Resuming)
        XX_(Stopping)
        XX_(Stopped)
#undef XX_
    default:
        return "Unknow";
    }
}

const char *G01AutoTask::GetServoSubStateStr(ServoSubState s) {
    switch (s) {
#define XX_(s__)             \
    case ServoSubState::s__: \
        return #s__;
        XX_(Servoing)
        XX_(JumpUping)
        XX_(JumpDowning)
        XX_(JumpDowningBuffer)
#undef XX_
    default:
        return "Unknow";
    }
}

const char *G01AutoTask::GetPauseOrStopSubStateStr(PauseOrStopSubState s) {
    switch (s) {
#define XX_(s__)                   \
    case PauseOrStopSubState::s__: \
        return #s__;
        XX_(WaitingForJumpEnd)
        XX_(BackingToBegin)
#undef XX_
    default:
        return "Unknow";
    }
}

const char *G01AutoTask::GetResumeSubStateStr(ResumeSubState s) {
    switch (s) {
#define XX_(s__)              \
    case ResumeSubState::s__: \
        return #s__;
        XX_(RecoveringToLastMachingPos)
#undef XX_
    default:
        return "Unknow";
    }
}

void G01AutoTask::_state_changeto(State new_s) {
    s_logger->trace("G01 State: {} -> {}", GetStateStr(state_),
                    GetStateStr(new_s));
    state_ = new_s;
}

void G01AutoTask::_servo_substate_changeto(ServoSubState new_s) {
    s_logger->trace("G01 ServoSubState: {} -> {}",
                    GetServoSubStateStr(servo_sub_state_),
                    GetServoSubStateStr(new_s));
    servo_sub_state_ = new_s;
}

void G01AutoTask::_pauseorstop_substate_changeto(PauseOrStopSubState new_s) {
    s_logger->trace("G01 PauseOrStopSubState: {} -> {}",
                    GetPauseOrStopSubStateStr(pause_or_stop_sub_state_),
                    GetPauseOrStopSubStateStr(new_s));
    pause_or_stop_sub_state_ = new_s;
}

void G01AutoTask::_resume_substate_changeto(ResumeSubState new_s) {
    s_logger->trace("G01 ResumeSubState: {} -> {}",
                    GetResumeSubStateStr(resume_sub_state_),
                    GetResumeSubStateStr(new_s));
    resume_sub_state_ = new_s;
}

void G01AutoTask::_state_normal_running() {
    switch (servo_sub_state_) {
    case ServoSubState::Servoing: {
        _servo_substate_servoing(); //! include jump start inside
        break;
    }
    case ServoSubState::JumpUping:
        _servo_substate_jumpuping();
        break;
    case ServoSubState::JumpDowning:
        _servo_substate_jumpdowning();
        break;
    case ServoSubState::JumpDowningBuffer:
        _servo_substate_jumpdowningbuffer();
        break;
    default:
        break;
    }
}

void G01AutoTask::_state_pausing() {
    _state_pausing_or_stopping(State::Paused);
}

void G01AutoTask::_state_paused() {
    // Do Nothing
}

void G01AutoTask::_state_resuming() {
    switch (resume_sub_state_) {
    case ResumeSubState::RecoveringToLastMachingPos:
        if (!back_to_begin_when_pause_) {
            // 重置抬刀计时
            last_jump_end_time_ms_ = GetCurrentTimeMs();
            cb_enable_votalge_gate_(true);
            last_send_enable_votalge_gate_time_ =
                std::chrono::high_resolution_clock::now();
            cb_mach_on_(true);
            _state_changeto(State::NormalRunning);
            assert(servo_sub_state_ == ServoSubState::Servoing);
            break;
        }

        // TODO Back To Begin Related Resume
        // Currently direct change to paused, do nothing
        assert(false); // should not be here
        last_jump_end_time_ms_ = GetCurrentTimeMs();
        cb_enable_votalge_gate_(true);
        last_send_enable_votalge_gate_time_ =
            std::chrono::high_resolution_clock::now();
        cb_mach_on_(true);
        _state_changeto(State::NormalRunning);
        break;
    default:
        break;
    }
}

void G01AutoTask::_state_stopping() {
    _state_pausing_or_stopping(State::Stopped);
}

void G01AutoTask::_state_stopped() {
    // Do Nothing
}

void G01AutoTask::_state_pausing_or_stopping(State target_state) {
    assert(target_state == State::Stopped || target_state == State::Paused);

    switch (pause_or_stop_sub_state_) {
    case PauseOrStopSubState::WaitingForJumpEnd: {
        // Check Servo Sub State, Wait for jump end (servo sub state return to
        // Servoing)
        switch (servo_sub_state_) {
        case ServoSubState::Servoing: {
            // if is servoing, then go to next pause and stop state
            _pauseorstop_substate_changeto(PauseOrStopSubState::BackingToBegin);

            cb_mach_on_(false);
            break;
        }
        case ServoSubState::JumpUping:
            _servo_substate_jumpuping();
            break;
        case ServoSubState::JumpDowning:
            _servo_substate_jumpdowning();
            break;
        case ServoSubState::JumpDowningBuffer:
            _servo_substate_jumpdowningbuffer();
            break;
        }

        break;
    }
    case PauseOrStopSubState::BackingToBegin: {
        assert(servo_sub_state_ == ServoSubState::Servoing);
        if (target_state == State::Stopped && !back_to_begin_when_stop_) {
            _state_changeto(State::Stopped);
            break;
        }
        if (target_state == State::Paused && !back_to_begin_when_pause_) {
            _state_changeto(State::Paused);
            break;
        }

        // TODO Back To Begin
        // Currently direct change to paused/stopped, do nothing
        assert(false); // should not be here
        _state_changeto(target_state);
        break;
    }
    default:
        break;
    }
}

void G01AutoTask::_servo_substate_servoing() {
    if (_servoing_check_and_plan_jump()) {
        // 抬刀规划成功, 已经转换抬刀状态, 不走伺服
        return;
    }

    // 抬刀规划失败, 继续正常走伺服
    auto servo_can_end = _servoing_do_servothings();
    if (servo_can_end) {
        _state_changeto(State::Stopping);
        return;
    }
}

void G01AutoTask::_servo_substate_jumpuping() {
    // static int _tmp = 0;
    // ++_tmp;
    this->jump_pm_handler_.run_once();
    // this->curr_cmd_axis_ = this->jump_pm_handler_.get_current_pos();
    s_motion_shared->set_global_cmd_axis(
        this->jump_pm_handler_.get_current_pos());

    // multi send
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = now - last_send_enable_votalge_gate_time_;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >
        MULTI_SEND_INTERVAL) {
        cb_enable_votalge_gate_(false);
        last_send_enable_votalge_gate_time_ = now;
        // s_logger->info("upping multi");
    }

    if (this->jump_pm_handler_.is_over()) {
        // UP走完
        // 计算dn的目标位置
        const auto &curr_maching_dir = this->line_traj_->get_unit_vector();
        const auto &down_start_pos = this->jump_up_target_pos_;
        axis_t down_target_pos{};
        unit_t down_length =
            (jumping_param_.up_blu - jumping_param_.buffer_blu);
        assert(down_length > 0);
        for (std::size_t i = 0; i < this->jump_up_target_pos_.size(); ++i) {
            down_target_pos[i] =
                (down_start_pos[i] + curr_maching_dir[i] * down_length);
        }

        //! assume this `start` will success ...
        this->jump_pm_handler_.start(jumping_param_.speed_param, down_start_pos,
                                     down_target_pos);

        // 置状态, 操作电压
        _servo_substate_changeto(ServoSubState::JumpDowning);
        cb_enable_votalge_gate_(true);
        last_send_enable_votalge_gate_time_ =
            std::chrono::high_resolution_clock::now();

        // s_logger->debug("jump up over: ltcurr-z: "
        //                 "{}, curr-z: {}, curr-length: {}, _tmp: {}",
        //                 this->line_traj_->curr_pos()[2],
        //                 this->curr_cmd_axis_[2], line_traj_->curr_length(),
        //                 _tmp);
        // _tmp = 0;
    }
}

void G01AutoTask::_servo_substate_jumpdowning() {
    this->jump_pm_handler_.run_once();
    // this->curr_cmd_axis_ = this->jump_pm_handler_.get_current_pos();
    s_motion_shared->set_global_cmd_axis(
        this->jump_pm_handler_.get_current_pos());

    // multi send
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = now - last_send_enable_votalge_gate_time_;
    if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >
        MULTI_SEND_INTERVAL) {
        cb_enable_votalge_gate_(true);
        last_send_enable_votalge_gate_time_ = now;
        // s_logger->info("downing multi");
    }

    if (this->jump_pm_handler_.is_over()) {
        // Down 走完, Buffer段要走伺服, 这里恢复line traj的长度

        line_traj_->set_curr_length(servoing_length_before_jump_ -
                                    jumping_param_.buffer_blu);

        s_logger->debug("jump down over: ltcurr-z: "
                        "{}, curr-z: {}, curr-length: {}",
                        this->line_traj_->curr_pos()[2],
                        s_motion_shared->get_global_cmd_axis()[2],
                        line_traj_->curr_length());

        _servo_substate_changeto(ServoSubState::JumpDowningBuffer);
    }
}

void G01AutoTask::_servo_substate_jumpdowningbuffer() {
    bool buffer_over = false;
    bool buffer_interrupted = false;

    // 获取一次伺服指令
    auto servo_cmd = _get_servo_cmd_from_shared();

    // 计算缓冲段剩余长度
    unit_t buffer_remaining_length =
        servoing_length_before_jump_ - line_traj_->curr_length();

    if (servo_cmd > 0.0) {
        if (servo_cmd >= buffer_remaining_length ||
            buffer_remaining_length <= 0) {
            // 缓冲段最后一次运动
            servo_cmd = buffer_remaining_length;

            s_logger->debug(
                "buffer over, servo_cmd:{}, buffer_remaining_length: {}",
                servo_cmd, buffer_remaining_length);

            buffer_over = true;
        }

        line_traj_->run_once(servo_cmd);
        // this->curr_cmd_axis_ = this->line_traj_->curr_pos();
        s_motion_shared->set_global_cmd_axis(this->line_traj_->curr_pos());

    } else if (servo_cmd <= 0.0) {
        s_logger->debug("buffer interrupted, buffer_remaining_length: {}",
                        buffer_remaining_length);
        buffer_interrupted = true; // 缓冲段有回退, 直接退出走正常伺服回退
    }

    if (buffer_over || buffer_interrupted) {
        last_jump_end_time_ms_ =
            GetCurrentTimeMs(); // 更新变量: 上一次抬刀结束时间

        s_logger->debug("jump down buffer over: ltcurr-z: "
                        "{}, curr-z: {}, curr-length: {}",
                        this->line_traj_->curr_pos()[2],
                        s_motion_shared->get_global_cmd_axis()[2],
                        line_traj_->curr_length());

        _servo_substate_changeto(ServoSubState::Servoing); // 抬刀全部结束
        return;
    }
}

bool G01AutoTask::_servoing_check_and_plan_jump() { // Jump Trigger
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    return false; // 小孔机不支持抬刀
#endif

    // 更新抬刀参数
    jumping_param_ = s_motion_shared->get_jump_param();

    // 检查抬刀间隔, 如果到了就尝试规划一次抬刀
    auto now_ms = GetCurrentTimeMs();
    assert(now_ms >= last_jump_end_time_ms_);
    if (now_ms - last_jump_end_time_ms_ <= (int64_t)jumping_param_.dn_ms) {
        return false; // 间隔未到
    }

    // 设定的抬刀高度小于10um, 基本上就是设定了 UP=0
    if (jumping_param_.up_blu < util::UnitConverter::um2blu(10)) {
        return false;
    }

    // 规划抬刀
    if (!_plan_jump_up()) {
        // 规划失败
        last_jump_end_time_ms_ = now_ms; // 更新一下上一次抬刀结束时间
        return false;
    }

    // 抬刀规划成功
    // 置状态, 操作电压
    _servo_substate_changeto(ServoSubState::JumpUping);
    cb_enable_votalge_gate_(false);
    last_send_enable_votalge_gate_time_ =
        std::chrono::high_resolution_clock::now();

    return true;
}

bool G01AutoTask::_servoing_do_servothings() {
    double servo_cmd =
        _get_servo_cmd_from_shared(); // return value's unit is blu

#ifndef EDM_USE_ZYNQ_SERVOBOARD
    if (s_motion_shared->get_settings().enable_g01_run_each_servo_cmd)
        [[unlikely]] {
        if (s_motion_shared->can_recv_buffer()->is_servo_data_new()) {
            s_motion_shared->can_recv_buffer()->clear_servo_data_new_flag();
        } else {
            servo_cmd = 0.0;
            return false; //! No need to continue
        }
    }
#endif

    // 记录数据
    auto data_record_instance1 = s_motion_shared->get_data_record_instance1();
    if (data_record_instance1->is_data_recorder_running()) {
        data_record_instance1->get_record_data_ref().g01_servo_cmd = servo_cmd;
        data_record_instance1->get_record_data_ref().is_g01_normal_servoing =
            true;
    }

    if (s_motion_shared->get_settings()
            .enable_g01_servo_with_dynamic_strategy) {
        // test get current pos

        // 获取驱动器当前实际位置
        auto act_axis = s_motion_shared->get_act_axis();

        // 测试打印 (每1000周期打印一次)
#if 1
        {
            static int jjj = 0;
            ++jjj;
            if (jjj > 1000) {
                jjj = 0;
                const auto &cmd_axis = s_motion_shared->get_global_cmd_axis();
                s_logger->debug("cmd_axis x: {}, y: {}, z: {}", cmd_axis.at(0),
                                cmd_axis.at(1), cmd_axis.at(2));

                s_logger->debug("act_axis x: {}, y: {}, z: {}", act_axis.at(0),
                                act_axis.at(1), act_axis.at(2));
            }
        }
#endif
    }

    if (servo_cmd > 0.0) {
        line_traj_->run_once(servo_cmd);
        if (line_traj_->at_end()) {
            // TODO Do Some Ending Check, like length filters ...

            // if satisfy all check, set stopped
            if (true) {
                // this->curr_cmd_axis_ = line_traj_->curr_pos();
                s_motion_shared->set_global_cmd_axis(line_traj_->curr_pos());
                return true;
            }
        }
    } else if (servo_cmd < 0.0) {
        line_traj_->run_once(servo_cmd);
        if (line_traj_->at_start()) {
            // TODO Maybe Trigger Warning After continuous `at start`
            //! 不需要支持回退到上一段, 那个以后放到多段加工
            // TODO 是否可以根据抬刀最大安全长度的值, 略微多回退一点,
            // 但这个要改输入的类型

            // Currently Do Nothing Here
        }
    }

    // 更新目前的计算位置
    // this->curr_cmd_axis_ = line_traj_->curr_pos();
    s_motion_shared->set_global_cmd_axis(line_traj_->curr_pos());

    return false;
}

bool G01AutoTask::_plan_jump_up() {
    servoing_length_before_jump_ = this->line_traj_->curr_length();

    if (!_check_and_validate_jump_height()) {
        s_logger->warn("plan jump failed: check jump height failed.");
        return false;
    }

    // 计算抬刀目标点
    const auto &curr_maching_dir = this->line_traj_->get_unit_vector();
    const auto &up_start_pos = this->line_traj_->curr_pos();
    for (std::size_t i = 0; i < this->jump_up_target_pos_.size(); ++i) {
        this->jump_up_target_pos_[i] =
            (up_start_pos[i] - curr_maching_dir[i] * jumping_param_.up_blu);
    }

    s_logger->debug("plan jump up: startpos-z: {}, targetpos-z: {}, ltcurr-z: "
                    "{}, curr-z: {}, curr-length: {}",
                    up_start_pos[2], jump_up_target_pos_[2],
                    this->line_traj_->curr_pos()[2],
                    s_motion_shared->get_global_cmd_axis()[2],
                    line_traj_->curr_length());

    return this->jump_pm_handler_.start(
        jumping_param_.speed_param, up_start_pos, this->jump_up_target_pos_);
}

bool G01AutoTask::_check_and_validate_jump_height() {
    unit_t max_jump_height =
        this->line_traj_->curr_length() + max_jump_height_from_begin_;

    // 预留100um间隙
    if (jumping_param_.up_blu >
        max_jump_height - util::UnitConverter::um2blu(100)) {
        jumping_param_.up_blu =
            max_jump_height - util::UnitConverter::um2blu(100);
    }

    // 如果处理过的抬刀高度小于10um, 不给抬刀
    if (jumping_param_.up_blu < util::UnitConverter::um2blu(10)) {
        return false;
    }

    // 缓冲距离判定, 缓冲距离不能超过当前段已走的长度
    // if (jumping_param_.buffer_blu >
    //     this->line_traj_->curr_length() - util::UnitConverter::um2blu(10)) {
    //     return false;
    // }
    if (jumping_param_.buffer_blu > this->line_traj_->curr_length()) {
        jumping_param_.buffer_blu = this->line_traj_->curr_length();
    }

    // 缓冲量太大, 接近抬刀长度了
    if (jumping_param_.buffer_blu >
        jumping_param_.up_blu - util::UnitConverter::um2blu(10)) {
        return false;
    }

    return true;
}

double G01AutoTask::_get_servo_cmd_from_shared() {
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    auto sv_speed =
        (double)s_motion_shared->cached_udp_message().servo_calced_speed_mm_min;

    return util::UnitConverter::mm_min2blu_p(sv_speed);
#else
    int dir = s_motion_shared->cached_servo_data().servo_direction;
    double dis_blu = util::UnitConverter::um2blu(
        (double)(s_motion_shared->cached_servo_data().servo_distance_0_001um) /
        1000.0);

    // EDM_CYCLIC_LOG(s_logger->debug, 200, "dis_blu: {}", dis_blu);

    if (dir > 0) {
        return dis_blu;
    } else {
        return -dis_blu;
    }
#endif
}

} // namespace move

} // namespace edm