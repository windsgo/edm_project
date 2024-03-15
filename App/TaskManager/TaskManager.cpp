#include "TaskManager.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {

namespace task {

TaskManager::TaskManager(edm::app::SharedCoreData *shared_core_data)
    : QObject(nullptr), shared_core_data_(shared_core_data),
      info_dispatcher_(shared_core_data->get_info_dispatcher()) {
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &TaskManager::_timer_slot);
    timer_->start(1000);

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
    // create motion cmd
    auto stop_pointmove_command =
        std::make_shared<edm::move::MotionCommandManualStopPointMove>(false);
    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        stop_pointmove_command);

    return true;
}

bool TaskManager::operation_gcode_start(
    const std::vector<GCodeTaskBase::ptr> &gcode_list) {
    switch (state_) {
    case State::Normal: {
        if (gcode_list.empty()) {
            return false;
        }

        if (!_check_gcode_list_at_first()) {
            s_logger->error("_check_gcode_list_at_first failed");
            return false;
        }

        // set to autogcode

        gcode_list_ = gcode_list; // copy the gcode list

        // 初始化相关状态量
        curr_gcode_num_ = 0;
        gcodeauto_stop_flag_ = false;

        // 这里assume gcodelist中都不为空指针, 都有效

        // 发出第一条GCode
        _autogcode_check_to_gcode(0);

        // 切换状态
        state_ = State::AutoGCode;
        emit sig_auto_started();
        return true;
    }

    case State::AutoGCode: {
        return false;
    }

    default:
        return false;
    }
}

void TaskManager::_init_state() {
    state_ = State::Normal;

    gcode_list_.clear();
    curr_gcode_num_ = -1;
    gcodeauto_stop_flag_ = false;
}

void TaskManager::_timer_slot() {
    switch (state_) {
    case State::Normal:
        // Do nothing
        break;
    case State::AutoGCode:
        _autogcode_state_run_once();
        break;
    default:
        s_logger->error("{}: unknow state_: {}", __PRETTY_FUNCTION__,
                        (int)state_);
        break;
    }
}

void TaskManager::_autogcode_state_run_once() {
    // TODO
    auto curr_task = gcode_list_[curr_gcode_num_];
    switch (curr_task->type()) {
        //! 要检查是否有stop_flag在本地, 如果有的话, 结束之后直接停掉
    case GCodeTaskType::G00MotionCommand:
        // 检查Motion状态, 如果已经Stopped, 处理下一个gcode
        // 如果Stopped的时候, 接触感知警报存在, 直接停止
        break;
    case GCodeTaskType::G01MotionCommand:
    case GCodeTaskType::PauseCommand:
    case GCodeTaskType::DelayCommand:
        // 检查Motion状态, 如果已经Stopped, 处理下一个gcode
        break;
    case GCodeTaskType::CoordinateIndexCommand:
    case GCodeTaskType::EleparamSetCommand:
        // 直接切换下一个, 这些gcode的任务在上一个 check_to_gcode 的初始化中就执行了
        ++curr_gcode_num_;
        _autogcode_check_to_gcode(curr_gcode_num_);
        break;
    case GCodeTaskType::ProgramEndCommand: // M02
        // M02 这里退出, 也可以在初始化M02这个node的时候退出
        _state_switchto(State::Normal);
        _init_state();
        emit sig_auto_stopped();
        break;
    }
}

bool TaskManager::_check_gcode_list_at_first() {
    // TODO

    // 判断电参数号是否存在
    // TODO

    return true;
}

void TaskManager::_autogcode_check_to_gcode(int next_gcode_num) {
    if (next_gcode_num >= gcode_list_.size()) {
        // 给出的 下一个gcode号 不存在
        // 切换到Normal模式
        _state_switchto(State::Normal);
        _init_state();
        emit sig_auto_stopped();
        return;
    }

    auto task = gcode_list_[next_gcode_num];
    // 根据不同的type, 有些直接在原地处理, 有些发送给MotionThread
    switch (task->type()) {
        // TODO
    }
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

void TaskManager::_state_switchto(State new_state) {
    s_logger->trace("TaskManager: state: {} -> {}", (int)state_, (int)new_state);

    state_ = new_state;
}

void TaskManager::_cmd_start_pointmove(
    std::shared_ptr<edm::move::MotionCommandManualStartPointMove> cmd) {
    this->shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
}

} // namespace task

} // namespace edm
