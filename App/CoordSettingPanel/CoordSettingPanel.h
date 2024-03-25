#pragma once

#include <QWidget>

#include <QWidget>
#include <QLabel>
#include <QLineEdit>

#include <array>

#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class CoordSettingPanel;
}

namespace edm {
namespace app {

class CoordSettingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CoordSettingPanel(SharedCoreData* shared_core_data, QWidget *parent = nullptr);
    ~CoordSettingPanel();

private:
    void update_all_display();

    void update_offset_display(); // 刷新偏置设定显示区域
    void update_softlimit_display(); // 刷新软限位显示区域

public: // slots
    // 供外部切换编程坐标系时调用, 以同时切换显示坐标系
    void slot_change_display_coord_index(uint32_t new_coord_index);

private:
    void _init_label_arr();
    void _init_coord_indexes();
    void _init_connection();

    void _init_offset_button_cb();
    void _init_softlimit_button_cb();

private: // slots
    void _change_display_coord_index(uint32_t new_index);

private:
    Ui::CoordSettingPanel *ui;

    SharedCoreData* shared_core_data_;

    edm::coord::CoordinateSystem::ptr coord_sys_;

    std::array<QLabel*, EDM_AXIS_MAX_NUM> offsetaxisname_label_arr_;
    
    std::array<QLineEdit*, EDM_AXIS_MAX_NUM> coord_offset_le_arr_;
    std::array<QLineEdit*, EDM_AXIS_MAX_NUM> global_offset_le_arr_;

    std::array<QLineEdit*, EDM_AXIS_MAX_NUM> softlimit_pos_le_arr_;
    std::array<QLineEdit*, EDM_AXIS_MAX_NUM> softlimit_neg_le_arr_;

    std::unordered_map<uint32_t, uint32_t> coord_index2combobox_index_map_;
};

} // namespace app
} // namespace edm
