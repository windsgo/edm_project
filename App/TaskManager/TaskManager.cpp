#include "TaskManager.h"
#include "Logger/LogMacro.h"

#include <QCoreApplication>
#include <QDateTime>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace task {

TaskManager::TaskManager(edm::app::SharedCoreData *shared_core_data,
                         QObject *parent)
    : QObject(parent), shared_core_data_(shared_core_data),
      info_dispatcher_(shared_core_data->get_info_dispatcher()) {

    _init_state();
    _init_connections();
}

bool TaskManager::operation_start_pointmove(
    std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd) {
    switch (state_) {
    case State::Normal:
        _cmd_start_pointmove(cmd);
        return true;
    case State::AutoGCode: {
        if (!_is_info_mainmode_auto() || !_is_info_autostate_paused()) {
            s_logger->error("TaskManager: start pm failed, auto and not paused");
            return false;
        }

        // 处于auto暂停情况
        _cmd_start_pointmove(cmd);

        return true;
    }

    default:
        return false;
    }
}

bool TaskManager::operation_stop_pointmove() {
    _cmd_stop_pointmove();

    return true;
}

bool TaskManager::operation_gcode_start(
    const std::vector<GCodeTaskBase::ptr> &gcode_list) {
    switch (state_) {
    case State::Normal: {
        if (gcode_list.empty()) {
            _autogcode_abort("gcode_list empty");
            return false;
        }

        _init_state();

        // copy the gcode list
        gcode_list_ = gcode_list;

        if (!_check_gcode_list_at_first()) {
            return false;
        }

        // 初始化相关状态量
        curr_gcode_num_ = 0;
        gcodeauto_stop_flag_ = false;

        s_logger->info("start gcode success.");

        // 这里assume gcodelist中都不为空指针, 都有效

        // 切换状态
        _state_switchto(State::AutoGCode);

        // 初始化第一条GCode
        _autogcode_init_curr_gcode();

        emit sig_auto_started();
        return true;
    }

    case State::AutoGCode: {
        s_logger->error("operation_gcode_start failed, already AutoGCode state");
        return false;
    }

    default:
        return false;
    }
}

bool TaskManager::operation_gcode_pause() { return _cmd_auto_pause(); }

bool TaskManager::operation_gcode_resume() { return _cmd_auto_resume(); }

bool TaskManager::operation_gcode_stop() {
    auto ret = _cmd_auto_stop();

    if (ret) {
        gcodeauto_stop_flag_ = true; // 置true
    }

    return ret;
}

bool TaskManager::operation_emergency_stop() {
    auto ret = _cmd_emergency_stop();

    if (ret) {
        _init_state();
    }

    return ret;
}

void TaskManager::_init_state() {
    state_ = State::Normal;

    last_error_message_.clear();

    gcode_list_.clear();
    curr_gcode_num_ = -1;
    gcodeauto_stop_flag_ = false;
}

void TaskManager::_init_connections() {
    connect(info_dispatcher_, &InfoDispatcher::sig_auto_stopped, this,
            &TaskManager::_slot_auto_stopped);

    connect(info_dispatcher_, &InfoDispatcher::sig_auto_paused, this,
            &TaskManager::_slot_auto_paused);

    connect(info_dispatcher_, &InfoDispatcher::sig_auto_started, this,
            &TaskManager::_slot_auto_started);

    connect(info_dispatcher_, &InfoDispatcher::sig_auto_resumed, this,
            &TaskManager::_slot_auto_resumed);

    // 转发点动开始停止信号
    connect(info_dispatcher_, &InfoDispatcher::sig_manual_pointmove_started,
            this, &TaskManager::sig_manual_pointmove_started);
    connect(info_dispatcher_, &InfoDispatcher::sig_manual_pointmove_stopped,
            this, &TaskManager::sig_manual_pointmove_stopped);
}

void TaskManager::_autogcode_normal_end() {
    _state_switchto(State::Normal);
    gcode_list_.clear();
    curr_gcode_num_ = -1;
    gcodeauto_stop_flag_ = false;

    // 发送全部gcode的结束信号
    emit sig_auto_stopped(false);
}

void TaskManager::_autogcode_abort(std::string_view error_str) {
    _state_switchto(State::Normal);
    gcode_list_.clear();
    curr_gcode_num_ = -1;
    gcodeauto_stop_flag_ = false;

    last_error_message_ = error_str;

    // 发送全部gcode的结束信号, 指明aborted
    emit sig_auto_stopped(true);
}

bool TaskManager::_check_gcode_list_at_first() {

    for (const auto &g : gcode_list_) {
        // 判断电参数号是否存在
        if (g->type() == GCodeTaskType::EleparamSetCommand) {
            auto eg = std::static_pointer_cast<GCodeTaskEleparamSet>(g);
            auto exist = this->shared_core_data_->get_power_manager()
                             ->exist_eleparam_index(eg->eleparam_index());
            if (!exist) {
                _autogcode_abort(EDM_FMT::format(
                    "check err: eleparam not exist: {}", eg->eleparam_index()));
                return false;
            }
        }
        // 检查坐标系序号是否存在
        else if (g->type() == GCodeTaskType::CoordinateIndexCommand) {
            auto cg = std::static_pointer_cast<GCodeTaskCoordinateIndex>(g);
            auto exist = this->shared_core_data_->get_coord_system()
                             ->exist_coordinate_index(cg->coord_index());
            if (!exist) {
                _autogcode_abort(EDM_FMT::format(
                    "check err: coord index not exist: {}", cg->coord_index()));
                return false;
            }
        }
    }

    // 判断最后一个是不是M02

    if (gcode_list_.back()->type() != GCodeTaskType::ProgramEndCommand) {
        _autogcode_abort("check err: last gcode is not m02");
        return false;
    }

    return true;
}

void TaskManager::_autogcode_init_curr_gcode() {
    // 根据不同的type, 有些直接在原地处理, 有些发送给MotionThread
    auto task = gcode_list_[curr_gcode_num_];

    switch (task->type()) {
    case GCodeTaskType::ProgramEndCommand:
        // M02, 结束
        s_logger->info("reach m02, gcode exit.");
        last_error_message_.clear();
        _autogcode_normal_end();
        break;
    case GCodeTaskType::CoordinateIndexCommand: {
        auto coord_index_task =
            std::static_pointer_cast<GCodeTaskCoordinateIndex>(task);

        if (!this->shared_core_data_->get_coord_system()
                 ->exist_coordinate_index(coord_index_task->coord_index())) {
            _autogcode_abort(EDM_FMT::format("abort: coord index not exist: {}",
                                             coord_index_task->coord_index()));
            break;
        }

        // 触发让坐标系panel改变当前坐标系
        emit sig_switch_coordindex(coord_index_task->coord_index());

        // 紧接着下一个
        _autogcode_check_to_next_gcode();
        break;
    }
    case GCodeTaskType::EleparamSetCommand: {
        auto ele_set_task =
            std::static_pointer_cast<GCodeTaskEleparamSet>(task);

        if (!this->shared_core_data_->get_power_manager()->exist_eleparam_index(
                ele_set_task->eleparam_index())) {
            _autogcode_abort(EDM_FMT::format("abort: ele param not exist: {}",
                                             ele_set_task->eleparam_index()));
            break;
        }

        bool set_ret =
            this->shared_core_data_->get_power_manager()
                ->set_current_eleparam_index(ele_set_task->eleparam_index());

        if (!set_ret) {
            _autogcode_abort(EDM_FMT::format("abort: ele param set failed: {}",
                                             ele_set_task->eleparam_index()));
            break;
        }

        // 紧接着下一个
        _autogcode_check_to_next_gcode();
        break;
    }
    case GCodeTaskType::CoordinateModeCommand:
    case GCodeTaskType::FeedSpeedSetCommand:
    default:
        s_logger->warn("should not have this gcode: type: {}",
                       int(task->type()));
        _autogcode_check_to_next_gcode();
        break;
    case GCodeTaskType::G00MotionCommand: {

        auto g00_task = std::static_pointer_cast<GCodeTaskG00Motion>(task);

        // check coord index
        if (!shared_core_data_->get_coord_system()->exist_coordinate_index(
                g00_task->coord_index())) {
            _autogcode_abort(
                EDM_FMT::format("abort: g00 coord index not exist: {}",
                                g00_task->coord_index()));
            break;
        }

        // motor start pos
        const auto &motor_start_pos = shared_core_data_->get_info_dispatcher()
                                          ->get_info()
                                          .curr_cmd_axis_blu;

        // mach start pos
        move::axis_t mach_start_pos;
        shared_core_data_->get_coord_system()->get_cm().motor_to_machine(
            motor_start_pos, mach_start_pos);

        assert(g00_task->cmd_values().size() == 6);

        // 根据增量/绝对模式给出 机床坐标系目标位置
        move::axis_t mach_target_pos{0.0};
        const auto &cmd_values = g00_task->cmd_values();
        if (g00_task->coord_mode() == GCodeCoordinateMode::IncrementMode) {
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
            uint32_t coord_index = g00_task->coord_index();

            // 先将输入的机床坐标起点转化为坐标系坐标起点
            move::axis_t coord_start_pos;
            bool ret1 = shared_core_data_->get_coord_system()
                            ->get_cm()
                            .machine_to_coord(coord_index, mach_start_pos,
                                              coord_start_pos);
            if (!ret1) {
                _autogcode_abort(EDM_FMT::format(
                    "abort: g00 machine_to_coord failed: {}", coord_index));
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
                _autogcode_abort(EDM_FMT::format(
                    "abort: g00 coord_to_machine failed: {}", coord_index));
                break;
            }
        }

        if (move::MotionUtils::IsAxisTheSame(mach_start_pos, mach_target_pos)) {
            s_logger->warn("g00 failed, next node : start and target the same");
            _autogcode_check_to_next_gcode();
            break;
        }

        move::axis_t motor_target_pos;
        shared_core_data_->get_coord_system()->get_cm().machine_to_motor(
            mach_target_pos, motor_target_pos);

        // 根据mach_target_pos, 判断软限位
        _check_posandneg_soft_limit(mach_target_pos);

        auto speed = _get_default_speed_param();
        speed.cruise_v =
            util::UnitConverter::mm_min2blu_s(g00_task->feed_speed());

        if (speed.cruise_v <= 0.0)
            speed.cruise_v = 1.0; // for safe

        // send g00 cmd
        auto g00_cmd =
            std::make_shared<move::MotionCommandAutoStartG00FastMove>(
                motor_start_pos, motor_target_pos, speed,
                g00_task->touch_detect_enable());

        shared_core_data_->get_motion_cmd_queue()->push_command(g00_cmd);

        // 等待命令被接收
        if (!_waitfor_cmd_tobe_accepted(g00_cmd, 100)) {
            _autogcode_abort(
                "abort: start g00 failed, cmd not accepted by motion or "
                "timeout");
            break;
        }

        break;
    }
    case GCodeTaskType::G01MotionCommand: {
        // TODO
        s_logger->warn("do not support G01MotionCommand yet");
        _autogcode_check_to_next_gcode();
        break;
    }
    case GCodeTaskType::DelayCommand:
        // TODO
        s_logger->warn("do not support DelayCommand yet");
        _autogcode_check_to_next_gcode();
        break;
    case GCodeTaskType::PauseCommand:
        // TODO
        s_logger->warn("do not support PauseCommand yet");
        _autogcode_check_to_next_gcode();
        break;
    }
}

void TaskManager::_autogcode_check_to_next_gcode() {
    ++curr_gcode_num_;

    auto task = gcode_list_[curr_gcode_num_];

    s_logger->trace("TaskManager: check to next gcode: {}, type: {}",
                    curr_gcode_num_, (int)task->type());

    if (curr_gcode_num_ >= gcode_list_.size()) {
        s_logger->warn("TaskManager: exit gcode, gcode num overflow");
        // 给出的 下一个gcode号 不存在 (这不正常, 正常应当由执行到M2主动结束)
        // 切换到Normal模式
        _autogcode_abort("gcode overflow");
        return;
    }

    _autogcode_init_curr_gcode();
}

bool TaskManager::_waitfor_cmd_tobe_accepted(move::MotionCommandBase::ptr cmd,
                                             int timeout_ms) {
    auto start_t = QDateTime::currentMSecsSinceEpoch();

    while (1) {
        if (cmd->is_accepted() || cmd->is_ignored()) {
            break;
        }

        auto now = QDateTime::currentMSecsSinceEpoch();
        if (now - start_t > timeout_ms) {
            break;
        }

        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    return cmd->is_accepted();
}

bool TaskManager::_is_info_mainmode_idle() const {
    return info_dispatcher_->get_info().main_mode == move::MotionMainMode::Idle;
}

bool TaskManager::_is_info_mainmode_auto() const {
    return info_dispatcher_->get_info().main_mode == move::MotionMainMode::Auto;
}

bool TaskManager::_is_info_autostate_paused() const {
    return info_dispatcher_->get_info().auto_state ==
           move::MotionAutoState::Paused;
}

bool TaskManager::_is_info_autostate_stopped() const {
    return info_dispatcher_->get_info().auto_state ==
           move::MotionAutoState::Stopped;
}

bool TaskManager::_check_posandneg_soft_limit(
    const move::axis_t &mach_target_pos) const {
    const auto &pos_sl =
        shared_core_data_->get_coord_system()->get_pos_soft_limit();
    const auto &neg_sl =
        shared_core_data_->get_coord_system()->get_neg_soft_limit();

    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        if (mach_target_pos[i] > pos_sl[i] || mach_target_pos[i] < neg_sl[i]) {
            return false;
        }
    }

    return true;
}

move::MoveRuntimePlanSpeedInput TaskManager::_get_default_speed_param() const {
    edm::move::MoveRuntimePlanSpeedInput speed;

    speed.nacc = util::UnitConverter::ms2p(
        shared_core_data_->get_system_settings().get_fmparam_nacc_ms());
    speed.acc0 = util::UnitConverter::um2blu(
        shared_core_data_->get_system_settings().get_fmparam_max_acc_um_s2());
    speed.dec0 = -speed.acc0;
    speed.entry_v = 0;
    speed.exit_v = 0;
    speed.cruise_v = 1000; // default, modify outside according to feed speed

    return speed;
}

void TaskManager::_state_switchto(State new_state) {
    s_logger->trace("TaskManager: state: {} -> {}", (int)state_,
                    (int)new_state);

    state_ = new_state;
}

void TaskManager::_slot_auto_stopped() {
    switch (state_) {
    case State::Normal:
        s_logger->warn("TaskManager: recv auto stopped signal at Normal");
        break;
    case State::AutoGCode: {
        s_logger->trace("TaskManager: recv auto stopped signal");

        // 处理停止信号

        if (gcodeauto_stop_flag_) {
            // 用户调用了停止加工 auto stop
            s_logger->info("user set auto stop, stopped");
            _autogcode_abort("user set auto stop, stopped");
            break;
        }

        // 如果是G00, 检查是否是因为接触感知而停止
        auto curr_task = gcode_list_[curr_gcode_num_];
        if (curr_task->type() == GCodeTaskType::G00MotionCommand) {
            if (info_dispatcher_->get_info().TouchWarning()) {
                _autogcode_abort("g00 exit due to Touch Warning. Exit whole gcode.");
                break;
            }
        }

        // 正常切换到下一个G代码
        _autogcode_check_to_next_gcode();

        break;
    }
    default:
        break;
    }
}

void TaskManager::_slot_auto_started() {
    // TODO
    s_logger->trace("TaskManager: recv auto started signal");
    // Do Nothing More
}

void TaskManager::_slot_auto_paused() {
    // TODO
    s_logger->trace("TaskManager: recv auto paused signal");
    emit sig_auto_paused();
    // Do Nothing More
}

void TaskManager::_slot_auto_resumed() {
    // TODO
    s_logger->trace("TaskManager: recv auto resumed signal");
    emit sig_auto_resumed();
    // Do Nothing More
}

void TaskManager::_cmd_start_pointmove(
    std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd) {
    this->shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
}

void TaskManager::_cmd_stop_pointmove() {
    // create motion cmd
    auto stop_pointmove_command =
        std::make_shared<edm::move::MotionCommandManualStopPointMove>(false);
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        stop_pointmove_command);
}

bool TaskManager::_cmd_auto_pause() {
    // create motion cmd
    auto auto_pause_command =
        std::make_shared<edm::move::MotionCommandAutoPause>();
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_pause_command);

    return _waitfor_cmd_tobe_accepted(auto_pause_command, 100);
}

bool TaskManager::_cmd_auto_resume() {
    // create motion cmd
    auto auto_resume_command =
        std::make_shared<edm::move::MotionCommandAutoResume>();
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_resume_command);

    return _waitfor_cmd_tobe_accepted(auto_resume_command, 100);
}

bool TaskManager::_cmd_auto_stop() {
    // create motion cmd
    auto auto_stop_command =
        std::make_shared<edm::move::MotionCommandAutoStop>(false);
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_stop_command);

    return _waitfor_cmd_tobe_accepted(auto_stop_command, 100);
}

bool TaskManager::_cmd_emergency_stop() {
    auto estop_cmd = std::make_shared<edm::move::MotionCommandManualEmergencyStopAllMove>();

    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        estop_cmd);

    return _waitfor_cmd_tobe_accepted(estop_cmd, 100);
}

} // namespace task

} // namespace edm
