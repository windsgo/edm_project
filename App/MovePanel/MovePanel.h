#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>

#include "SharedCoreData/SharedCoreData.h"

#include "array"

namespace Ui {
class MovePanel;
}

namespace edm {
namespace app {

class MovePanel : public QWidget {
    Q_OBJECT

public:
    explicit MovePanel(SharedCoreData *shared_core_data, QWidget *parent = nullptr);
    ~MovePanel();

private:
    void _init_array();

    void _init_button_cb();

private:
    void _start_pointmove_no_softlimit_check(const move::axis_t& target_pos);

    void _start_pointmove(const move::axis_t& target_pos);

    void _start_single_axis_pointmove_pos(uint32_t axis_index);
    void _start_single_axis_pointmove_neg(uint32_t axis_index);

    void _stop_pointmove();

private:
    Ui::MovePanel *ui;

    SharedCoreData *shared_core_data_;

    coord::CoordinateSystem::ptr coord_sys_;

    std::array<QPushButton*, EDM_AXIS_MAX_NUM> arr_pb_singlepm_posdir_;
    std::array<QPushButton*, EDM_AXIS_MAX_NUM> arr_pb_singlepm_negdir_;

    std::array<QLineEdit*, EDM_AXIS_MAX_NUM> arr_le_pm_value_;
};

} // namespace app
} // namespace edm
