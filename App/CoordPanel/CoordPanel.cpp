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
    _init_offset_button_cb();
    _init_softlimit_button_cb();

    update_all_display();
}

CoordPanel::~CoordPanel() { delete ui; }

void CoordPanel::update_all_display() {
    update_axis_display();
    update_offset_display();
    update_softlimit_display();
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

void CoordPanel::update_offset_display() {
    auto coord_offset = coord_sys_->get_current_coord_offset();
    if (coord_offset) {
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            coord_offset_le_arr_[i]->setText(
                InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                    util::UnitConverter::blu2mm((*coord_offset)[i])));
        }
    } else {
        s_logger->warn("{}: get_current_coord_offset failed",
                       __PRETTY_FUNCTION__);
    }

    const auto &global_offset = coord_sys_->get_current_global_offset();
    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        global_offset_le_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(global_offset[i])));
    }
}

void CoordPanel::update_softlimit_display() {
    const auto &sl_pos = coord_sys_->get_pos_soft_limit();
    const auto &sl_neg = coord_sys_->get_neg_soft_limit();

    for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
        softlimit_pos_le_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(sl_pos[i])));
        softlimit_neg_le_arr_[i]->setText(
            InputHelper::MMDoubleToExplictlyPosNegFormatedQString(
                util::UnitConverter::blu2mm(sl_neg[i])));
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

    offsetaxisname_label_arr_.at(0) = ui->lb_ofs_x;
    offsetaxisname_label_arr_.at(1) = ui->lb_ofs_y;
    offsetaxisname_label_arr_.at(2) = ui->lb_ofs_z;
    offsetaxisname_label_arr_.at(3) = ui->lb_ofs_b;
    offsetaxisname_label_arr_.at(4) = ui->lb_ofs_c;
    offsetaxisname_label_arr_.at(5) = ui->lb_ofs_a;

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

    coord_offset_le_arr_.at(0) = ui->le_coord_offset_x;
    coord_offset_le_arr_.at(1) = ui->le_coord_offset_y;
    coord_offset_le_arr_.at(2) = ui->le_coord_offset_z;
    coord_offset_le_arr_.at(3) = ui->le_coord_offset_b;
    coord_offset_le_arr_.at(4) = ui->le_coord_offset_c;
    coord_offset_le_arr_.at(5) = ui->le_coord_offset_a;

    global_offset_le_arr_.at(0) = ui->le_global_offset_x;
    global_offset_le_arr_.at(1) = ui->le_global_offset_y;
    global_offset_le_arr_.at(2) = ui->le_global_offset_z;
    global_offset_le_arr_.at(3) = ui->le_global_offset_b;
    global_offset_le_arr_.at(4) = ui->le_global_offset_c;
    global_offset_le_arr_.at(5) = ui->le_global_offset_a;

    for (auto &le : coord_offset_le_arr_) {
        InputHelper::SetLineeditCoordValidator(le, this);
    }

    for (auto &le : global_offset_le_arr_) {
        InputHelper::SetLineeditCoordValidator(le, this);
    }

    softlimit_pos_le_arr_.at(0) = ui->le_sl_pos_x;
    softlimit_pos_le_arr_.at(1) = ui->le_sl_pos_y;
    softlimit_pos_le_arr_.at(2) = ui->le_sl_pos_z;
    softlimit_pos_le_arr_.at(3) = ui->le_sl_pos_b;
    softlimit_pos_le_arr_.at(4) = ui->le_sl_pos_c;
    softlimit_pos_le_arr_.at(5) = ui->le_sl_pos_a;

    softlimit_neg_le_arr_.at(0) = ui->le_sl_neg_x;
    softlimit_neg_le_arr_.at(1) = ui->le_sl_neg_y;
    softlimit_neg_le_arr_.at(2) = ui->le_sl_neg_z;
    softlimit_neg_le_arr_.at(3) = ui->le_sl_neg_b;
    softlimit_neg_le_arr_.at(4) = ui->le_sl_neg_c;
    softlimit_neg_le_arr_.at(5) = ui->le_sl_neg_a;

    // set some invalid
    for (std::size_t i = coord::Coordinate::Size;
         i < cmd_axis_label_arr_.size(); ++i) {
        axisname_label_arr_.at(i)->setEnabled(false);
        offsetaxisname_label_arr_.at(i)->setEnabled(false);

        cmd_axis_label_arr_.at(i)->setText("null");
        cmd_axis_label_arr_.at(i)->setEnabled(false);
        act_axis_label_arr_.at(i)->setText("null");
        act_axis_label_arr_.at(i)->setEnabled(false);
        mach_cmd_axis_label_arr_.at(i)->setText("null");
        mach_cmd_axis_label_arr_.at(i)->setEnabled(false);
        mach_act_axis_label_arr_.at(i)->setText("null");
        mach_act_axis_label_arr_.at(i)->setEnabled(false);

        coord_offset_le_arr_.at(i)->setEnabled(false);
        coord_offset_le_arr_.at(i)->setText("null");
        global_offset_le_arr_.at(i)->setEnabled(false);
        global_offset_le_arr_.at(i)->setText("null");

        softlimit_pos_le_arr_.at(i)->setEnabled(false);
        softlimit_pos_le_arr_.at(i)->setText("null");
        softlimit_neg_le_arr_.at(i)->setEnabled(false);
        softlimit_neg_le_arr_.at(i)->setText("null");
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

void CoordPanel::_init_offset_button_cb() {
    ui->pb_coord_offset_clear->setToolTip(
        QStringLiteral("Set Coordinate Offsets to ZERO"));

    ui->pb_global_offset_clear->setToolTip(
        QStringLiteral("Set Global Offsets to ZERO"));

    ui->pb_coord_offset_set->setToolTip(
        QStringLiteral("Set Coordinate Offsets by given value upside"));

    ui->pb_global_offset_set->setToolTip(
        QStringLiteral("Set Coordinate Offsets by given value upside"));

    ui->pb_coord_offset_setorigin->setToolTip(
        QStringLiteral("Set current point as the ORIGIN of the coordinate"));

    ui->pb_global_offset_setorigin->setToolTip(QStringLiteral(
        "Set current point as the ORIGIN of machine coord (global)"));

    connect(ui->pb_coord_offset_clear, &QPushButton::clicked, this, [this]() {
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            this->coord_offset_le_arr_[i]->setText("+0.0000");
        }

        this->coord_sys_->set_current_coord_offset(coord::coord_offset_t{0.0});

        this->update_all_display();
    });

    connect(ui->pb_global_offset_clear, &QPushButton::clicked, this, [this]() {
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            this->global_offset_le_arr_[i]->setText("+0.0000");
        }

        this->coord_sys_->set_global_offset(coord::coord_offset_t{0.0});

        this->update_all_display();
    });

    connect(ui->pb_coord_offset_set, &QPushButton::clicked, this, [this]() {
        coord::coord_offset_t offsets{0.0};
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            auto v = InputHelper::QStringToDouble(
                this->coord_offset_le_arr_[i]->text());
            if (!v) {
                s_logger->error("pb_coord_offset_set: toDouble Failed, i = {}",
                                i);
                return;
            } else {
                offsets[i] = util::UnitConverter::mm2blu(*v);
            }
        }

        this->coord_sys_->set_current_coord_offset(offsets);

        this->update_all_display();
    });

    connect(ui->pb_global_offset_set, &QPushButton::clicked, this, [this]() {
        coord::coord_offset_t offsets{0.0};
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            auto v = InputHelper::QStringToDouble(
                this->global_offset_le_arr_[i]->text());
            if (!v) {
                s_logger->error("pb_global_offset_set: toDouble Failed, i = {}",
                                i);
                return;
            } else {
                offsets[i] = util::UnitConverter::mm2blu(*v);
            }
        }

        this->coord_sys_->set_global_offset(offsets);

        this->update_all_display();
    });

    connect(ui->pb_coord_offset_setorigin, &QPushButton::clicked, this,
            [this]() {
                coord::coord_offset_t offsets{0.0};
                const auto &curr_mach_cmd_axis =
                    this->coord_sys_->get_current_machine_axis();
                for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                    offsets[i] = curr_mach_cmd_axis[i];
                }

                this->coord_sys_->set_current_coord_offset(offsets);

                this->update_all_display();
            });

    connect(ui->pb_global_offset_setorigin, &QPushButton::clicked, this,
            [this]() {
                coord::coord_offset_t offsets{0.0};
                const auto &curr_motor_cmd_axis =
                    this->coord_sys_->get_current_motor_axis();
                for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
                    offsets[i] = curr_motor_cmd_axis[i];
                }

                this->coord_sys_->set_global_offset(offsets);

                this->update_all_display();
            });
}

void CoordPanel::_init_softlimit_button_cb() {
    connect(ui->pb_sl_set, &QPushButton::clicked, this, [this]() {
        coord::CoordSoftLimit csl;
        for (std::size_t i = 0; i < coord::Coordinate::Size; ++i) {
            auto vp = InputHelper::QStringToDouble(
                this->softlimit_pos_le_arr_[i]->text());
            if (!vp) {
                s_logger->error("pb_sl_set: toDouble Failed, pos, i = {}", i);
                return;
            } else {
                csl.pos[i] = util::UnitConverter::mm2blu(*vp);
            }

            auto vn = InputHelper::QStringToDouble(
                this->softlimit_neg_le_arr_[i]->text());
            if (!vn) {
                s_logger->error("pb_sl_set: toDouble Failed, neg, i = {}", i);
                return;
            } else {
                csl.neg[i] = util::UnitConverter::mm2blu(*vn);
            }
        }

        auto set_csl_ret = this->coord_sys_->set_soft_limits(csl);
        if (!set_csl_ret) {
            s_logger->error("set_soft_limits failed");
            auto msgbx_ret = QMessageBox::critical(
                this, QStringLiteral("Set Softlimits Failed"),
                QStringLiteral("Set Softlimits Failed.\n Do you need to reset "
                               "softlimits?"),
                QMessageBox::StandardButton::Yes,
                QMessageBox::StandardButton::No);
            if (msgbx_ret == QMessageBox::StandardButton::Yes) {
                this->update_softlimit_display();
            }
        } else {
            this->update_softlimit_display();
        }
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
