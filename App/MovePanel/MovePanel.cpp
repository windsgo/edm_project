#include "MovePanel.h"
#include "ui_MovePanel.h"

#include "InputHelper/InputHelper.h"

#include "Utils/UnitConverter/UnitConverter.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

MovePanel::MovePanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::MovePanel),
      shared_core_data_(shared_core_data),
      coord_sys_(shared_core_data->get_coord_system()) {
    ui->setupUi(this);

    _init_array();

    _init_button_cb();
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

        arr_pb_singlepm_posdir_.at(i)->setEnabled(false);
        arr_pb_singlepm_negdir_.at(i)->setEnabled(false);
    }
}

void MovePanel::_init_button_cb() {
#define CONNECT_PB_SINGLEPM_POS_SLOT(i__)                                    \
    connect(arr_pb_singlepm_posdir_.at((i__)), &QPushButton::pressed, this,  \
            [this]() { this->_start_single_axis_pointmove_pos((i__)); });    \
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
            [this]() { this->_start_single_axis_pointmove_neg((i__)); });    \
    connect(arr_pb_singlepm_negdir_.at((i__)), &QPushButton::released, this, \
            [this]() { this->_stop_pointmove(); });

    CONNECT_PB_SINGLEPM_NEG_SLOT(0);
    CONNECT_PB_SINGLEPM_NEG_SLOT(1);
    CONNECT_PB_SINGLEPM_NEG_SLOT(2);
    CONNECT_PB_SINGLEPM_NEG_SLOT(3);
    CONNECT_PB_SINGLEPM_NEG_SLOT(4);
    CONNECT_PB_SINGLEPM_NEG_SLOT(5);

#undef CONNECT_PB_SINGLEPM_NEG_SLOT
}

void MovePanel::_start_pointmove_no_softlimit_check(
    const move::axis_t &target_pos) {
    const auto &start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;
    
    // TODO get speed param
    // create motion cmd
    edm::move::MoveRuntimePlanSpeedInput speed;
    speed.nacc = 60;
    speed.entry_v = 0;
    speed.exit_v = 0;
    speed.cruise_v = util::UnitConverter::um2blu(30000);
    speed.acc0 = util::UnitConverter::um2blu(500000);
    speed.dec0 = -speed.acc0;

    auto start_pointmove_cmd =
        std::make_shared<edm::move::MotionCommandManualStartPointMove>(
            start_pos, target_pos, speed, true);

    // wrap and push to global command queue
    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [this](edm::move::MotionCommandManualStartPointMove::ptr a) {
            this->shared_core_data_->get_motion_cmd_queue()->push_command(a);
        },
        start_pointmove_cmd);

    shared_core_data_->get_global_cmd_queue()->push_command(gcmd);
}

void MovePanel::_start_pointmove(const move::axis_t &target_pos) {
    // TODO

    // TODO if coord, transvert coord pos to mach pos

    // TODO check mach pos soft limit, and check if start pos is out of limit

    // TODO transvert to motor pos

    // _start_pointmove_no_softlimit_check(target_pos);
}

void MovePanel::_start_single_axis_pointmove_pos(uint32_t axis_index) {
    if (axis_index >= coord::Coordinate::Size) {
        s_logger->error("{}, index error: {}", __PRETTY_FUNCTION__, axis_index);
        return;
    }

    // motor start pos
    const auto &motor_start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    // mach start pos
    move::axis_t mach_start_pos;
    coord_sys_->get_cm().motor_to_machine(motor_start_pos, mach_start_pos);

    // get mach target axis pos by softlimit, other axis by start_pos
    move::axis_t mach_target_pos{mach_start_pos};
    mach_target_pos[axis_index] =  coord_sys_->get_pos_soft_limit()[axis_index];

    if (mach_start_pos[axis_index] > mach_target_pos[axis_index]) {
        s_logger->warn("axis {} is already outof pos softlimit, limit mach value: {}", axis_index,
                       mach_target_pos[axis_index]);
        return;
    }

    // transvert to motor target pos
    move::axis_t motor_target_pos;
    coord_sys_->get_cm().machine_to_motor(mach_target_pos, motor_target_pos);

    _start_pointmove_no_softlimit_check(motor_target_pos);
}

void MovePanel::_start_single_axis_pointmove_neg(uint32_t axis_index) {
    if (axis_index >= coord::Coordinate::Size) {
        s_logger->error("{}, index error: {}", __PRETTY_FUNCTION__, axis_index);
        return;
    }

    // motor start pos
    const auto &motor_start_pos =
        shared_core_data_->get_info_dispatcher()->get_info().curr_cmd_axis_blu;

    // mach start pos
    move::axis_t mach_start_pos;
    coord_sys_->get_cm().motor_to_machine(motor_start_pos, mach_start_pos);

    // get mach target axis pos by softlimit, other axis by start_pos
    move::axis_t mach_target_pos{mach_start_pos};
    mach_target_pos[axis_index] =  coord_sys_->get_neg_soft_limit()[axis_index];

    if (mach_start_pos[axis_index] < mach_target_pos[axis_index]) {
        s_logger->warn("axis {} is already outof neg softlimit, limit mach value: {}", axis_index,
                       mach_target_pos[axis_index]);
        return;
    }

    // transvert to motor target pos
    move::axis_t motor_target_pos;
    coord_sys_->get_cm().machine_to_motor(mach_target_pos, motor_target_pos);

    _start_pointmove_no_softlimit_check(motor_target_pos);
}

void MovePanel::_stop_pointmove() {
    // create motion cmd
    auto stop_pointmove_command =
        std::make_shared<edm::move::MotionCommandManualStopPointMove>(false);

    // wrap and push to global command queue
    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [this](edm::move::MotionCommandManualStopPointMove::ptr a) {
            this->shared_core_data_->get_motion_cmd_queue()->push_command(a);
        },
        stop_pointmove_command);

    shared_core_data_->get_global_cmd_queue()->push_command(gcmd);
}

} // namespace app
} // namespace edm
