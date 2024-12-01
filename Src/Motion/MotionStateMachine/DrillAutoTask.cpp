#include "DrillAutoTask.h"

#include "Logger/LogMacro.h"
#include "Motion/MotionSharedData/MotionSharedData.h"
#include "Utils/UnitConverter/UnitConverter.h"
#include <cassert>
#include <chrono>

EDM_STATIC_LOGGER_NAME(s_logger, "motion");

static auto s_motion_shared = edm::move::MotionSharedData::instance();

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)

#define CURRENT_POS \
    (s_motion_shared->get_global_cmd_axis().at(EDM_DRILL_S_AXIS_IDX))

namespace edm {

namespace move {

DrillAutoTask::DrillAutoTask(const DrillStartParams &start_params,
                             const MotionCallbacks &cbs)
    : AutoTask(AutoTaskType::Drill), start_params_(start_params), cbs_(cbs) {

    // 开始加工, 初始化Runtime
    if (start_params_.touch) {
        // 需要先碰边
        runtime_.touch_start_pos = CURRENT_POS;
        _drillstate_changeto(DrillState::PrepareTouch);
    } else {
        // 不需要碰边, 直接开始加工
        runtime_.mach_start_pos = CURRENT_POS;
        runtime_.mach_target_pos =
            runtime_.mach_start_pos -
            util::UnitConverter::um2blu(start_params_.depth_um);
        _drillstate_changeto(DrillState::PrepareDrill);
    }

    s_motion_shared->set_current_drill_total_blu(
        util::UnitConverter::um2blu(start_params_.depth_um));
}

bool DrillAutoTask::pause() {
    if (drill_state_ == DrillState::Idle) {
        return false;
    }

    pause_flag_ = true;
    _mach_on(false);
    _all_pump_and_spindle_on(false);
    return true;
}

bool DrillAutoTask::resume() {
    if (pause_flag_ != true) {
        // 不是暂停状态
        return false;
    }

    pause_flag_ = false;

    switch (drill_state_) {
    // 根据不同的状态, 打开高频等
    default:
    case DrillState::Idle:
        break;
    case DrillState::PrepareTouch:
    case DrillState::PrepareTouchDelaying:
    case DrillState::Touching:
    case DrillState::TouchedBacking:
        _all_pump_and_spindle_on(true);
        _mach_on(false);
        break;
    case DrillState::PrepareDrill:
    case DrillState::PrepareDrillDelaying:
    case DrillState::Drilling:
        _all_pump_and_spindle_on(true);
        _mach_on(true);
        break;
    case DrillState::PrepareBack:
    case DrillState::PrepareBackDelaying:
    case DrillState::ReturnBacking:
        _all_pump_and_spindle_on(true);
        _mach_on(false);
        break;
    }

    return true;
}

bool DrillAutoTask::stop(bool immediate) {
    (void)immediate;
    _clear_all_status();

    _all_pump_and_spindle_on(false);
    _mach_on(false);
    return true;
}

void DrillAutoTask::_drillstate_idle() {
    // Do Nothing
}

void DrillAutoTask::_drillstate_prepare_touch() {
    _all_pump_and_spindle_on(true);
    _mach_on(false);

    _start_timer(1000);
    _drillstate_changeto(DrillState::PrepareTouchDelaying);
}

void DrillAutoTask::_drillstate_prepare_touch_delaying() {
    if (_is_timer_timeout()) {
        _drillstate_changeto(DrillState::Touching);
    }
}

void DrillAutoTask::_drillstate_touching() {
    if (s_motion_shared->cached_udp_message().touch_detected) {
        s_logger->debug("DrillAutoTask Touch Detected");

        // 碰边完成, 记录碰边位置, 计算加工目标位置
        runtime_.touched_pos = CURRENT_POS;
        runtime_.mach_start_pos =
            runtime_.touched_pos; // 碰边回退后会重新给这个值
        runtime_.mach_target_pos =
            runtime_.mach_start_pos -
            util::UnitConverter::um2blu(start_params_.depth_um);

        s_logger->debug("DrillAutoTask, Touched Pos: {}, Mach Target Pos: {}",
                        runtime_.touched_pos, runtime_.mach_target_pos);

        if (start_params_.breakout) {
            double start_detect_rate =
                s_motion_shared->get_drill_params()
                    .breakout_params.breakout_start_detect_length_percent;
            runtime_.start_detect_pos =
                runtime_.touched_pos -
                start_detect_rate *
                    util::UnitConverter::um2blu(start_params_.depth_um);

            s_logger->debug("DrillAutoTask, rate: {}, Start Detect Pos: {}",
                            start_detect_rate, runtime_.start_detect_pos);

            runtime_.detect_state = 0;
            runtime_.start_breakthrough_detect = 0;
        }

        _drillstate_changeto(DrillState::TouchedBacking);
    }

    // 继续碰边
    double touch_speed_blu_per_peroid = util::UnitConverter::um_ms2blu_p(
        s_motion_shared->get_drill_params().touch_speed_um_ms);

    CURRENT_POS = CURRENT_POS - touch_speed_blu_per_peroid;
}

void DrillAutoTask::_drillstate_touched_backing() {
    double target_back_pos =
        runtime_.touched_pos +
        util::UnitConverter::um2blu(
            s_motion_shared->get_drill_params().touch_return_um);

    CURRENT_POS = CURRENT_POS + 5; // TODO, 目前固定速度回退

    if (CURRENT_POS >= target_back_pos) {
        // 回退完成
        CURRENT_POS = target_back_pos;
        s_logger->debug("DrillAutoTask, Touched Backing Over: {}", CURRENT_POS);
        runtime_.mach_start_pos = CURRENT_POS; // 重新给出mach start pos

        _drillstate_changeto(DrillState::PrepareDrill);
    }
}

void DrillAutoTask::_drillstate_prepare_drill() {
    _all_pump_and_spindle_on(true);
    _mach_on(true);

    _start_timer(1000);
    _drillstate_changeto(DrillState::PrepareDrillDelaying);
}

void DrillAutoTask::_drillstate_prepare_drill_delaying() {
    if (_is_timer_timeout()) {
        _drillstate_changeto(DrillState::Drilling);
    }
}

void DrillAutoTask::_drillstate_drilling() {
    bool drill_over = false;

    double _servo_dis = util::UnitConverter::mm_min2blu_p(
        s_motion_shared->cached_udp_message().servo_calced_speed_mm_min);

    if (start_params_.breakout && false) {
        // TODO
    } else {
        // 正常运行, 非穿透检测
        CURRENT_POS = CURRENT_POS - _servo_dis; // 进给速度为正, 是负方向走
    }

    // 通用位置限制处理
    if (CURRENT_POS >= runtime_.mach_start_pos) {
        CURRENT_POS = runtime_.mach_start_pos; // 防止回退到起点以前
    } else if (CURRENT_POS <= runtime_.mach_target_pos) {
        // 定深结束
        CURRENT_POS = runtime_.mach_target_pos;
        drill_over = true;
    }

    s_motion_shared->set_current_drill_remaining_blu(CURRENT_POS -
                                                     runtime_.mach_target_pos);

    if (drill_over) {
        if (start_params_.back) {
            _drillstate_changeto(DrillState::PrepareBack);
        } else {
            _clear_all_status();

            _mach_on(false);
            _all_pump_and_spindle_on(false);
        }
    }
}

void DrillAutoTask::_drillstate_prepare_back() {
    _all_pump_and_spindle_on(true);
    _mach_on(false);

    _start_timer(1000);
    _drillstate_changeto(DrillState::PrepareBackDelaying);
}

void DrillAutoTask::_drillstate_prepare_back_delaying() {
    if (_is_timer_timeout()) {
        _drillstate_changeto(DrillState::ReturnBacking);
    }
}

void DrillAutoTask::_drillstate_return_backing() {
    double target_back_pos = runtime_.touched_pos;

    CURRENT_POS = CURRENT_POS + 5; // TODO, 目前固定速度回退

    if (CURRENT_POS >= target_back_pos) {
        // 回退完成
        s_logger->debug("DrillAutoTask, Return Backing Over: {}", CURRENT_POS);
        CURRENT_POS = target_back_pos;

        _clear_all_status();

        _mach_on(false);
        _all_pump_and_spindle_on(false);
    }
}

void DrillAutoTask::run_once() {
    if (pause_flag_) {
        return;
    }

    switch (drill_state_) {
    default:
        assert(false);
        s_logger->error("Unknow drill state: {}", (int)drill_state_);
        break;
    case DrillState::Idle:
        _drillstate_idle();
        break;
    case DrillState::PrepareTouch:
        _drillstate_prepare_touch();
        break;
    case DrillState::PrepareTouchDelaying:
        _drillstate_prepare_touch_delaying();
        break;
    case DrillState::Touching:
        _drillstate_touching();
        break;
    case DrillState::TouchedBacking:
        _drillstate_touched_backing();
        break;
    case DrillState::PrepareDrill:
        _drillstate_prepare_drill();
        break;
    case DrillState::PrepareDrillDelaying:
        _drillstate_prepare_drill_delaying();
        break;
    case DrillState::Drilling:
        _drillstate_drilling();
        break;
    case DrillState::PrepareBack:
        _drillstate_prepare_back();
        break;
    case DrillState::PrepareBackDelaying:
        _drillstate_prepare_back_delaying();
        break;
    case DrillState::ReturnBacking:
        _drillstate_return_backing();
        break;
    }
}

void DrillAutoTask::_all_pump_and_spindle_on(bool on) {
    cbs_.cb_opump_on(on);
    cbs_.cb_ipump_on(on);
    if (on) {
        s_motion_shared->get_spindle_controller()->start_spindle();
    } else {
        s_motion_shared->get_spindle_controller()->stop_spindle();
    }
}

void DrillAutoTask::_mach_on(bool on) { cbs_.cb_mach_on(on); }

const char *DrillAutoTask::GetDrillStateStr(DrillAutoTask::DrillState s) {
    switch (s) {
#define XX_(__s)                         \
    case DrillAutoTask::DrillState::__s: \
        return #__s;
        XX_(Idle)

        XX_(PrepareTouch)
        XX_(PrepareTouchDelaying)
        XX_(Touching)
        XX_(TouchedBacking)

        XX_(PrepareDrill)
        XX_(PrepareDrillDelaying)
        XX_(Drilling)

        XX_(PrepareBack)
        XX_(PrepareBackDelaying)
        XX_(ReturnBacking)

#undef XX_
    default:
        return "Unknow";
    }
}

void DrillAutoTask::_drillstate_changeto(DrillAutoTask::DrillState new_state) {
    s_logger->trace("Drill State: {} -> {}", GetDrillStateStr(drill_state_),
                    GetDrillStateStr(new_state));

    drill_state_ = new_state;
}

void DrillAutoTask::_clear_all_status() {
    _drillstate_changeto(DrillState::Idle);
    runtime_.clear();
    pause_flag_ = false;
}

} // namespace move

} // namespace edm

#endif