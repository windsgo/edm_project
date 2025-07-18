#include "GCodeRunner.h"

#include "Coordinate/Coordinate.h"
#include "Logger/LogMacro.h"
#include "Motion/MotionUtils/MotionUtils.h"
#include "Motion/MoveDefines.h"
#include "QtDependComponents/PowerController/EleparamDefine.h"
#include "SystemSettings/SystemSettings.h"
#include "TaskManager/GCodeTask.h"
#include "TaskManager/GCodeTaskBase.h"
#include "Utils/Format/edm_format.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include "config.h"
#include "spdlog/spdlog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <bits/chrono.h>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <memory>
#include <sstream>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());
EDM_STATIC_LOGGER_NAME(s_loglist, "loglist")

static std::string get_time_str_from_timepoint(
    const std::chrono::system_clock::time_point &time_point,
    const char* format = "%Y-%m-%d %H:%M:%S") {
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    auto tm = std::localtime(&time_t);
    std::stringstream ss;
    ss << std::put_time(tm, format);
    return ss.str();
}   

namespace edm {

namespace task {

GCodeRunner::GCodeRunner(app::SharedCoreData *shared_core_data, QObject *parent)
    : shared_core_data_(shared_core_data) {
    update_timer_ = new QTimer(this);
    connect(update_timer_, &QTimer::timeout, this, &GCodeRunner::_run_once);

    _reset_state();
    _init_help_connections();

    // QTimer *light_timer = new QTimer(this);
    // connect(light_timer, &QTimer::timeout, this,
    //         &GCodeRunner::_auto_led_control);
    // light_timer->start(1000);
}

bool GCodeRunner::start(const std::vector<GCodeTaskBase::ptr> &gcode_list) {
    if (state_ != State::Stopped) {
        s_logger->error("GCodeRunner start failed: not stopped");
        return false;
    }

    if (!this->is_over()) {
        s_logger->error("GCodeRunner start failed: not over");
        return false;
    }

    gcode_list_ = gcode_list;
    curr_gcode_num_ = 0;
    total_elapsed_time_ = std::chrono::system_clock::duration::zero();

    if (!_check_gcode_list_at_first()) {
        return false; //! set abort inside _check_xxx()
    }

    update_timer_->start(update_timer_regular_peroid_ms_);
    _switch_to_state(State::ReadyToStart);
    return true;
}

bool GCodeRunner::pause() {
    if (this->is_over()) {
        return false;
    }

    auto curr_gcode = gcode_list_[curr_gcode_num_];

    switch (state_) {
    case State::WaitingForResumed:
    case State::Running: {
        if (!curr_gcode->is_motion_task()) {
            // 本地运行的task, 直接暂停在本地
            _switch_to_state(State::Paused);
            emit sig_auto_paused();

            curr_gcode->timer().pause();
            // s_logger->debug(
            //     "in GCodeRunner::pause: 暂停计时器(local), gcode_num: {}, line: {}, code: {}",
            //     curr_gcode_num_, curr_gcode->line_number(),
            //     curr_gcode->get_gcode_str());

            return true;
        } else {
            bool ret = _cmd_auto_pause();
            if (!ret) {
                s_logger->warn("motion task pause failed");
                delay_pause_flag_ = true; // delay pause
                return false;
            }

            _switch_to_state(WaitingForPaused);

            curr_gcode->timer().pause();
            // s_logger->debug(
            //     "in GCodeRunner::pause: 暂停计时器(motion), gcode_num: {}, line: {}, code: {}",
            //     curr_gcode_num_, curr_gcode->line_number(),
            //     curr_gcode->get_gcode_str());
            return true;
        }
    }

    case State::WaitingForPaused:
    case State::Paused:
        return true;
    case State::CurrentNodeIniting:
        delay_pause_flag_ = true; // delay pause
        s_logger->warn("pause failed, when CurrentNodeIniting");
    case State::WaitingForStopped:
    case State::Stopped:
    default:
        return false;
    }
}

bool GCodeRunner::resume() {
    if (this->is_over()) {
        return false;
    }

    auto curr_gcode = gcode_list_[curr_gcode_num_];

    switch (state_) {
    case State::Paused: {
        if (!curr_gcode->is_motion_task()) {
            // 本地运行的task, 直接继续运行
            _switch_to_state(State::Running);
            emit sig_auto_resumed();

            curr_gcode->timer().resume();
            // s_logger->debug(
            //     "in GCodeRunner::resume: 继续计时器(local), gcode_num: {}, line: {}, code: {}",
            //     curr_gcode_num_, curr_gcode->line_number(),
            //     curr_gcode->get_gcode_str());

            return true;
        } else {
            bool ret = _cmd_auto_resume();
            if (!ret) {
                return false;
            }

            _switch_to_state(WaitingForResumed);

            curr_gcode->timer().resume();
            // s_logger->debug(
            //     "in GCodeRunner::resume: 继续计时器(motion), gcode_num: {}, line: {}, code: {}",
            //     curr_gcode_num_, curr_gcode->line_number(),
            //     curr_gcode->get_gcode_str());

            return true;
        }
    }
    case State::WaitingForResumed:
        return true;
    case State::CurrentNodeIniting:
    case State::Running:
    case State::WaitingForPaused:
    case State::WaitingForStopped:
    case State::Stopped:
    default:
        return false;
    }
    return false;
}

bool GCodeRunner::stop() {
    if (this->is_over()) {
        return false;
    }

    auto curr_gcode = gcode_list_[curr_gcode_num_];

    switch (state_) {
    case State::WaitingForResumed:
    case State::Paused:
    case State::Running: {
        if (!curr_gcode->is_motion_task()) {
            // 本地运行的task, 直接停止
            _end();
            return true;
        } else {
            bool ret = _cmd_auto_stop();
            if (!ret) {

                // 可能存在一段已经走完, 但本地还没更新为Stopped, 这里强制停止,
                // 防止停不下来
                _cmd_emergency_stop();
                _end();

                return true;
            }

            _switch_to_state(WaitingForStopped);
            return true;
        }
    }
    case State::CurrentNodeIniting: {
        _end(); // 直接停止 // TODO 要注意NURBS初始化时如果停止,
                // 对应的future是否需要释放, 资源释放问题
        return true;
    }
    case State::WaitingForStopped:
    case State::Stopped:
        return true;
    case State::WaitingForPaused: {
        bool ret = _cmd_auto_stop();
        if (!ret) {
            return false;
        }

        _switch_to_state(WaitingForStopped);
        return true;
    }
    default:
        return false;
    }
    return false;
}

bool GCodeRunner::estop() {
    auto ret = _cmd_emergency_stop();

    if (ret) {
        if (this->is_over()) {
            _reset_state();
        } else {
            _abort("ESTOP");
        }
    }

    return ret;
}

std::vector<std::string> GCodeRunner::get_time_report() const {
    std::vector<std::string> str_vec;

    for (int i = 0; i < gcode_list_.size(); ++i) {
        const auto &gcode = gcode_list_[i];

        if (gcode->timer().state() != TaskTimer::State::Stopped) {
            // 没执行完的
            continue;
            // break;
        }

        auto elapsed_time = gcode->timer().elapsed_time();
        auto start_time_str =
            get_time_str_from_timepoint(gcode->timer().start_time());
        auto end_time_str =
            get_time_str_from_timepoint(gcode->timer().end_time());
        
        auto str = EDM_FMT::format("[{} -> {}] [{:4}s] line: {}, code: {}",
                                   start_time_str, end_time_str,
                                   std::chrono::duration_cast<std::chrono::seconds>(
                                       elapsed_time)
                                       .count(),
                                   gcode->line_number(),
                                   gcode->get_gcode_str());

        str_vec.push_back(str);
    }

    return str_vec;
}

bool GCodeRunner::_cmd_auto_pause() {
    // create motion cmd
    auto auto_pause_command =
        std::make_shared<edm::move::MotionCommandAutoPause>();
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_pause_command);

    return TaskHelper::WaitforCmdTobeAccepted(auto_pause_command, -1);
}

bool GCodeRunner::_cmd_auto_resume() {
    // create motion cmd
    auto auto_resume_command =
        std::make_shared<edm::move::MotionCommandAutoResume>();
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_resume_command);

    return TaskHelper::WaitforCmdTobeAccepted(auto_resume_command, -1);
}

bool GCodeRunner::_cmd_auto_stop() {
    // create motion cmd
    auto auto_stop_command =
        std::make_shared<edm::move::MotionCommandAutoStop>(false);
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_stop_command);

    return TaskHelper::WaitforCmdTobeAccepted(auto_stop_command, -1);
}

bool GCodeRunner::_cmd_emergency_stop() {
    auto estop_cmd =
        std::make_shared<edm::move::MotionCommandManualEmergencyStopAllMove>();

    this->shared_core_data_->get_motion_cmd_queue()->push_command(estop_cmd);

    return TaskHelper::WaitforCmdTobeAccepted(estop_cmd, -1);
}

void GCodeRunner::_run_once() {
    // 直接获取最新的info
#ifdef EDM_MOTION_INFO_GET_USE_ATOMIC
    shared_core_data_->get_motion_thread_ctrler()->load_at_info_cache(
        local_info_cache_);
#else  // EDM_MOTION_INFO_GET_USE_ATOMIC
    local_info_cache_ =
        shared_core_data_->get_motion_thread_ctrler()->get_info_cache();
#endif // EDM_MOTION_INFO_GET_USE_ATOMIC

    switch (state_) {
    case State::ReadyToStart: {
        _switch_to_state(State::CurrentNodeIniting);
        emit sig_auto_started();
        s_loglist->info("GCodeRunner Start");
        break;
    }
    case State::CurrentNodeIniting: {
        _state_current_node_initing();
        break;
    }
    case State::Running: {
        _state_running();
        break;
    }
    case State::WaitingForPaused: {
        _state_waiting_for_paused();
        break;
    }
    case State::Paused: {
        // Do Nothing here
        auto curr_gcode = gcode_list_[curr_gcode_num_];
        if (curr_gcode->timer().state() != TaskTimer::State::Paused) {
            curr_gcode->timer().pause();
            s_logger->warn(
                "in GCodeRunner::Paused: timer state not paused, paused now, gcode_num: {}, "
                "line: {}, code: {}",
                curr_gcode_num_, curr_gcode->line_number(),
                curr_gcode->get_gcode_str());
        }
        break;
    }
    case State::WaitingForResumed: {
        _state_waiting_for_resumed();
        break;
    }
    case State::WaitingForStopped: {
        _state_waiting_for_stopped();
        break;
    }
    case State::Stopped: {
        // Do Nothing here
        auto curr_gcode = gcode_list_[curr_gcode_num_];
        if (curr_gcode->timer().state() != TaskTimer::State::Stopped) {
            curr_gcode->timer().stop();
            s_logger->warn(
                "in GCodeRunner::Stopped: timer state not stopped, stopped now, gcode_num: {}, "
                "line: {}, code: {}",
                curr_gcode_num_, curr_gcode->line_number(),
                curr_gcode->get_gcode_str());
        }
        break;
    }
    default:
        assert(false);
        break;
    }

    if (delay_pause_flag_) {
        auto ret = this->pause();
        if (ret) {
            delay_pause_flag_ = false;
        }
    }
}

bool GCodeRunner::_check_gcode_list_at_first() {
    for (const auto &g : gcode_list_) {
        // 判断电参数号是否存在
        if (g->type() == GCodeTaskType::EleparamSetCommand) {
            auto eg = std::static_pointer_cast<GCodeTaskEleparamSet>(g);
            auto exist = this->shared_core_data_->get_power_manager()
                             ->exist_eleparam_index(eg->eleparam_index());
            if (!exist) {
                _abort(EDM_FMT::format("check err: eleparam not exist: {}",
                                       eg->eleparam_index()));
                return false;
            }
        }
        // 检查坐标系序号是否存在
        else if (g->type() == GCodeTaskType::CoordinateIndexCommand) {
            auto cg = std::static_pointer_cast<GCodeTaskCoordinateIndex>(g);
            auto exist = this->shared_core_data_->get_coord_system()
                             ->exist_coordinate_index(cg->coord_index());
            if (!exist) {
                _abort(EDM_FMT::format("check err: coord index not exist: {}",
                                       cg->coord_index()));
                return false;
            }
        }
    }

    // 判断最后一个是不是M02

    if (gcode_list_.back()->type() != GCodeTaskType::ProgramEndCommand) {
        _abort("check err: last gcode is not m02");
        return false;
    }

    return true;
}

void GCodeRunner::_abort(std::string_view error_str) {
    s_logger->error("GCodeRunner Abort: {}", error_str);
    s_loglist->error("GCodeRunner Abort: {}", error_str);

    s_logger->info("Due to abort, emergency stopping motion");
    _cmd_emergency_stop();

    _reset_state();
    last_error_str_ = error_str;
    emit sig_auto_stopped(true);
}

void GCodeRunner::_end() {
    s_logger->info("GCodeRunner End");
    s_loglist->info("GCodeRunner End");
    _switch_to_state(State::Stopped);

    _reset_state();
    last_error_str_.clear();
    emit sig_auto_stopped(false);

    // 测试打印时间
    // for (int i = 0; i < gcode_list_.size(); ++i) {
    //     const auto& gcode = gcode_list_[i];

    //     if (gcode->timer().state() == TaskTimer::State::Idle) {
    //         break;
    //     }

    //     auto elapsed_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
    //         gcode->timer().elapsed_time()).count();
        
    //     s_logger->info(
    //         "{} - {}: {:04}ms: node[{:02}]: {}",
    //         get_time_str_from_timepoint(gcode->timer().start_time()),
    //         get_time_str_from_timepoint(gcode->timer().end_time()),
    //         elapsed_time_ms,
    //         i,
    //         gcode->get_gcode_str());
    // }
}

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
void GCodeRunner::_drill_record_data(bool start) {
    if (!SystemSettings::instance().get_drill_settings().auto_record_data) {
        return;
    }

    emit sig_record_data(2, start); // motion data
    emit sig_record_data(0xAE, start); // audio signal (AE signal)
}
#endif

void GCodeRunner::_check_to_next_gcode() {
    auto curr_over_gcode = gcode_list_[curr_gcode_num_];

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    if (curr_over_gcode) {
        if (curr_over_gcode->type() == GCodeTaskType::DrillMotionCommand) {
            _drill_record_data(false); // stop record data2
        }
    }
#endif

    curr_over_gcode->timer().stop();
    total_elapsed_time_ += curr_over_gcode->timer().elapsed_time();
    // s_logger->debug(
    //     "in _check_to_next_gcode: 结束计时器, gcode_num: {}, line: {}, code: {}",
    //     curr_gcode_num_, curr_over_gcode->line_number(),
    //     curr_over_gcode->get_gcode_str());

    // next gcode
    ++curr_gcode_num_;

    if (curr_gcode_num_ >= gcode_list_.size()) {
        _abort("_check_to_next_gcode: gcode overflow");
        // _end();
        return;
    }

    _switch_to_state(State::CurrentNodeIniting);
}

void GCodeRunner::_reset_state() {
    state_ = State::Stopped;

    // 结束这个node的计时
    if (curr_gcode_num_ < gcode_list_.size()) {
        auto curr_gcode = gcode_list_[curr_gcode_num_];
        if (curr_gcode) {
            curr_gcode->timer().stop();
        }
        s_logger->debug("in _reset_state: 结束计时器, gcode_num: {}",
                        curr_gcode_num_);
    }

    // curr_gcode_num_ = -1; //! reset to -1, means the state is un-inited
    update_timer_->stop();
    // gcode_list_.clear();
    delay_pause_flag_ = false;
}

void GCodeRunner::_init_help_connections() {
    auto info_dispatcher = shared_core_data_->get_info_dispatcher();

    // 不连接started信号
    connect(info_dispatcher, &InfoDispatcher::sig_auto_resumed, this,
            &GCodeRunner::_run_once);
    connect(info_dispatcher, &InfoDispatcher::sig_auto_paused, this,
            &GCodeRunner::_run_once);
    connect(info_dispatcher, &InfoDispatcher::sig_auto_stopped, this,
            &GCodeRunner::_run_once);
}

void GCodeRunner::_state_current_node_initing() {
    assert(curr_gcode_num_ < gcode_list_.size());
    auto curr_gcode = gcode_list_[curr_gcode_num_];
    assert(curr_gcode);

    s_loglist->trace("GCodeRunner switch node: {}, line: {}, code: {}", curr_gcode_num_,
                     curr_gcode->line_number(), curr_gcode->get_gcode_str());
    s_logger->trace(
        "GCodeRunner switch node: {}, line: {}, code: {}", curr_gcode_num_,
        curr_gcode->line_number(), curr_gcode->get_gcode_str());
    
    if (!curr_gcode->timer().is_running()) {
        curr_gcode->timer().restart(); // 启动计时器
        // s_logger->debug(
        //     "in _state_current_node_initing: 启动计时器, gcode_num: {}, line: {}, code: {}",
        //     curr_gcode_num_, curr_gcode->line_number(), curr_gcode->get_gcode_str());
    }

    //! 可以在这里emit, 更好的是在 ++curr_gcode_num_ 的时候emit
    emit sig_autogcode_switched_to_line(curr_gcode->line_number());

    // 非Motion GCode
    if (!curr_gcode->is_motion_task()) {
        _switch_to_state(State::Running);
        return;
    }

    // do motion task prepare and send
    switch (curr_gcode->type()) {
    case GCodeTaskType::G00MotionCommand: {
        auto g00_gcode =
            std::static_pointer_cast<GCodeTaskG00Motion>(curr_gcode);

        // check coord index
        if (!shared_core_data_->get_coord_system()->exist_coordinate_index(
                g00_gcode->coord_index())) {
            _abort(EDM_FMT::format("abort: g00 coord index not exist: {}",
                                   g00_gcode->coord_index()));
            break;
        }

        // motor start pos
        const auto &motor_start_pos = local_info_cache_.curr_cmd_axis_blu;

        // mach start pos
        move::axis_t mach_start_pos;
        shared_core_data_->get_coord_system()->get_cm().motor_to_machine(
            motor_start_pos, mach_start_pos);

        assert(g00_gcode->cmd_values().size() == 6);

        // 根据增量/绝对模式给出 机床坐标系目标位置
        move::axis_t mach_target_pos{0.0};
        const auto &cmd_values = g00_gcode->cmd_values();
        if (g00_gcode->coord_mode() == GCodeCoordinateMode::IncrementMode) {
            // inc
            mach_target_pos = mach_start_pos;
            for (std::size_t i = 0; i < EDM_AXIS_NUM; ++i) {
                if (cmd_values[i]) {
                    mach_target_pos[i] +=
                        util::UnitConverter::mm2blu(*(cmd_values[i]));
                }
            }
        } else {
            // abs, 目前只有工件坐标系模式, 没有机床坐标系模式
            uint32_t coord_index = g00_gcode->coord_index();

            // 先将输入的机床坐标起点转化为坐标系坐标起点
            move::axis_t coord_start_pos;
            bool ret1 = shared_core_data_->get_coord_system()
                            ->get_cm()
                            .machine_to_coord(coord_index, mach_start_pos,
                                              coord_start_pos);
            if (!ret1) {
                _abort(EDM_FMT::format("abort: g00 machine_to_coord failed: {}",
                                       coord_index));
                break;
            }

            // 根据cmd_values设定coord_target_pos
            move::axis_t coord_target_pos;
            for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                if (cmd_values[i]) {
                    coord_target_pos[i] =
                        util::UnitConverter::mm2blu(*(cmd_values[i]));
                } else {
                    coord_target_pos[i] = coord_start_pos[i];
                }
            }

            // 再转化为 MachTargetPos
            bool ret2 = shared_core_data_->get_coord_system()
                            ->get_cm()
                            .coord_to_machine(coord_index, coord_target_pos,
                                              mach_target_pos);
            if (!ret2) {
                _abort(EDM_FMT::format("abort: g00 coord_to_machine failed: {}",
                                       coord_index));
                break;
            }
        }

        if (move::MotionUtils::IsAxisTheSame(mach_start_pos, mach_target_pos)) {
            s_logger->warn("g00 failed, next node : start and target the same");
            _check_to_next_gcode();
            break;
        }

        // 根据mach_target_pos, 判断软限位
        auto curr_dir = move::MotionUtils::CalcAxisUnitVector(mach_start_pos,
                                                              mach_target_pos);
        auto sl_check_ret = TaskHelper::CheckPosandnegSoftLimit(
            shared_core_data_->get_coord_system(), mach_target_pos, curr_dir);
        if (!sl_check_ret) {
            if (!g00_gcode->is_touch_motion()) {
                _abort(EDM_FMT::format("abort: g00 softlimit reached"));
                break;
            } else {
                // touch motion, 将各坐标数据按当前方向(target - start)折算到,
                // 不超过软限位的最大长度的位置

                const auto &pos_sl =
                    shared_core_data_->get_coord_system()->get_pos_soft_limit();
                const auto &neg_sl =
                    shared_core_data_->get_coord_system()->get_neg_soft_limit();

                // 计算当前方向上剩余的长度
                move::unit_t min_scale =
                    0.0; // 对每个方向都计算单位向量方向上的剩余scale,
                         // 并取最小的一个scale(这个最小的scale就是该方向上剩余的长度)
                bool refreshed_scale = false;
                for (size_t i = 0; i < EDM_AXIS_NUM; ++i) {
                    move::unit_t scale_left = 0.0;
                    if (curr_dir[i] == 0.0) {
                        continue;
                    } else if (curr_dir[i] > 0.0) {
                        scale_left =
                            (pos_sl[i] - mach_start_pos[i]) / curr_dir[i];
                        if (scale_left <= 0.0) {
                            s_logger->warn(
                                "abort: g00(touch) softlimit pos reached {}",
                                i);
                        }
                    } else if (curr_dir[i] < 0.0) {
                        scale_left =
                            (neg_sl[i] - mach_start_pos[i]) / curr_dir[i];
                        if (scale_left <= 0.0) {
                            s_logger->warn(
                                "abort: g00(touch) softlimit neg reached {}",
                                i);
                        }
                    }
                    s_logger->debug(
                        "i: {}, min_scale: {}, scale_left: {}, pos_sl: {}, "
                        "mach_start_pos: {}, mach_target_pos: {}",
                        i, min_scale, scale_left, pos_sl[i], mach_start_pos[i],
                        mach_target_pos[i]);
                    if (!refreshed_scale || scale_left < min_scale) {
                        min_scale = scale_left > 0 ? scale_left : 0;
                        refreshed_scale = true;
                    }
                }
                if (min_scale <= 0.0) {
                    s_logger->warn(
                        "g00 failed, next node : g00(touch) softlimit reached");
                    _check_to_next_gcode();
                    break;
                }

                for (size_t i = 0; i < EDM_AXIS_NUM; ++i) {
                    mach_target_pos[i] =
                        mach_start_pos[i] + curr_dir[i] * min_scale;
                    s_logger->debug("target[{}]: {}", i, mach_target_pos[i]);
                }
            }
        }

        move::axis_t motor_target_pos;
        shared_core_data_->get_coord_system()->get_cm().machine_to_motor(
            mach_target_pos, motor_target_pos);

        auto speed = TaskHelper::GetDefaultSpeedparam();
        speed.cruise_v =
            util::UnitConverter::mm_min2blu_s(g00_gcode->feed_speed());

        if (speed.cruise_v <= 0.0)
            speed.cruise_v = 1.0; // for safe

        // send g00 cmd
        auto g00_cmd =
            std::make_shared<move::MotionCommandAutoStartG00FastMove>(
                motor_start_pos, motor_target_pos, speed,
                g00_gcode->touch_detect_enable());

        shared_core_data_->get_motion_cmd_queue()->push_command(g00_cmd);

        // 等待命令被接收
        if (!TaskHelper::WaitforCmdTobeAccepted(g00_cmd, 1000)) {
            _abort("abort: start g00 failed, cmd not accepted by motion or "
                   "timeout");
            break;
        }

        _switch_to_state(State::Running);

        break;
    }
    case GCodeTaskType::G01MotionCommand: {
        auto g01_gcode =
            std::static_pointer_cast<GCodeTaskG01Motion>(curr_gcode);

        // check coord index
        if (!shared_core_data_->get_coord_system()->exist_coordinate_index(
                g01_gcode->coord_index())) {
            _abort(EDM_FMT::format("abort: g01 coord index not exist: {}",
                                   g01_gcode->coord_index()));
            break;
        }

        // motor start pos
        const auto &motor_start_pos = local_info_cache_.curr_cmd_axis_blu;

        // mach start pos
        move::axis_t mach_start_pos;
        shared_core_data_->get_coord_system()->get_cm().motor_to_machine(
            motor_start_pos, mach_start_pos);

        assert(g01_gcode->cmd_values().size() == 6);

        // 根据增量/绝对模式给出 机床坐标系目标位置
        move::axis_t mach_target_pos{0.0};
        const auto &cmd_values = g01_gcode->cmd_values();
        if (g01_gcode->coord_mode() == GCodeCoordinateMode::IncrementMode) {
            // inc
            mach_target_pos = mach_start_pos;
            for (std::size_t i = 0; i < EDM_AXIS_NUM; ++i) {
                if (cmd_values[i]) {
                    mach_target_pos[i] +=
                        util::UnitConverter::mm2blu(*(cmd_values[i]));
                }
            }
        } else {
            // abs, 目前只有工件坐标系模式, 没有机床坐标系模式
            uint32_t coord_index = g01_gcode->coord_index();

            // 先将输入的机床坐标起点转化为坐标系坐标起点
            move::axis_t coord_start_pos;
            bool ret1 = shared_core_data_->get_coord_system()
                            ->get_cm()
                            .machine_to_coord(coord_index, mach_start_pos,
                                              coord_start_pos);
            if (!ret1) {
                _abort(EDM_FMT::format("abort: g01 machine_to_coord failed: {}",
                                       coord_index));
                break;
            }

            // 根据cmd_values设定coord_target_pos
            move::axis_t coord_target_pos;
            for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                if (cmd_values[i]) {
                    coord_target_pos[i] =
                        util::UnitConverter::mm2blu(*(cmd_values[i]));
                } else {
                    coord_target_pos[i] = coord_start_pos[i];
                }
            }

            // 再转化为 MachTargetPos
            bool ret2 = shared_core_data_->get_coord_system()
                            ->get_cm()
                            .coord_to_machine(coord_index, coord_target_pos,
                                              mach_target_pos);
            if (!ret2) {
                _abort(EDM_FMT::format("abort: g01 coord_to_machine failed: {}",
                                       coord_index));
                break;
            }
        }

        if (move::MotionUtils::IsAxisTheSame(mach_start_pos, mach_target_pos)) {
            s_logger->warn("g10 failed, next node : start and target the same");
            _check_to_next_gcode();
            break;
        }

        // 根据mach_target_pos, 判断软限位
        auto curr_dir = move::MotionUtils::CalcAxisUnitVector(mach_start_pos,
                                                              mach_target_pos);
        auto sl_check_ret = TaskHelper::CheckPosandnegSoftLimit(
            shared_core_data_->get_coord_system(), mach_target_pos, curr_dir);
        if (!sl_check_ret) {
            _abort(EDM_FMT::format("abort: g01 softlimit reached"));
            break;
        }

        move::axis_t motor_target_pos;
        shared_core_data_->get_coord_system()->get_cm().machine_to_motor(
            mach_target_pos, motor_target_pos);

        // 计算从开始点的最大抬刀距离
        move::axis_t jump_dir = move::MotionUtils::CalcAxisUnitVector(
            mach_target_pos, mach_start_pos);
        auto max_length_from_start = TaskHelper::GetMaxLengthOnCurrentDir(
            shared_core_data_->get_coord_system(), jump_dir, mach_start_pos);
        s_logger->debug("** max jump length: {}", max_length_from_start);

        // send g01 cmd
        auto g01_cmd =
            std::make_shared<move::MotionCommandAutoStartG01ServoMove>(
                motor_start_pos, motor_target_pos, max_length_from_start);

        shared_core_data_->get_motion_cmd_queue()->push_command(g01_cmd);

        // 等待命令被接收
        if (!TaskHelper::WaitforCmdTobeAccepted(g01_cmd, 1000)) {
            _abort("abort: start g01 failed, cmd not accepted by motion or "
                   "timeout");
            break;
        }

        _switch_to_state(State::Running);

        s_loglist->trace("GCodeRunner G01 Started");

        break;
    }
    case GCodeTaskType::DelayCommand: {
        auto g04_gcode = std::static_pointer_cast<GCodeTaskDeley>(curr_gcode);

        auto g04_cmd = std::make_shared<move::MotionCommandAutoG04Delay>(
            g04_gcode->delay_s());

        shared_core_data_->get_motion_cmd_queue()->push_command(g04_cmd);

        // 等待命令被接收
        if (!TaskHelper::WaitforCmdTobeAccepted(g04_cmd, 1000)) {
            _abort("abort: start g04 failed, cmd not accepted by motion or "
                   "timeout");
            break;
        }

        _switch_to_state(State::Running);
        break;
    }
    case GCodeTaskType::PauseCommand: {
        auto m00_cmd =
            std::make_shared<move::MotionCommandAutoM00FakePauseTask>();

        shared_core_data_->get_motion_cmd_queue()->push_command(m00_cmd);

        // 等待命令被接收
        if (!TaskHelper::WaitforCmdTobeAccepted(m00_cmd, 1000)) {
            _abort("abort: start m00 failed, cmd not accepted by motion or "
                   "timeout");
            break;
        }

        _switch_to_state(State::Running);
        break;
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    case GCodeTaskType::DrillMotionCommand: {
        auto drill_gcode =
            std::static_pointer_cast<GCodeTaskDrillMotion>(curr_gcode);

        // check soft limit of drill axis
        double depth_blu =
            util::UnitConverter::um2blu(drill_gcode->start_params().depth_um);
        double current_drill_axis_blu =
            local_info_cache_.curr_cmd_axis_blu[EDM_DRILL_S_AXIS_IDX];
        double target_drill_axis_blu = current_drill_axis_blu - depth_blu;

        if (target_drill_axis_blu <
            shared_core_data_->get_coord_system()
                ->get_neg_soft_limit()[EDM_DRILL_S_AXIS_IDX]) {
            _abort("abort: drill depth out of soft limit");
            break;
        }

        // make cmd and send to motion_cmd_queue
        auto drill_cmd =
            std::make_shared<move::MotionCommandAutoStartDrillMove>(
                drill_gcode->start_params());

        shared_core_data_->get_motion_cmd_queue()->push_command(drill_cmd);

        // 等待命令被接收
        if (!TaskHelper::WaitforCmdTobeAccepted(drill_cmd, 1000)) {
            _abort("abort: start drill failed, cmd not accepted by motion or "
                   "timeout");
            break;
        }

        _drill_record_data(true); // record data start

        _switch_to_state(State::Running);
        break;
    }
#endif
    case GCodeTaskType::G01GroupMotionCommand: {
        auto g01group_gcode =
            std::static_pointer_cast<GCodeTaskG01GroupMotion>(curr_gcode);

        move::G01GroupStartParam start_param;

        // check coord index
        if (!shared_core_data_->get_coord_system()->exist_coordinate_index(
                g01group_gcode->coord_index())) {
            _abort(EDM_FMT::format("abort: g01 group coord index not exist: {}",
                                   g01group_gcode->coord_index()));
            return;
        }

        // motor start pos
        const auto &motor_start_pos = local_info_cache_.curr_cmd_axis_blu;

        // mach start pos
        move::axis_t mach_start_pos;
        shared_core_data_->get_coord_system()->get_cm().motor_to_machine(
            motor_start_pos, mach_start_pos);

        for (int i = 0; i < g01group_gcode->points().size(); ++i) {
            const auto &point = g01group_gcode->points()[i];

            move::axis_t mach_target_pos = mach_start_pos;

            move::G01GroupItem item;
            item.line = point.line_number;

            move::MotionUtils::ClearAxis(item.incs);

            const auto &cmd_values = point.cmd_values;
            if (point.coord_mode == GCodeCoordinateMode::IncrementMode) {
                // inc
                for (std::size_t i = 0; i < EDM_AXIS_NUM; ++i) {
                    if (cmd_values[i]) {
                        mach_target_pos[i] +=
                            util::UnitConverter::mm2blu(*(cmd_values[i]));
                        item.incs[i] =
                            util::UnitConverter::mm2blu(*(cmd_values[i]));
                    }
                }
            } else {
                // abs
                uint32_t coord_index = g01group_gcode->coord_index();

                // 先将输入的机床坐标起点转化为坐标系坐标起点
                move::axis_t coord_start_pos;
                bool ret1 = shared_core_data_->get_coord_system()
                                ->get_cm()
                                .machine_to_coord(coord_index, mach_start_pos,
                                                  coord_start_pos);
                if (!ret1) {
                    _abort(EDM_FMT::format(
                        "abort: g01 group machine_to_coord failed: {}",
                        coord_index));
                    return;
                }

                // 根据cmd_values设定coord_target_pos
                move::axis_t coord_target_pos;
                for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                    if (cmd_values[i]) {
                        coord_target_pos[i] =
                            util::UnitConverter::mm2blu(*(cmd_values[i]));
                    } else {
                        coord_target_pos[i] = coord_start_pos[i];
                    }
                }

                // 再转化为 MachTargetPos
                bool ret2 = shared_core_data_->get_coord_system()
                                ->get_cm()
                                .coord_to_machine(coord_index, coord_target_pos,
                                                  mach_target_pos);
                if (!ret2) {
                    _abort(EDM_FMT::format(
                        "abort: g01 group coord_to_machine failed: {}",
                        coord_index));
                    return;
                }

                for (std::size_t i = 0; i < EDM_AXIS_NUM; ++i) {
                    item.incs[i] = mach_target_pos[i] - mach_start_pos[i];

                    if (std::abs(item.incs[i]) < 0.000001) {
                        item.incs[i] = 0.0;
                    }
                }
            }

            if (move::MotionUtils::IsAxisTheSame(mach_start_pos,
                                                 mach_target_pos)) {
                s_logger->warn("g01 group warn: start and target the same");
                continue;
            }

            // 根据mach_target_pos, 判断软限位
            auto curr_dir = move::MotionUtils::CalcAxisUnitVector(
                mach_start_pos, mach_target_pos);
            auto sl_check_ret = TaskHelper::CheckPosandnegSoftLimit(
                shared_core_data_->get_coord_system(), mach_target_pos,
                curr_dir);
            if (!sl_check_ret) {
                _abort(EDM_FMT::format("abort: g01 group softlimit reached"));
                return;
            }

            start_param.items.push_back(item);
            mach_start_pos = mach_target_pos;
        }

        auto g01group_cmd =
            std::make_shared<move::MotionCommandAutoG01Group>(start_param);

        shared_core_data_->get_motion_cmd_queue()->push_command(g01group_cmd);

        // 等待命令被接收
        if (!TaskHelper::WaitforCmdTobeAccepted(g01group_cmd, 1000)) {
            _abort(
                "abort: start g01 group failed, cmd not accepted by motion or "
                "timeout");
            break;
        }

        _switch_to_state(State::Running);
        break;
    }

    default:
        s_logger->error("GCodeRunner: unknown gcode type: {}",
                        (int)curr_gcode->type());
        assert(false);
        break;
    }
}

void GCodeRunner::_state_running() {
    auto curr_gcode = gcode_list_[curr_gcode_num_];

    if (curr_gcode->timer().state() == TaskTimer::State::Idle) {
        s_logger->warn("in _state_running: 启动计时器 (from idle), gcode_num: {}, line: {}, code: {}",
                        curr_gcode_num_, curr_gcode->line_number(),
                        curr_gcode->get_gcode_str());
        curr_gcode->timer().restart(); // 启动计时器
    } else if (curr_gcode->timer().state() == TaskTimer::State::Paused) {
        s_logger->warn("in _state_running: 启动计时器 (from paused), gcode_num: {}, line: {}, code: {}",
                        curr_gcode_num_, curr_gcode->line_number(),
                        curr_gcode->get_gcode_str());
        curr_gcode->timer().resume(); // 恢复计时器
    }

    if (!curr_gcode->is_motion_task()) {
        // 非Motion GCode, 在此处执行
        _state_running_non_motion(curr_gcode);
        return;
    }

    if (curr_gcode->type() == GCodeTaskType::G01GroupMotionCommand) {
        emit this->sig_autogcode_switched_to_line(
            local_info_cache_.sub_line_number);
    }

    // Motion GCode, 此处进行状态轮训, 转换状态
    switch (local_info_cache_.main_mode) {
    case move::MotionMainMode::Auto: {
        switch (local_info_cache_.auto_state) {
        case move::MotionAutoState::Paused:
            _switch_to_state(State::Paused);
            delay_pause_flag_ = false;
            emit sig_auto_paused();
            break;
        default:
            break;
        }
        break;
    }
    case move::MotionMainMode::Idle: {
        // 表明当前gcode执行完毕, 与AutoStopped都可

        //! 检查G00接触感知报警
        if (curr_gcode->type() == GCodeTaskType::G00MotionCommand) {
            auto g00_gcode =
                std::static_pointer_cast<GCodeTaskG00Motion>(curr_gcode);
            if (local_info_cache_.TouchWarning()) {
                if (g00_gcode->is_touch_motion()) {
                    s_logger->trace("touch over");
                    // 清错
                    auto ack_cmd = std::make_shared<
                        move::MotionCommandSettingClearWarning>(0);
                    this->shared_core_data_->get_motion_cmd_queue()
                        ->push_command(ack_cmd);

                    auto ret = TaskHelper::WaitforCmdTobeAccepted(ack_cmd, 200);
                    if (!ret) {
                        _abort("Clear Warning Failed");
                        break;
                    }
                } else {
                    _abort("***Touch Warning !!");
                    break;
                }
            }
        }

        _check_to_next_gcode();
        break;
    }
    case move::MotionMainMode::Manual:
        _abort("_state_running err: detected MM=Manual");
        return;
    default:
        assert(false);
        break;
    }
}

void GCodeRunner::_state_running_non_motion(GCodeTaskBase::ptr curr_gcode) {
    switch (curr_gcode->type()) {
    case GCodeTaskType::ProgramEndCommand: {
        s_logger->debug("reach M02, GCodeRunner exit.");
        _end(); //! emit auto_stopped inside
        break;
    }
    case GCodeTaskType::CoordinateIndexCommand: {
        auto ci_gcode =
            std::static_pointer_cast<GCodeTaskCoordinateIndex>(curr_gcode);

        if (!this->shared_core_data_->get_coord_system()
                 ->exist_coordinate_index(ci_gcode->coord_index())) {
            _abort(EDM_FMT::format("abort: coord index not exist: {}",
                                   ci_gcode->coord_index()));
            break;
        }

        // 触发让坐标系panel改变当前坐标系
        emit sig_switch_coordindex(ci_gcode->coord_index());

        _check_to_next_gcode();
        break;
    }
    case GCodeTaskType::EleparamSetCommand: {
        auto es_gcode =
            std::static_pointer_cast<GCodeTaskEleparamSet>(curr_gcode);

        if (!this->shared_core_data_->get_power_manager()->exist_eleparam_index(
                es_gcode->eleparam_index())) {
            _abort(EDM_FMT::format("abort: ele param not exist: {}",
                                   es_gcode->eleparam_index()));
            break;
        }

        bool set_ret =
            this->shared_core_data_->get_power_manager()
                ->set_current_eleparam_index(es_gcode->eleparam_index());

        if (!set_ret) {
            _abort(EDM_FMT::format("abort: ele param set failed: {}",
                                   es_gcode->eleparam_index()));
            break;
        }

        // 紧接着下一个
        _check_to_next_gcode();
        break;
    }
    case GCodeTaskType::CoordSetZeroCommand: {
        auto csz_gcode =
            std::static_pointer_cast<GCodeTaskCoordSetZeroCommand>(curr_gcode);

        auto current_coord_index = this->shared_core_data_->get_coord_system()
                                       ->get_current_coord_index();

        auto original_coord_offset_opt =
            this->shared_core_data_->get_coord_system()
                ->get_current_coord_offset();
        if (!original_coord_offset_opt) {
            _abort(EDM_FMT::format(
                "abort: CoordSetZeroCommand: original offset get failed"));
            break;
        }

        auto original_coord_offset = *original_coord_offset_opt;
        auto new_coord_offset = original_coord_offset;
        const auto &set_zero_axis_list = csz_gcode->set_zero_axis_list();
        for (int i = 0; i < coord::Coordinate::Size; ++i) {
            if (set_zero_axis_list[i]) {
                new_coord_offset[i] = local_info_cache_.curr_cmd_axis_blu[i];
            }
        }

        auto set_ret = this->shared_core_data_->get_coord_system()
                           ->set_current_coord_offset(new_coord_offset);
        if (!set_ret) {
            _abort(
                EDM_FMT::format("abort: CoordSetZeroCommand: set zero failed"));
            break;
        }

        emit sig_coord_offset_changed(); // TODO

        // 紧接着下一个
        _check_to_next_gcode();
        break;
    }
    case GCodeTaskType::CoordinateModeCommand:
    case GCodeTaskType::FeedSpeedSetCommand: {
        // 无作用的GCodeNode
        _check_to_next_gcode();
        break;
    }

    default:
        assert(false);
        break;
    }
}

void GCodeRunner::_state_waiting_for_paused() {
    auto curr_gcode = gcode_list_[curr_gcode_num_];
    assert(curr_gcode->is_motion_task());

    switch (local_info_cache_.main_mode) {
    case move::MotionMainMode::Auto: {
        switch (local_info_cache_.auto_state) {
        case move::MotionAutoState::Paused:
            _switch_to_state(State::Paused);
            delay_pause_flag_ = false;
            emit sig_auto_paused();
            break;
        case move::MotionAutoState::Pausing:
            break;
        default:
            _abort(
                EDM_FMT::format("_state_waiting_for_paused err: detected MM={}",
                                move::AutoTaskRunner::GetAutoStateStr(
                                    local_info_cache_.auto_state)));
            break;
        }
        break;
    }
    case move::MotionMainMode::Idle: {
        _abort("_state_waiting_for_paused err: detected MM=Idle");
        break;
    }
    case move::MotionMainMode::Manual:
        _abort("_state_waiting_for_paused err: detected MM=Manual");
        return;
    default:
        assert(false);
        break;
    }
}

void GCodeRunner::_state_waiting_for_resumed() {
    auto curr_gcode = gcode_list_[curr_gcode_num_];
    assert(curr_gcode->is_motion_task());

    switch (local_info_cache_.main_mode) {
    case move::MotionMainMode::Auto: {
        switch (local_info_cache_.auto_state) {
        case move::MotionAutoState::NormalMoving:
            _switch_to_state(State::Running);
            emit sig_auto_resumed();
            break;
        case move::MotionAutoState::Stopping: // Maybe it's stopping/stopped
                                              // after resumed
        case move::MotionAutoState::Stopped:
            _switch_to_state(
                State::Running); // still goto Running State, and check to next
                                 // gcode in the running state
            emit sig_auto_resumed();
            break;
        case move::MotionAutoState::Paused:
            // May be paused move recovering
            s_logger->debug("_state_waiting_for_resumed, still paused");
            break;
        case move::MotionAutoState::Resuming:
            break;
        default:
            _abort(EDM_FMT::format(
                "_state_waiting_for_resumed err: detected MM={}",
                move::AutoTaskRunner::GetAutoStateStr(
                    local_info_cache_.auto_state)));
            break;
        }
        break;
    }
    case move::MotionMainMode::Idle: {
        // Maybe this gcode has run over after resumed ...
        _switch_to_state(State::Running); // still goto Running State, and check
                                          // to next gcode in the running state
        emit sig_auto_resumed();
        break;
    }
    case move::MotionMainMode::Manual:
        _abort("_state_waiting_for_resumed err: detected MM=Manual");
        return;
    default:
        assert(false);
        break;
    }
}

void GCodeRunner::_state_waiting_for_stopped() {
    // 这里意味着这是外界用户人为输入了停止命令, 结束整个G代码
    auto curr_gcode = gcode_list_[curr_gcode_num_];
    assert(curr_gcode->is_motion_task());

    switch (local_info_cache_.main_mode) {
    case move::MotionMainMode::Auto: {
        switch (local_info_cache_.auto_state) {
        case move::MotionAutoState::Stopping:
            break;
        case move::MotionAutoState::Stopped: {
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
            if (curr_gcode->type() == GCodeTaskType::DrillMotionCommand) {
                _drill_record_data(false); // stop record data2
            }
#endif

            _end(); //! emit auto stopped inside
            break;
        }
        default:
            _abort(EDM_FMT::format(
                "_state_waiting_for_stopped err: detected MM={}",
                move::AutoTaskRunner::GetAutoStateStr(
                    local_info_cache_.auto_state)));
            break;
        }
        break;
    }
    case move::MotionMainMode::Idle: {
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
        if (curr_gcode->type() == GCodeTaskType::DrillMotionCommand) {
            _drill_record_data(false); // stop record data2
        }
#endif

        _end(); //! emit auto stopped inside
        break;
    }
    case move::MotionMainMode::Manual:
        _abort("_state_waiting_for_stopped err: detected MM=Manual");
        return;
    default:
        assert(false);
        break;
    }
}

const char *GCodeRunner::GetStateStr(State s) {
    switch (s) {
#define XX_(s__)     \
    case State::s__: \
        return #s__;

        XX_(ReadyToStart)
        XX_(CurrentNodeIniting)
        XX_(Running)
        XX_(WaitingForPaused)
        XX_(Paused)
        XX_(WaitingForResumed)
        XX_(WaitingForStopped)
        XX_(Stopped)

#undef XX_
    default:
        return "Unknow";
    }
}

void GCodeRunner::_switch_to_state(State new_state) {
    s_logger->trace("GCodeRunner State: {} -> {}", GetStateStr(state_),
                    GetStateStr(new_state));

    // test
    // 根据 state_ 和 new_state 来动态调整定时器周期, 更快速响应

    auto f_should_fast_peroid = [&]() -> bool {
        if (new_state == State::CurrentNodeIniting ||
            new_state == State::ReadyToStart ||
            new_state == State::WaitingForPaused ||
            new_state == State::WaitingForResumed ||
            new_state == State::WaitingForStopped) {
            return true;
        }

        if (new_state == State::Running) {
            if (curr_gcode_num_ >= 0 && curr_gcode_num_ < gcode_list_.size() &&
                gcode_list_[curr_gcode_num_]) {
                return !gcode_list_[curr_gcode_num_]->is_motion_task();
            }
        }

        return false;
    };

    if (f_should_fast_peroid()) {
        update_timer_->setInterval(update_timer_fast_peroid_ms_);
    } else {
        update_timer_->setInterval(update_timer_regular_peroid_ms_);
    }

    state_ = new_state;
}

} // namespace task

} // namespace edm
