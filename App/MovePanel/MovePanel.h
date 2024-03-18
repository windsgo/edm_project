#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "SharedCoreData/SharedCoreData.h"

#include "TaskManager/TaskManager.h"

#include "array"

namespace Ui {
class MovePanel;
}

namespace edm {
namespace app {

class MovePanel : public QWidget {
    Q_OBJECT

public:
    explicit MovePanel(SharedCoreData *shared_core_data,
                        task::TaskManager* task_manager,
                       QWidget *parent = nullptr);
    ~MovePanel();

private:
    void _init_array();

    void _init_button_cb();

    void _init_motionsig_connection();

    void _init_handbox_auto_signals();

private:
    void _clear_pm_le_values();

private:
    bool _start_pointmove_no_softlimit_check(
        const move::axis_t &target_pos,
        const move::MoveRuntimePlanSpeedInput &speed_param,
        bool enable_touch_detect) const;

    void _start_single_axis_pointmove_pos(uint32_t axis_index, bool touch_detect_enable) const;
    void _start_single_axis_pointmove_neg(uint32_t axis_index, bool touch_detect_enable) const;

    void _le_start_pointmove() const;

    bool _stop_pointmove() const;

    void _cmd_ecat_trigger_connect() const;

private:
    move::unit_t _get_speed_blu_s_from_mfrx() const;
    move::MoveRuntimePlanSpeedInput _get_default_speed_param() const;

    bool _get_levalues_blu_from_ui(
        std::array<std::optional<double>, EDM_AXIS_NUM> &le_opt_value)
        const; // if invalid, return false
    bool _get_target_pos_by_incabs_and_machcoord(
        move::axis_t &mach_target_pos, const move::axis_t &mach_start_pos,
        const std::array<std::optional<double>, EDM_AXIS_NUM> &le_opt_value) const;
    bool _check_posandneg_soft_limit(const move::axis_t &mach_target_pos) const;

private:
    Ui::MovePanel *ui;

    SharedCoreData *shared_core_data_;
    task::TaskManager* task_manager_;

    coord::CoordinateSystem::ptr coord_sys_;

    std::array<QPushButton *, EDM_AXIS_MAX_NUM> arr_pb_singlepm_posdir_;
    std::array<QPushButton *, EDM_AXIS_MAX_NUM> arr_pb_singlepm_negdir_;

    std::array<QLineEdit *, EDM_AXIS_MAX_NUM> arr_le_pm_value_;

    std::array<QPushButton *, EDM_AXIS_MAX_NUM> arr_pb_lepm_select_axis_;
};

} // namespace app
} // namespace edm
