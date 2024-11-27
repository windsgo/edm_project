#include "MotionStateMachine.h"

#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "Motion/MotionUtils/MotionUtils.h"

#include <cassert>

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

namespace edm {

namespace move {

static auto s_motion_shared = MotionSharedData::instance();

MotionStateMachine::MotionStateMachine(
    SignalBuffer::ptr signal_buffer,
    const std::function<void(bool)> &cb_enable_votalge_gate,
    const std::function<void(bool)> &cb_mach_on)
    : signal_buffer_(signal_buffer), 
      cb_enable_votalge_gate_(cb_enable_votalge_gate), cb_mach_on_(cb_mach_on) {

    //! use assert more than exception is better

    if (!cb_enable_votalge_gate_) {
        throw exception("no cb_enable_votalge_gate_");
    }

    if (!signal_buffer_) {
        throw exception("no signal_buffer_");
    }
    
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    auto physical_touch_detect_cb = [this]() -> bool {
        return s_motion_shared->cached_udp_message().touch_detected;
    };
    touch_detect_handler_ = std::make_shared<TouchDetectHandler>(physical_touch_detect_cb);
#else
    auto physical_touch_detect_cb = [this]() -> bool {
        return s_motion_shared->cached_servo_data().touch_detected;
    };
    touch_detect_handler_ = std::make_shared<TouchDetectHandler>(physical_touch_detect_cb);
#endif

    // cb_get_jump_param_ =
    //     std::bind_front(&MotionStateMachine::_get_jump_param, this);

    auto_task_runner_ =
        std::make_shared<AutoTaskRunner>(touch_detect_handler_, signal_buffer_);

    touch_detect_handler_->reset();
    reset();
    // 初始化计算坐标
    // 先清0 (离线调试模式时实际坐标会直接返回当前值)
    // MotionUtils::ClearAxis(cmd_axis_);
#ifndef EDM_OFFLINE_RUN_NO_ECAT
    // cb_get_act_axis_(cmd_axis_);
#endif // EDM_OFFLINE_RUN_NO_ECAT
}

void MotionStateMachine::run_once() {
#ifdef EDM_USE_ZYNQ_SERVOBOARD
    s_motion_shared->update_zynq_udpmessage_holder();
#else
    //! 状态机每周期开始更新can buffer缓存到本地
    s_motion_shared->update_can_buffer_cache();
#endif

    auto data_record_instance1 = s_motion_shared->get_data_record_instance1();
    if (data_record_instance1->is_data_recorder_running()) {
        //! 每周期开始, 将记录数据缓存清空
        data_record_instance1->clear_data_record();

        // 记录周期开始时驱动器返回的数据: 实际位置, 跟随误差
        auto& rd1 = data_record_instance1->get_record_data_ref();

        rd1.thread_tick_us = s_motion_shared->get_thread_tick_us();

        s_motion_shared->get_act_axis(rd1.act_axis);

#ifndef EDM_OFFLINE_RUN_NO_ECAT
        auto em = s_motion_shared->get_ecat_manager();
        for (int i = 0; i < EDM_SERVO_NUM; ++i) {
            auto d = em->get_servo_device(i);
            rd1.following_error_axis[i] = d->get_following_error();
        }
#endif // EDM_OFFLINE_RUN_NO_ECATs
        // 记录已经更新的放电信息反馈
#ifdef EDM_USE_ZYNQ_SERVOBOARD
        const auto& csd = s_motion_shared->cached_udp_message();
        rd1.average_voltage = csd.averaged_voltage;
        rd1.current = 0; // TODO
        rd1.normal_charge_rate = 0; // TODO
        rd1.short_charge_rate = 0; // TODO
        rd1.open_charge_rate = 0; // TODO
#else
        auto& csd = s_motion_shared->cached_servo_data();
        rd1.average_voltage = csd.average_voltage;
        rd1.current = csd.current;
        rd1.normal_charge_rate = csd.normal_rate;
        rd1.short_charge_rate = csd.short_rate;
        rd1.open_charge_rate = csd.open_rate;
#endif
    }

    if (!enabled_) {
        return;
    }

    switch (main_mode_) {
    case MotionMainMode::Idle:
        _mainmode_idle();
        break;
    case MotionMainMode::Manual:
        _mainmode_manual();
        break;
    case MotionMainMode::Auto:
        _mainmode_auto();
        break;
    default:
        s_logger->error("{}: unknown main mode: {}", __PRETTY_FUNCTION__,
                        static_cast<int>(main_mode_));
        assert(false);
        enabled_ = false;
        break;
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // 主轴控制
    auto spindle_control = s_motion_shared->get_spindle_controller();
    spindle_control->run_once();
#endif // (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

    if (data_record_instance1->is_data_recorder_running()) {
        data_record_instance1->get_record_data_ref().new_cmd_axis = s_motion_shared->get_global_cmd_axis();

        data_record_instance1->push_data_to_recorder();
    }
}

void MotionStateMachine::reset() {
    enabled_ = false;
    main_mode_ = MotionMainMode::Idle;
    // auto_state_ = MotionAutoState::Stopped;

    pm_handler_.clear();

    //    touch_detect_handler_->reset();

    auto_task_runner_->reset();
}

// void MotionStateMachine::set_cmd_axis(const axis_t &init_cmd_axis) {
//     s_logger->info("MotionStateMachine::set_cmd_axis.");
//     cmd_axis_ = init_cmd_axis;
// }

// void MotionStateMachine::refresh_axis_using_actpos() {
//     s_logger->info("MotionStateMachine::refresh_axis_using_actpos.");
//     if (cb_get_act_axis_) {
//         cb_get_act_axis_(this->cmd_axis_);
//     }
// }

bool MotionStateMachine::start_manual_pointmove(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &target_pos) {
    switch (main_mode_) {
    case MotionMainMode::Idle: {
        s_logger->trace("{}: idle mode.", __PRETTY_FUNCTION__);
        auto ret = pm_handler_.start(speed_param, s_motion_shared->get_global_cmd_axis(), target_pos);
        if (ret) {
            _mainmode_switch_to(MotionMainMode::Manual);
            signal_buffer_->set_signal(MotionSignal_ManualPointMoveStarted);

            //! test
            // r_.start_record("output.bin");
        }
        return ret;
    }
    case MotionMainMode::Manual:
        s_logger->warn("{}: manual mode not stopped", __PRETTY_FUNCTION__);
        return false;
    case MotionMainMode::Auto:
        s_logger->trace("{}: in auto mode", __PRETTY_FUNCTION__);
        return auto_task_runner_->start_manual_pointmove(speed_param, s_motion_shared->get_global_cmd_axis(),
                                                         target_pos);
        // 处理暂停点动的启动.
        break;
    default:
        return false;
    }
}

bool MotionStateMachine::stop_manual_pointmove(bool immediate) {
    switch (main_mode_) {
    case MotionMainMode::Idle:
        s_logger->warn("{}: idle mode.", __PRETTY_FUNCTION__);
        return false;
    case MotionMainMode::Manual:
        s_logger->trace("{}: in manual mode, immediate = {}",
                        __PRETTY_FUNCTION__, immediate);
        return pm_handler_.stop(immediate);
    case MotionMainMode::Auto:
        s_logger->trace("{}: in auto mode, immediate = {}", __PRETTY_FUNCTION__,
                        immediate);
        return auto_task_runner_->stop_manual_pointmove(immediate);
        break;
    default:
        return false;
    }
}

bool MotionStateMachine::start_auto_g00(
    const MoveRuntimePlanSpeedInput &speed_param, const axis_t &target_pos,
    bool enable_touch_detect) {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Idle) {
        return false;
    }

    auto new_g00_auto_task = std::make_shared<G00AutoTask>(
        target_pos, speed_param, enable_touch_detect,
        touch_detect_handler_);

    if (new_g00_auto_task->is_over()) {
        return false;
    }

    auto ret = auto_task_runner_->restart_task(new_g00_auto_task);
    if (!ret) {
        return false;
    }

    signal_buffer_->set_signal(MotionSignal_AutoStarted);
    _mainmode_switch_to(MotionMainMode::Auto);
    return true;
}

bool MotionStateMachine::start_auto_g01(const axis_t &target_pos,
                                        unit_t max_jump_height_from_begin) {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Idle) {
        return false;
    }

    auto g01_line_traj =
        std::make_shared<TrajectoryLinearSegement>(s_motion_shared->get_global_cmd_axis(), target_pos);

    auto new_g01_auto_task = std::make_shared<G01AutoTask>(
        g01_line_traj, max_jump_height_from_begin,
        this->cb_enable_votalge_gate_, this->cb_mach_on_);

    if (new_g01_auto_task->is_over()) {
        return false;
    }

    auto ret = auto_task_runner_->restart_task(new_g01_auto_task);
    if (!ret) {
        return false;
    }

    signal_buffer_->set_signal(MotionSignal_AutoStarted);
    _mainmode_switch_to(MotionMainMode::Auto);
    return true;
}

bool MotionStateMachine::start_auto_g04(double deley_s) {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Idle) {
        return false;
    }

    auto new_g04_auto_task = std::make_shared<G04AutoTask>(deley_s);

    if (new_g04_auto_task->is_over()) {
        return false;
    }

    auto ret = auto_task_runner_->restart_task(new_g04_auto_task);
    if (!ret) {
        return false;
    }

    signal_buffer_->set_signal(MotionSignal_AutoStarted);
    _mainmode_switch_to(MotionMainMode::Auto);
    return true;
}

bool MotionStateMachine::start_auto_m00fake() {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Idle) {
        return false;
    }

    auto new_m00fake_auto_task = std::make_shared<M00FakeAutoTask>();

    if (new_m00fake_auto_task->is_over()) {
        return false;
    }

    auto ret = auto_task_runner_->restart_task(new_m00fake_auto_task);
    if (!ret) {
        return false;
    }

    signal_buffer_->set_signal(MotionSignal_AutoStarted);
    _mainmode_switch_to(MotionMainMode::Auto);
    return true;
}

bool MotionStateMachine::pause_auto() {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Auto) {
        return false;
    }

    return auto_task_runner_->pause();
}

bool MotionStateMachine::resume_auto() {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Auto) {
        return false;
    }

    return auto_task_runner_->resume();
}

bool MotionStateMachine::stop_auto(bool immediate) {
    s_logger->trace("{}", __PRETTY_FUNCTION__);

    if (main_mode_ != MotionMainMode::Auto) {
        return false;
    }

    return auto_task_runner_->stop(immediate);
}

void MotionStateMachine::_mainmode_idle() {
    // TODO
    touch_detect_handler_->set_detect_enable(false);

    // static util::SlidingFilter<double> sf {1000};

    // double cmd = cb_get_servo_cmd_();
    // sf.push_back(cmd);

    // printf("%0.3f\t %0.3f\n", cmd, sf.average());
}

void MotionStateMachine::_mainmode_manual() {
    if (pm_handler_.is_over()) {
        // emit manual stopped signal
        signal_buffer_->set_signal(MotionSignal_ManualPointMoveStopped);
        _mainmode_switch_to(MotionMainMode::Idle);
        // r_.stop_record();
        return;
    }

    // touch detect
    if (touch_detect_handler_->run_detect_once()) {
        pm_handler_.stop(true);
        signal_buffer_->set_signal(MotionSignal_ManualPointMoveStopped);
        // 接触感知报警需要上层控制处理
        _mainmode_switch_to(MotionMainMode::Idle);
        return; // return 掉, 防止继续run 或产生赋值
    }

    pm_handler_.run_once();

    // double last = cmd_axis_[0];

    // cmd_axis_ = pm_handler_.get_current_pos();

    // set to global cmd axis
    s_motion_shared->set_global_cmd_axis(pm_handler_.get_current_pos());

    // r_.emplace(cmd_axis_[0] - last, last, cmd_axis_[0]);
}

void MotionStateMachine::_mainmode_auto() {
    auto_task_runner_->run_once();

    // cmd_axis_ = auto_task_runner_->get_curr_cmd_axis();

    if (auto_task_runner_->state() == MotionAutoState::Stopped) {
        _mainmode_switch_to(MotionMainMode::Idle);
        return;
    }
}

void MotionStateMachine::_mainmode_switch_to(MotionMainMode new_main_mode) {
    s_logger->trace("MainMode: {} -> {}", GetMainModeStr(main_mode_),
                    GetMainModeStr(new_main_mode));
    main_mode_ = new_main_mode;
}

const char *MotionStateMachine::GetMainModeStr(MotionMainMode mode) {
    switch (mode) {
#define XX_(m)              \
    case MotionMainMode::m: \
        return #m;
        XX_(Idle)
        XX_(Manual)
        XX_(Auto)
#undef XX_
    default:
        return "Unknow";
    }
}

} // namespace move

} // namespace edm
