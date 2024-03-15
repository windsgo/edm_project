#include "TaskManager.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace task {

TaskManager::TaskManager(edm::app::SharedCoreData *shared_core_data,
                         QObject *parent)
    : QObject(parent), shared_core_data_(shared_core_data),
      info_dispatcher_(shared_core_data->get_info_dispatcher()) {

    _init_state();
}

bool TaskManager::operation_start_pointmove(
    std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd) {
    switch (state_) {
    case State::Normal:
        _cmd_start_pointmove(cmd);
        return true;
    case State::AutoGCode: {
        if (!_is_info_mainmode_auto() || !_is_info_autostate_paused()) {
            _error("TaskManager: start pm failed, auto and not paused");
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
            _error("gcode_list empty");
            return false;
        }

        if (!_check_gcode_list_at_first()) {
            return false;
        }

        _init_state();

        // copy the gcode list
        gcode_list_ = gcode_list;

        // 初始化相关状态量
        curr_gcode_num_ = 0;
        gcodeauto_stop_flag_ = false;

        // 这里assume gcodelist中都不为空指针, 都有效

        // 初始化第一条GCode
        _autogcode_init_curr_gcode();

        // 切换状态
        s_logger->info("start gcode success.");
        _state_switchto(State::AutoGCode);
        emit sig_auto_started();
        return true;
    }

    case State::AutoGCode: {
        _error("operation_gcode_start failed, already AutoGCode state");
        return false;
    }

    default:
        return false;
    }
}

bool TaskManager::operation_gcode_pause() {
    _cmd_auto_pause();

    return true;
}

bool TaskManager::operation_gcode_resume() {
    _cmd_auto_resume();

    return true;
}

bool TaskManager::operation_gcode_stop() {
    _cmd_auto_stop();

    gcodeauto_stop_flag_ = true; // 置true

    return true;
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

void TaskManager::_autogcode_set_end() {
    // 先不区分abort和普通结束了

    _state_switchto(State::Normal);
    gcode_list_.clear();
    curr_gcode_num_ = -1;
    gcodeauto_stop_flag_ = false;

    // 发送全部gcode的结束信号
    emit sig_auto_stopped();
}

bool TaskManager::_check_gcode_list_at_first() {

    for (const auto &g : gcode_list_) {
        // 判断电参数号是否存在
        if (g->type() == GCodeTaskType::EleparamSetCommand) {
            auto eg = std::static_pointer_cast<GCodeTaskEleparamSet>(g);
            auto exist = this->shared_core_data_->get_power_manager()
                             ->exist_eleparam_index(eg->eleparam_index());
            if (!exist) {
                _error(EDM_FMT::format("check err: eleparam not exist: {}",
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
                s_logger->error(EDM_FMT::format(
                    "check err: coord index not exist: {}", cg->coord_index()));
                return false;
            }
        }
    }

    // 判断最后一个是不是M02

    if (gcode_list_.back()->type() != GCodeTaskType::ProgramEndCommand) {
        _error("check err: last gcode is not m02");
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
        _autogcode_set_end();
        break;
    case GCodeTaskType::CoordinateIndexCommand: {
        auto coord_index_task =
            std::static_pointer_cast<GCodeTaskCoordinateIndex>(task);

        if (!this->shared_core_data_->get_coord_system()
                 ->exist_coordinate_index(coord_index_task->coord_index())) {
            _error(EDM_FMT::format("abort: coord index not exist: {}",
                                   coord_index_task->coord_index()));
            _autogcode_set_end();
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
            _error(EDM_FMT::format("abort: ele param not exist: {}",
                                   ele_set_task->eleparam_index()));
            _autogcode_set_end();
            break;
        }

        bool set_ret =
            this->shared_core_data_->get_power_manager()
                ->set_current_eleparam_index(ele_set_task->eleparam_index());

        if (!set_ret) {
            _error(EDM_FMT::format("abort: ele param set failed: {}",
                                   ele_set_task->eleparam_index()));
            _autogcode_set_end();
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
        // TODO
        break;
    }
    case GCodeTaskType::G01MotionCommand: {
        // TODO
        break;
    }
    case GCodeTaskType::DelayCommand:
        // TODO
        s_logger->warn("do not support DelayCommand yer");
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
        _autogcode_set_end();
        return;
    }

    _autogcode_init_curr_gcode();
}

void TaskManager::_error(std::string_view error_str) {
    last_error_message_ = error_str;
    s_logger->error(last_error_message_);
}

bool TaskManager::_is_info_mainmode_idle() const {
    return info_dispatcher_->get_info().main_mode ==
    move::MotionMainMode::Idle;
}

bool TaskManager::_is_info_mainmode_auto() const {
    return info_dispatcher_->get_info().main_mode ==
    move::MotionMainMode::Auto;
}

bool TaskManager::_is_info_autostate_paused() const {
    return info_dispatcher_->get_info().auto_state ==
           move::MotionAutoState::Paused;
}

bool TaskManager::_is_info_autostate_stopped() const {
    return info_dispatcher_->get_info().auto_state ==
           move::MotionAutoState::Stopped;
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
            _autogcode_set_end();
            break;
        }

        // 如果是G00, 检查是否是因为接触感知而停止
        auto curr_task = gcode_list_[curr_gcode_num_];
        if (curr_task->type() == GCodeTaskType::G00MotionCommand) {
            if (info_dispatcher_->get_info().TouchWarning()) {
                _error("g00 exit due to Touch Warning. Exit whole gcode.");
                _autogcode_set_end();
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

void TaskManager::_cmd_auto_pause() {
    // create motion cmd
    auto auto_pause_command =
        std::make_shared<edm::move::MotionCommandAutoPause>();
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_pause_command);
}

void TaskManager::_cmd_auto_resume() {
    // create motion cmd
    auto auto_resume_command =
        std::make_shared<edm::move::MotionCommandAutoResume>();
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_resume_command);
}

void TaskManager::_cmd_auto_stop() {
    // create motion cmd
    auto auto_stop_command =
        std::make_shared<edm::move::MotionCommandAutoStop>(false);
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        auto_stop_command);
}

} // namespace task

} // namespace edm
