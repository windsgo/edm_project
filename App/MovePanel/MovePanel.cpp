#include "MovePanel.h"
#include "Motion/MoveDefines.h"
#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"
#include "ui_MovePanel.h"

#include <qpushbutton.h>
#include <sstream>

#include "InputHelper/InputHelper.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

constexpr static const int s_statusbar_timeout = 10000;

namespace edm {
namespace app {

MovePanel::MovePanel(SharedCoreData *shared_core_data,
                     task::TaskManager *task_manager, QWidget *parent)
    : QWidget(parent), ui(new Ui::MovePanel),
      shared_core_data_(shared_core_data), task_manager_(task_manager),
      coord_sys_(shared_core_data->get_coord_system()) {
    ui->setupUi(this);

    _init_array();

    _init_button_cb();

    _init_motionsig_connection();

    _init_handbox_auto_signals();
}

MovePanel::~MovePanel() { delete ui; }

void MovePanel::_init_array() {
    arr_le_pm_value_.at(0) = ui->le_pm_value_x;
    arr_le_pm_value_.at(1) = ui->le_pm_value_y;
    arr_le_pm_value_.at(2) = ui->le_pm_value_z;
    arr_le_pm_value_.at(3) = ui->le_pm_value_b;
    arr_le_pm_value_.at(4) = ui->le_pm_value_c;
    arr_le_pm_value_.at(5) = ui->le_pm_value_a;

    for (auto &le : arr_le_pm_value_) {
        InputHelper::SetLineeditCoordValidator(le, this);
    }

    arr_pb_lepm_select_axis_.at(0) = ui->pb_check_le_pm_x;
    arr_pb_lepm_select_axis_.at(1) = ui->pb_check_le_pm_y;
    arr_pb_lepm_select_axis_.at(2) = ui->pb_check_le_pm_z;
    arr_pb_lepm_select_axis_.at(3) = ui->pb_check_le_pm_b;
    arr_pb_lepm_select_axis_.at(4) = ui->pb_check_le_pm_c;
    arr_pb_lepm_select_axis_.at(5) = ui->pb_check_le_pm_a;

    arr_pb_singlepm_posdir_.at(0) = ui->pb_pos_pm_x;
    arr_pb_singlepm_posdir_.at(1) = ui->pb_pos_pm_y;
    arr_pb_singlepm_posdir_.at(2) = ui->pb_pos_pm_z;
    arr_pb_singlepm_posdir_.at(3) = ui->pb_pos_pm_b;
    arr_pb_singlepm_posdir_.at(4) = ui->pb_pos_pm_c;
    arr_pb_singlepm_posdir_.at(5) = ui->pb_pos_pm_a;

    arr_pb_singlepm_negdir_.at(0) = ui->pb_neg_pm_x;
    arr_pb_singlepm_negdir_.at(1) = ui->pb_neg_pm_y;
    arr_pb_singlepm_negdir_.at(2) = ui->pb_neg_pm_z;
    arr_pb_singlepm_negdir_.at(3) = ui->pb_neg_pm_b;
    arr_pb_singlepm_negdir_.at(4) = ui->pb_neg_pm_c;
    arr_pb_singlepm_negdir_.at(5) = ui->pb_neg_pm_a;

    for (std::size_t i = EDM_AXIS_NUM; i < EDM_AXIS_MAX_NUM; ++i) {
        arr_le_pm_value_.at(i)->setEnabled(false);
        arr_le_pm_value_.at(i)->setText("null");

        arr_pb_lepm_select_axis_.at(i)->setChecked(false);
        arr_pb_lepm_select_axis_.at(i)->setEnabled(false);

        arr_pb_singlepm_posdir_.at(i)->setEnabled(false);
        arr_pb_singlepm_negdir_.at(i)->setEnabled(false);
    }
}

void MovePanel::_init_button_cb() {

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    ui->pb_drill_spindle->setCheckable(true);
    connect(ui->pb_drill_spindle, &QPushButton::clicked, this,
            [this](bool checked) {
                auto cmd = std::make_shared<move::MotionCommandSetSpindleState>(
                    checked);
                shared_core_data_->get_motion_cmd_queue()->push_command(cmd);

                auto ret = task::TaskHelper::WaitforCmdTobeAccepted(cmd, 1000);
                if (!ret) {
                    s_logger->error("set spindle [{}] failed", checked);
                }
            });

    connect(shared_core_data_->get_info_dispatcher(),
            &InfoDispatcher::info_updated, this,
            [this](const move::MotionInfo &info) {
                ui->pb_drill_spindle->setChecked(info.is_spindle_on);
            });

#else
    ui->pb_drill_spindle->hide();
#endif

    //! x+ x- ...
#define CONNECT_PB_SINGLEPM_POS_SLOT(i__)                                    \
    connect(arr_pb_singlepm_posdir_.at((i__)), &QPushButton::pressed, this,  \
            [this]() {                                                       \
                this->_start_single_axis_pointmove_pos(                      \
                    (i__), !ui->pb_check_st->isChecked());                   \
            });                                                              \
    connect(arr_pb_singlepm_posdir_.at((i__)), &QPushButton::released, this, \
            [this]() { this->_stop_pointmove(); });

    CONNECT_PB_SINGLEPM_POS_SLOT(0);
    CONNECT_PB_SINGLEPM_POS_SLOT(1);
    CONNECT_PB_SINGLEPM_POS_SLOT(2);
    CONNECT_PB_SINGLEPM_POS_SLOT(3);
    CONNECT_PB_SINGLEPM_POS_SLOT(4);
    CONNECT_PB_SINGLEPM_POS_SLOT(5);

#undef CONNECT_PB_SINGLEPM_POS_SLOT

#define CONNECT_PB_SINGLEPM_NEG_SLOT(i__)                                    \
    connect(arr_pb_singlepm_negdir_.at((i__)), &QPushButton::pressed, this,  \
            [this]() {                                                       \
                this->_start_single_axis_pointmove_neg(                      \
                    (i__), !ui->pb_check_st->isChecked());                   \
            });                                                              \
    connect(arr_pb_singlepm_negdir_.at((i__)), &QPushButton::released, this, \
            [this]() { this->_stop_pointmove(); });

    CONNECT_PB_SINGLEPM_NEG_SLOT(0);
    CONNECT_PB_SINGLEPM_NEG_SLOT(1);
    CONNECT_PB_SINGLEPM_NEG_SLOT(2);
    CONNECT_PB_SINGLEPM_NEG_SLOT(3);
    CONNECT_PB_SINGLEPM_NEG_SLOT(4);
    CONNECT_PB_SINGLEPM_NEG_SLOT(5);

#undef CONNECT_PB_SINGLEPM_NEG_SLOT

    //! ecat connect
    connect(ui->pb_trigger_ecat_connect, &QPushButton::clicked, this,
            &MovePanel::_cmd_ecat_trigger_connect);

    //! le start clear stop ...
    connect(ui->pb_clear_pm_le, &QPushButton::clicked, this,
            &MovePanel::_clear_pm_le_values);

    connect(ui->pb_le_start_pointmove, &QPushButton::clicked, this,
            &MovePanel::_le_start_pointmove);

    connect(ui->pb_le_stop_pointmove, &QPushButton::clicked, this,
            &MovePanel::_stop_pointmove);
}

void MovePanel::_init_motionsig_connection() {
    connect(task_manager_, &task::TaskManager::sig_manual_pointmove_started,
            this, [this]() {
                emit shared_core_data_->sig_info_message("pointmove started",
                                                         s_statusbar_timeout);
            });

    connect(task_manager_, &task::TaskManager::sig_manual_pointmove_stopped,
            this, [this]() {
                if (shared_core_data_->get_info_dispatcher()
                        ->get_info()
                        .TouchWarning()) {
                    emit shared_core_data_->sig_warn_message(
                        "pointmove stopped: Touch Warning",
                        s_statusbar_timeout);
                } else {
                    emit shared_core_data_->sig_info_message(
                        "pointmove stopped", s_statusbar_timeout);
                }
            });
}

void MovePanel::_init_handbox_auto_signals() {
    // 手控盒信号处理
    auto s = this->shared_core_data_;

    connect(s, &SharedCoreData::sig_handbox_stop_pointmove, this,
            [this]() { this->_stop_pointmove(); });

    connect(s, &SharedCoreData::sig_handbox_stop_auto, this, [this]() {
        this->_stop_pointmove();
        this->shared_core_data_->send_ioboard_bz_once();
    });

    connect(s, &SharedCoreData::sig_handbox_start_pointmove, this,
            [this](const move::axis_t &dir, uint32_t speed_level,
                   bool touch_detect_enable) {
                if (speed_level >= 3) {
                    ui->pb_check_mfr3->setChecked(true);
                } else if (speed_level == 2) {
                    ui->pb_check_mfr2->setChecked(true);
                } else if (speed_level == 1) {
                    ui->pb_check_mfr1->setChecked(true);
                } else {
                    ui->pb_check_mfr0->setChecked(true);
                }

                this->_start_multiaxis_vectormove(dir, touch_detect_enable);
            });
}

void MovePanel::_clear_pm_le_values() {
    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        arr_le_pm_value_[i]->clear();
        arr_pb_lepm_select_axis_[i]->setChecked(false);
    }
}

bool MovePanel::_start_pointmove_no_softlimit_check(
    const move::axis_t &target_pos,
    const move::MoveRuntimePlanSpeedInput &speed_param,
    bool enable_touch_detect) const {
    const auto &start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    auto start_pointmove_cmd =
        std::make_shared<edm::move::MotionCommandManualStartPointMove>(
            start_pos, target_pos, speed_param, enable_touch_detect);

    return task_manager_->operation_start_pointmove(start_pointmove_cmd);

    // this->shared_core_data_->get_motion_cmd_queue()->push_command(
    //     start_pointmove_cmd);

    // // wrap and push to global command queue
    // auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
    //     [this](edm::move::MotionCommandManualStartPointMove::ptr a) {
    //         this->shared_core_data_->get_motion_cmd_queue()->push_command(a);
    //     },
    //     start_pointmove_cmd);

    // shared_core_data_->get_global_cmd_queue()->push_command(gcmd);
}

void MovePanel::_le_start_pointmove() const {
    // motor start pos
    const auto &motor_start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    // mach start pos
    move::axis_t mach_start_pos;
    coord_sys_->get_cm().motor_to_machine(motor_start_pos, mach_start_pos);

    // 界面输入值 (只取选中的坐标)
    std::array<std::optional<double>, EDM_AXIS_NUM> le_opt_value;

    auto input_valid = _get_levalues_blu_from_ui(le_opt_value);
    if (!input_valid) {
        s_logger->error("start pointmove failed : input invalid");
        emit shared_core_data_->sig_error_message(
            "start pointmove failed : input invalid", s_statusbar_timeout);
        return;
    }

    move::axis_t mach_target_pos{mach_start_pos};
    auto get_ret = _get_target_pos_by_incabs_and_machcoord(
        mach_target_pos, mach_start_pos, le_opt_value);
    if (!get_ret) {
        return; // log error inside the function
    }

    // {
    //     std::stringstream ss;
    //     for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
    //         ss << mach_target_pos[i] << ", ";
    //     }
    //     // s_logger->debug("mach_target_pos: {}", ss.str());
    // }

    if (move::MotionUtils::IsAxisTheSame(mach_start_pos, mach_target_pos)) {
        s_logger->error("start pointmove failed : start and target the same");
        emit shared_core_data_->sig_error_message(
            "start pointmove failed : start and target the same",
            s_statusbar_timeout);
        return;
    }

    // check softlimit, 只要超过软限位, 这里正反都不让走, 不区分方向, 简化逻辑
    if (!_check_posandneg_soft_limit(mach_target_pos)) {
        s_logger->error("start pointmove failed : out of soft limit");
        emit shared_core_data_->sig_error_message(
            "start pointmove failed: out of soft limit", s_statusbar_timeout);
        return;
    }

    move::axis_t motor_target_pos;
    coord_sys_->get_cm().machine_to_motor(mach_target_pos, motor_target_pos);

    // get speed param
    auto speed = _get_default_speed_param();

    auto ret = _start_pointmove_no_softlimit_check(
        motor_target_pos, speed, !ui->pb_check_st->isChecked());

    if (!ret) {
        emit shared_core_data_->sig_error_message(
            "start pointmove failed: send cmd error", s_statusbar_timeout);
    }
}

void MovePanel::_start_single_axis_pointmove_pos(
    uint32_t axis_index, bool touch_detect_enable) const {
    if (axis_index >= coord::Coordinate::Size) {
        s_logger->error("{}, index error: {}", __PRETTY_FUNCTION__, axis_index);
        return;
    }

    move::axis_t dir;
    for (uint8_t i = 0; i < dir.size(); ++i) {
        if (i == axis_index) {
            dir[i] = 1.0;
        } else {
            dir[i] = 0.0;
        }
    }
    this->_start_multiaxis_vectormove(dir, touch_detect_enable);
    return;

#if 0
    // motor start pos
    const auto &motor_start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    // mach start pos
    move::axis_t mach_start_pos;
    coord_sys_->get_cm().motor_to_machine(motor_start_pos, mach_start_pos);

    // get mach target axis pos by softlimit, other axis by start_pos
    move::axis_t mach_target_pos{mach_start_pos};
    mach_target_pos[axis_index] = coord_sys_->get_pos_soft_limit()[axis_index];

    if (mach_start_pos[axis_index] > mach_target_pos[axis_index]) {
        s_logger->warn(
            "axis {} is already outof pos softlimit, limit mach value: {}",
            axis_index, mach_target_pos[axis_index]);
        emit shared_core_data_->sig_warn_message(
            "start pointmove failed: already outof pos softlimit",
            s_statusbar_timeout);
        return;
    }

    // transvert to motor target pos
    move::axis_t motor_target_pos;
    coord_sys_->get_cm().machine_to_motor(mach_target_pos, motor_target_pos);

    // get speed param, and deal with MFR3
    auto speed = _get_default_speed_param();
    if (ui->pb_check_mfr3->isChecked()) {
        // as fast as possible to reach 1um target incs
        speed.cruise_v = 30000;

        // modify target pos
        motor_target_pos[axis_index] =
            motor_start_pos[axis_index] + util::UnitConverter::um2blu(1.0);
    }

    auto ret = _start_pointmove_no_softlimit_check(motor_target_pos, speed,
                                                   touch_detect_enable);

    if (!ret) {
        emit shared_core_data_->sig_error_message(
            "start pointmove failed: send cmd error", s_statusbar_timeout);
    }
#endif
}

void MovePanel::_start_single_axis_pointmove_neg(
    uint32_t axis_index, bool touch_detect_enable) const {
    if (axis_index >= coord::Coordinate::Size) {
        s_logger->error("{}, index error: {}", __PRETTY_FUNCTION__, axis_index);
        return;
    }

    move::axis_t dir;
    for (uint8_t i = 0; i < dir.size(); ++i) {
        if (i == axis_index) {
            dir[i] = -1.0;
        } else {
            dir[i] = 0.0;
        }
    }
    this->_start_multiaxis_vectormove(dir, touch_detect_enable);
    return;

#if 0
    // motor start pos
    const auto &motor_start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    // mach start pos
    move::axis_t mach_start_pos;
    coord_sys_->get_cm().motor_to_machine(motor_start_pos, mach_start_pos);

    // get mach target axis pos by softlimit, other axis by start_pos
    move::axis_t mach_target_pos{mach_start_pos};
    mach_target_pos[axis_index] = coord_sys_->get_neg_soft_limit()[axis_index];

    if (mach_start_pos[axis_index] < mach_target_pos[axis_index]) {
        s_logger->warn(
            "axis {} is already outof neg softlimit, limit mach value: {}",
            axis_index, mach_target_pos[axis_index]);
        return;
    }

    // transvert to motor target pos
    move::axis_t motor_target_pos;
    coord_sys_->get_cm().machine_to_motor(mach_target_pos, motor_target_pos);

    // get speed param, and deal with MFR3
    auto speed = _get_default_speed_param();
    if (ui->pb_check_mfr3->isChecked()) {
        // as fast as possible to reach 1um target incs
        speed.cruise_v = 30000;

        // modify target pos
        motor_target_pos[axis_index] =
            motor_start_pos[axis_index] - util::UnitConverter::um2blu(1.0);
    }

    auto ret = _start_pointmove_no_softlimit_check(motor_target_pos, speed,
                                                   touch_detect_enable);

    if (!ret) {
        emit shared_core_data_->sig_error_message(
            "start pointmove failed: send cmd error", s_statusbar_timeout);
    }
#endif
}

void MovePanel::_start_multiaxis_vectormove(const move::axis_t &dir,
                                            bool touch_detect_enable) const {
    // motor start pos
    const auto &motor_start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    // mach start pos
    move::axis_t mach_start_pos;
    coord_sys_->get_cm().motor_to_machine(motor_start_pos, mach_start_pos);

    // Solve Vector Move End Point (Mach Pos) (According to soft limits)
    move::axis_t incs;
    {
        const auto &pos_soft_limits = coord_sys_->get_pos_soft_limit();
        const auto &neg_soft_limits = coord_sys_->get_neg_soft_limit();

        bool flag = false;
        move::unit_t each_axis_inc_length = 0.0;
        for (uint8_t i = 0; i < dir.size(); ++i) {
            move::unit_t left_length = 0.0;
            if (dir[i] > 0) {
                // 正方向剩余距离，正常为正
                left_length = pos_soft_limits[i] - mach_start_pos[i];
                if (left_length <= 0) {
                    s_logger->warn("axis {} is already outof pos softlimit, "
                                   "left_length: {}",
                                   i, left_length);
                    return; // 放弃点动
                }
            } else if (dir[i] < 0) {
                // 负方向剩余距离，正常为负
                left_length = neg_soft_limits[i] - mach_start_pos[i];
                if (left_length >= 0) {
                    s_logger->warn("axis {} is already outof neg softlimit, "
                                   "left_length: {}",
                                   i, left_length);
                    return; // 放弃点动
                }
            } else {
                continue;
            }

            if ((!flag) || std::fabs(left_length) < each_axis_inc_length) {
                flag = true;
                each_axis_inc_length = fabs(left_length); // 取更小的那个
            }
        }

        for (uint8_t i = 0; i < dir.size(); ++i) {
            if (dir[i] > 0) {
                incs[i] = each_axis_inc_length;
            } else if (dir[i] < 0) {
                incs[i] = -each_axis_inc_length;
            } else {
                incs[i] = 0;
            }
        }
    }

    // calc mach target axis
    move::axis_t mach_target_pos{mach_start_pos};
    for (uint8_t i = 0; i < dir.size(); ++i) {
        mach_target_pos[i] = mach_start_pos[i] + incs[i];
    }

    // transvert to motor target pos
    move::axis_t motor_target_pos;
    coord_sys_->get_cm().machine_to_motor(mach_target_pos, motor_target_pos);

    // get speed param, and deal with MFR3
    auto speed = _get_default_speed_param();
    if (ui->pb_check_mfr3->isChecked()) {
        // as fast as possible to reach 1um target incs
        speed.cruise_v = 30000;

        // modify target pos
        for (uint8_t i = 0; i < dir.size(); ++i) {
            if (dir[i] > 0) {
                motor_target_pos[i] =
                    motor_start_pos[i] + util::UnitConverter::um2blu(1.0);
            } else if (dir[i] < 0) {
                motor_target_pos[i] =
                    motor_start_pos[i] - util::UnitConverter::um2blu(1.0);
            } else {
                motor_target_pos[i] = motor_start_pos[i];
            }
        }
    }

    auto ret = _start_pointmove_no_softlimit_check(motor_target_pos, speed,
                                                   touch_detect_enable);

    if (!ret) {
        emit shared_core_data_->sig_error_message(
            "start pointmove failed: send cmd error", s_statusbar_timeout);
    }
}

bool MovePanel::_stop_pointmove() const {

    return task_manager_->operation_stop_pointmove();

    // // create motion cmd
    // auto stop_pointmove_command =
    //     std::make_shared<edm::move::MotionCommandManualStopPointMove>(false);

    // // wrap and push to global command queue
    // auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
    //     [this](edm::move::MotionCommandManualStopPointMove::ptr a) {
    //         this->shared_core_data_->get_motion_cmd_queue()->push_command(a);
    //     },
    //     stop_pointmove_command);

    // shared_core_data_->get_global_cmd_queue()->push_command(gcmd);
}

void MovePanel::_cmd_ecat_trigger_connect() const {
    // create motion cmd
    auto ecat_connect_cmd =
        std::make_shared<edm::move::MotionCommandSettingTriggerEcatConnect>();

    this->shared_core_data_->get_motion_cmd_queue()->push_command(
        ecat_connect_cmd);

    return;

    // wrap and push to global command queue
    // auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
    //     [this](edm::move::MotionCommandSettingTriggerEcatConnect::ptr a) {
    //         this->shared_core_data_->get_motion_cmd_queue()->push_command(a);
    //     },
    //     ecat_connect_cmd);

    // shared_core_data_->get_global_cmd_queue()->push_command(gcmd);
}

move::unit_t MovePanel::_get_speed_blu_s_from_mfrx() const {
    move::unit_t speed_um_s = 0.0;

    if (ui->pb_check_mfr0->isChecked()) {
        speed_um_s =
            shared_core_data_->get_system_settings().get_fmparam_speed_0_um_s();
    } else if (ui->pb_check_mfr1->isChecked()) {
        speed_um_s =
            shared_core_data_->get_system_settings().get_fmparam_speed_1_um_s();
    } else if (ui->pb_check_mfr2->isChecked()) {
        speed_um_s =
            shared_core_data_->get_system_settings().get_fmparam_speed_2_um_s();
    } else {
        speed_um_s =
            shared_core_data_->get_system_settings().get_fmparam_speed_3_um_s();
    }

    return util::UnitConverter::um2blu(speed_um_s);
}

move::MoveRuntimePlanSpeedInput MovePanel::_get_default_speed_param() const {
    edm::move::MoveRuntimePlanSpeedInput speed;

    speed.nacc = util::UnitConverter::ms2p(
        shared_core_data_->get_system_settings().get_fmparam_nacc_ms());
    speed.acc0 = util::UnitConverter::um2blu(
        shared_core_data_->get_system_settings().get_fmparam_max_acc_um_s2());
    speed.dec0 = -speed.acc0;
    speed.entry_v = 0;
    speed.exit_v = 0;
    speed.cruise_v = _get_speed_blu_s_from_mfrx();

    return speed;
}

bool MovePanel::_get_levalues_blu_from_ui(
    std::array<std::optional<double>, EDM_AXIS_NUM> &le_opt_value) const {
    bool has_axis_selected = false;

    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        if (arr_pb_lepm_select_axis_[i]->isChecked()) {
            auto ret =
                InputHelper::QStringToDouble(arr_le_pm_value_[i]->text());

            if (!ret) {
                return false;
            } else {
                le_opt_value[i] = util::UnitConverter::mm2blu(*ret);
                has_axis_selected = true;
            }
        }
    }

    if (!has_axis_selected) {
        return false;
    }

    return true;
}

bool MovePanel::_get_target_pos_by_incabs_and_machcoord(
    move::axis_t &mach_target_pos, const move::axis_t &mach_start_pos,
    const std::array<std::optional<double>, EDM_AXIS_NUM> &le_opt_value) const {
    if (ui->pb_check_pmmode_inc->isChecked()) {
        // inc, mach/coord the same
        mach_target_pos = mach_start_pos;
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            if (le_opt_value[i]) {
                mach_target_pos[i] +=
                    *(le_opt_value[i]); // add inc to get target mach pos
            }
        }
    } else {
        // abs
        if (ui->pb_check_machcoord->isChecked()) {
            // mach coord
            for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                if (le_opt_value[i]) {
                    mach_target_pos[i] =
                        *(le_opt_value[i]); // set to target mach pos
                } else {
                    mach_target_pos[i] = mach_start_pos[i];
                }
            }
        } else {
            // gxx coord
            uint32_t curr_coord_index = coord_sys_->get_current_coord_index();

            // 先将输入的机床坐标起点转化为坐标系坐标起点
            move::axis_t coord_start_pos;
            bool ret1 = coord_sys_->get_cm().machine_to_coord(
                curr_coord_index, mach_start_pos, coord_start_pos);
            if (!ret1) {
                s_logger->error("{}, machine_to_coord failed",
                                __PRETTY_FUNCTION__);
                return false;
            }

            // 根据le_opt_value设定coord_target_pos
            move::axis_t coord_target_pos;
            for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                if (le_opt_value[i]) {
                    coord_target_pos[i] = *(le_opt_value[i]);
                } else {
                    coord_target_pos[i] = coord_start_pos[i];
                }
            }

            // 再转化为 MachTargetPos
            bool ret2 = coord_sys_->get_cm().coord_to_machine(
                curr_coord_index, coord_target_pos, mach_target_pos);
            if (!ret2) {
                s_logger->error("{}, coord_to_machine failed",
                                __PRETTY_FUNCTION__);
                return false;
            }
        }
    }

    return true;
}

bool MovePanel::_check_posandneg_soft_limit(
    const move::axis_t &mach_target_pos) const {
    const auto &pos_sl = coord_sys_->get_pos_soft_limit();
    const auto &neg_sl = coord_sys_->get_neg_soft_limit();

    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        if (mach_target_pos[i] > pos_sl[i] || mach_target_pos[i] < neg_sl[i]) {
            return false;
        }
    }

    return true;
}

} // namespace app

} // namespace edm
