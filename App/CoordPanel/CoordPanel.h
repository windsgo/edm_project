#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>

#include <array>
#include <memory>

#include "SharedCoreData/SharedCoreData.h"
#include "Coordinate/CoordinateSystem.h"

namespace Ui {
class CoordPanel;
}

namespace edm {
namespace app {

class CoordPanel : public QWidget {
    Q_OBJECT

public:
    explicit CoordPanel(SharedCoreData* shared_core_data, QWidget *parent = nullptr);
    ~CoordPanel();

private:
    void update_all_display();

    void update_axis_display(); // 刷新坐标显示区域

public: // slots
    // 供外部 (如G代码) 切换编程坐标系时调用, 以同时切换显示坐标系
    void slot_change_display_coord_index(uint32_t new_coord_index);

private:
    void _init_label_arr();
    void _init_coord_indexes();
    void _init_connection();

private: // slots
    void _update_info(const move::MotionInfo& info);
    void _change_display_coord_index(uint32_t new_index);

private:
    Ui::CoordPanel *ui;

    SharedCoreData* shared_core_data_;

    edm::coord::CoordinateSystem::ptr coord_sys_;

    std::array<QLabel*, EDM_AXIS_MAX_NUM> axisname_label_arr_;

    std::array<QLabel*, EDM_AXIS_MAX_NUM> cmd_axis_label_arr_;
    std::array<QLabel*, EDM_AXIS_MAX_NUM> act_axis_label_arr_;
    std::array<QLabel*, EDM_AXIS_MAX_NUM> mach_cmd_axis_label_arr_;
    std::array<QLabel*, EDM_AXIS_MAX_NUM> mach_act_axis_label_arr_;

    std::unordered_map<uint32_t, uint32_t> coord_index2combobox_index_map_;
};

} // namespace app
} // namespace edm
