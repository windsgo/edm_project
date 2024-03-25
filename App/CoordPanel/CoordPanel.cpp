#include "CoordPanel.h"
#include "ui_CoordPanel.h"

#include <QMessageBox>

#include "Logger/LogMacro.h"

#include "InputHelper/InputHelper.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
CoordPanel::CoordPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::CoordPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    coord_sys_ = shared_core_data_->get_coord_system();

    _init_label_arr();
    _init_coord_indexes();

    _init_connection();

    update_all_display();
}

CoordPanel::~CoordPanel() { delete ui; }

void CoordPanel::update_all_display() {
    update_axis_display();
}

void CoordPanel::update_axis_display() {
    // 直接从 coord_sys_ 的缓存中取坐标数据
    const auto &cmd_coord_axis = coord_sys_->get_current_coord_axis();
    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        cmd_axis_label_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(cmd_coord_axis[i])));
    }

    const auto &act_coord_axis = coord_sys_->get_current_coord_axis_act();
    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        act_axis_label_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(act_coord_axis[i])));
    }

    const auto &cmd_mach_axis = coord_sys_->get_current_machine_axis();
    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        mach_cmd_axis_label_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(cmd_mach_axis[i])));
    }

    const auto &act_mach_axis = coord_sys_->get_current_machine_axis_act();
    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        mach_act_axis_label_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(act_mach_axis[i])));
    }
}

void CoordPanel::slot_change_display_coord_index(uint32_t new_coord_index) {
    auto find_ret = coord_index2combobox_index_map_.find(new_coord_index);
    if (find_ret == coord_index2combobox_index_map_.end()) {
        s_logger->error("slot_change_display_coord_index: wrong index: {}",
                        new_coord_index);
        return;
    }
    ui->comboBox_select_coord_index->setCurrentIndex(find_ret->second);

    _change_display_coord_index(new_coord_index);
}

void CoordPanel::_init_label_arr() {
    axisname_label_arr_.at(0) = ui->lb_x;
    axisname_label_arr_.at(1) = ui->lb_y;
    axisname_label_arr_.at(2) = ui->lb_z;
    axisname_label_arr_.at(3) = ui->lb_b;
    axisname_label_arr_.at(4) = ui->lb_c;
    axisname_label_arr_.at(5) = ui->lb_a;

    cmd_axis_label_arr_.at(0) = ui->lb_x_cmd_axis;
    cmd_axis_label_arr_.at(1) = ui->lb_y_cmd_axis;
    cmd_axis_label_arr_.at(2) = ui->lb_z_cmd_axis;
    cmd_axis_label_arr_.at(3) = ui->lb_b_cmd_axis;
    cmd_axis_label_arr_.at(4) = ui->lb_c_cmd_axis;
    cmd_axis_label_arr_.at(5) = ui->lb_a_cmd_axis;

    act_axis_label_arr_.at(0) = ui->lb_x_act_axis;
    act_axis_label_arr_.at(1) = ui->lb_y_act_axis;
    act_axis_label_arr_.at(2) = ui->lb_z_act_axis;
    act_axis_label_arr_.at(3) = ui->lb_b_act_axis;
    act_axis_label_arr_.at(4) = ui->lb_c_act_axis;
    act_axis_label_arr_.at(5) = ui->lb_a_act_axis;

    mach_cmd_axis_label_arr_.at(0) = ui->lb_x_mach_cmd_axis;
    mach_cmd_axis_label_arr_.at(1) = ui->lb_y_mach_cmd_axis;
    mach_cmd_axis_label_arr_.at(2) = ui->lb_z_mach_cmd_axis;
    mach_cmd_axis_label_arr_.at(3) = ui->lb_b_mach_cmd_axis;
    mach_cmd_axis_label_arr_.at(4) = ui->lb_c_mach_cmd_axis;
    mach_cmd_axis_label_arr_.at(5) = ui->lb_a_mach_cmd_axis;

    mach_act_axis_label_arr_.at(0) = ui->lb_x_mach_act_axis;
    mach_act_axis_label_arr_.at(1) = ui->lb_y_mach_act_axis;
    mach_act_axis_label_arr_.at(2) = ui->lb_z_mach_act_axis;
    mach_act_axis_label_arr_.at(3) = ui->lb_b_mach_act_axis;
    mach_act_axis_label_arr_.at(4) = ui->lb_c_mach_act_axis;
    mach_act_axis_label_arr_.at(5) = ui->lb_a_mach_act_axis;

    // set some invalid
    for (std::size_t i = coord::Coordinate::Size;
         i < cmd_axis_label_arr_.size(); ++i) {
        axisname_label_arr_.at(i)->setEnabled(false);

        cmd_axis_label_arr_.at(i)->setText("null");
        cmd_axis_label_arr_.at(i)->setEnabled(false);
        act_axis_label_arr_.at(i)->setText("null");
        act_axis_label_arr_.at(i)->setEnabled(false);
        mach_cmd_axis_label_arr_.at(i)->setText("null");
        mach_cmd_axis_label_arr_.at(i)->setEnabled(false);
        mach_act_axis_label_arr_.at(i)->setText("null");
        mach_act_axis_label_arr_.at(i)->setEnabled(false);
    }
}

void CoordPanel::_init_coord_indexes() {
    auto avaiable_index_vec = coord_sys_->get_avaiable_coord_indexes();

    std::sort(avaiable_index_vec.begin(), avaiable_index_vec.end());

    for (const auto &index : avaiable_index_vec) {
        auto tmp = EDM_FMT::format("G{:02d}", index);
        ui->comboBox_select_coord_index->addItem(QString::fromStdString(tmp),
                                                 QVariant(index));

        // 建立坐标系索引到combobox索引的查找表
        coord_index2combobox_index_map_.emplace(
            index, ui->comboBox_select_coord_index->count() - 1);
    }

    coord_sys_->set_coord_index(
        ui->comboBox_select_coord_index->currentData().toUInt());
}

void CoordPanel::_init_connection() {
    QObject::connect(shared_core_data_->get_info_dispatcher(),
                     &InfoDispatcher::info_updated, this,
                     &CoordPanel::_update_info);

    QObject::connect(
        ui->comboBox_select_coord_index,
        QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [&](int index) {
            auto coord_index =
                ui->comboBox_select_coord_index->currentData().toUInt();
            this->_change_display_coord_index(coord_index);
        });
}

void CoordPanel::_update_info(const move::MotionInfo &info) {
    coord_sys_->update_motor_pos(info.curr_cmd_axis_blu,
                                 info.curr_act_axis_blu);

    update_axis_display();
}

void CoordPanel::_change_display_coord_index(uint32_t new_index) {
    coord_sys_->set_coord_index(new_index);

    update_all_display();
}

} // namespace app
} // namespace edm
