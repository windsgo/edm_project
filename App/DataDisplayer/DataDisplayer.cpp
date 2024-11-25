#include "DataDisplayer.h"
#include "qwt_axis.h"
#include "qwt_legend.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_scale_draw.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_widget.h"
#include "qwt_text.h"
#include "ui_DataDisplayer.h"
#include <QButtonGroup>
#include <cassert>
#include <qabstractbutton.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qvector.h>

#include "qwt_plot_grid.h"

namespace edm {
namespace app {

DataDisplayer::DataDisplayer(int x_points, QWidget *parent)
    : QWidget(parent), ui(new Ui::DataDisplayer) {
    ui->setupUi(this);

    // init plot
    ui->qwtPlot->enableAxis(QwtPlot::xBottom);
    ui->qwtPlot->enableAxis(QwtPlot::yLeft);
    ui->qwtPlot->enableAxis(QwtPlot::yRight);
    //    ui->qwtPlot->setAxisTitle(QwtPlot::xBottom, "x");
    //    ui->qwtPlot->setAxisTitle(QwtPlot::yLeft, "y");
    //    ui->qwtPlot->setAxisTitle(QwtPlot::yRight, "y");
    ui->qwtPlot->setAxisAutoScale(QwtPlot::xBottom, true);

    // init legend
    legend_ = new QwtLegend();
    ui->qwtPlot->insertLegend(legend_);

    // init grid
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(ui->qwtPlot);

    // init xpoints
    _set_xpoints(x_points);

    _init_buttons();
    _update_settings_from_ui();
    _replot();
}

int DataDisplayer::add_data_item(const DisplayedDataDesc &data_desc) {
    int next_index = datas_.size();

    _DataInfo data;
    data.data_desc = data_desc;

    // init vec
    data.raw_data_vec.resize(_get_plotting_points_count(data));
    data.raw_data_vec.fill(0.0);

    // init ui data if first (scale zero visible)
    _set_ui_data_desc_from_member_data_desc();

    // check yaxis
    if (data.data_desc.yAxis != QwtPlot::yLeft &&
        data.data_desc.yAxis != QwtPlot::yRight) {
        data.data_desc.yAxis = QwtPlot::yLeft;
    }

    // init curve
    data.curve = new QwtPlotCurve(data.data_desc.data_name);
    if (data.data_desc.visible) {
        data.curve->attach(ui->qwtPlot);
        data.curve->setYAxis(data.data_desc.yAxis);
        data.curve->setPen(data.data_desc.preferred_color,
                           data.data_desc.line_width);
    }

    // set curve init samples
    _update_curve_from_vec(data);

    datas_.push_back(data);

    ui->comboBox_item_select->addItem(data.data_desc.data_name);

    Q_ASSERT(ui->comboBox_item_select->count() == (int)datas_.size());
    return next_index;
}

bool DataDisplayer::push_data(int index, double v) {
    if (!data_input_enable_) {
        return false;
    }

    if (index < 0 || index >= (int)datas_.size()) {
        return false;
    }

    auto &data = datas_[index];

    data.raw_data_vec.pop_front();
    data.raw_data_vec.push_back(v);

    return true;
}

void DataDisplayer::update_display() {
    if (!this->isVisible()) {
        return;
    }

    _replot();
}

void DataDisplayer::set_axis_title(QwtPlot::Axis axis, const QString &title,
                                   QColor color) {
    QwtText t{title};
    t.setColor(color);
    ui->qwtPlot->setAxisTitle(axis, t);
}

void DataDisplayer::set_axis_scale(QwtPlot::Axis axis, double min, double max) {
    if (axis == QwtPlot::yLeft) {
        _set_yleft_scale(min, max);
    } else if (axis == QwtPlot::yRight) {
        _set_yright_scale(min, max);
    }
}

void DataDisplayer::clear() { emit ui->pb_clear->clicked(); }

void DataDisplayer::set_datainput_enable(bool enable) {
    _set_datainput_enable(enable);
    ui->pb_datainput_enable->setChecked(enable);
}

void DataDisplayer::set_plot_enable(bool enable) {
    _set_plot_enable(enable);
    ui->pb_plot_enable->setChecked(enable);
}

DataDisplayer::~DataDisplayer() { delete ui; }

void DataDisplayer::_init_buttons() {
    connect(ui->pb_update, &QPushButton::clicked, this, [this]() {
        _update_settings_from_ui();
        _replot();
    });

    connect(ui->pb_clear, &QPushButton::clicked, this, [this]() {
        _clear();
        _replot();
    });

    connect(ui->pb_datainput_enable, &QPushButton::clicked, this,
            [this](bool checked) { _set_datainput_enable(checked); });

    connect(ui->pb_plot_enable, &QPushButton::clicked, this,
            [this](bool checked) { _set_plot_enable(checked); });

    connect(ui->pb_item_visible, &QPushButton::clicked, this,
            [this](bool checked) {
                auto index = ui->comboBox_item_select->currentIndex();
                if (index < 0) {
                    return;
                }

                auto &data = datas_[index];

                if (!data.data_desc.visible && checked) {
                    data.curve->attach(ui->qwtPlot);
                    data.curve->setYAxis(data.data_desc.yAxis);
                } else if (data.data_desc.visible && !checked) {
                    data.curve->detach();
                }

                data.data_desc.visible = checked;

                _replot();
            });

    QButtonGroup *yleft_yright_group = new QButtonGroup(this);
    yleft_yright_group->addButton(ui->pb_select_yleft);
    yleft_yright_group->addButton(ui->pb_select_yright);
    yleft_yright_group->setExclusive(true);

    connect(ui->pb_select_yleft, &QPushButton::toggled, this,
            [this](bool checked) {
                auto index = ui->comboBox_item_select->currentIndex();
                if (index < 0) {
                    return;
                }

                auto &data = datas_[index];

                if (checked) {
                    data.curve->setYAxis(QwtPlot::yLeft);
                    data.data_desc.yAxis = QwtPlot::yLeft;
                } else {
                    data.curve->setYAxis(QwtPlot::yRight);
                    data.data_desc.yAxis = QwtPlot::yRight;
                }

                _replot();
            });

    connect(ui->comboBox_item_select,
            QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (index < 0)
                    return;

                _set_ui_data_desc_from_member_data_desc();
            });
}

void DataDisplayer::_set_xpoints(int x_points) {
    int old_x_points = x_points_;

    x_points_ = x_points;
    if (x_points_ < 1) {
        x_points_ = 1;
    }

    ui->sb_points->setValue(x_points_);

    if (old_x_points != x_points_) {
        _reset_x_vec(); // 重新设置x_vec_
        ui->qwtPlot->setAxisScale(QwtPlot::xBottom, 0, x_points_ - 1);

        for (auto &data : datas_) {
            _resize_data_vec(data); // 重新设置data数组的长度
        }
    }
}

void DataDisplayer::_set_yleft_scale(double min, double max) {
    // deal with left ymin ymax
    if (min < max) {
        left_y_min_ = min;
        left_y_max_ = max;
    }

    ui->sb_left_ymin->setValue(left_y_min_);
    ui->sb_left_ymax->setValue(left_y_max_);

    ui->qwtPlot->setAxisScale(QwtPlot::yLeft, left_y_min_, left_y_max_);
}

void DataDisplayer::_set_yright_scale(double min, double max) {
    // deal with left ymin ymax
    if (min < max) {
        right_y_min_ = min;
        right_y_max_ = max;
    }

    ui->sb_right_ymin->setValue(right_y_min_);
    ui->sb_right_ymax->setValue(right_y_max_);

    ui->qwtPlot->setAxisScale(QwtPlot::yRight, right_y_min_, right_y_max_);
}

void DataDisplayer::_update_label_data() {
    auto index = ui->comboBox_item_select->currentIndex();
    if (index < 0) {
        return;
    }

    auto &data = datas_[index];
    
    auto last_data = data.raw_data_vec.back();

    ui->lb_show_data->setText(QString::number(last_data));
}

void DataDisplayer::_clear() {
    for (auto &data : datas_) {
        data.raw_data_vec.fill(0.0);
    }
}

void DataDisplayer::_replot() {
    if (!plot_enable_) {
        return;
    }

    for (auto &data : datas_) {
        if (!data.data_desc.visible) {
            continue;
        }

        _update_curve_from_vec(data);
    }

    _update_label_data();

    ui->qwtPlot->replot();
}

void DataDisplayer::_update_settings_from_ui() {
    _set_xpoints(ui->sb_points->value());

    // get min and max from ui
    _set_yleft_scale(ui->sb_left_ymin->value(), ui->sb_left_ymax->value());
    _set_yright_scale(ui->sb_right_ymin->value(), ui->sb_right_ymax->value());
}

void DataDisplayer::_resize_plotted_vector(QVector<double> &vec,
                                           int new_length) {
    if (vec.size() == new_length || new_length <= 0) {
        return;
    }

    if (vec.size() > new_length) {
        // 删除vec靠前的一部分, 以达到new_length长度
        vec = vec.mid(vec.size() - new_length);

        Q_ASSERT(vec.size() == new_length);
    } else {
        // vec前面增加一部分0, 以达到new_length长度
        QVector<double> zero_vec;
        zero_vec.resize(new_length - vec.size());
        zero_vec.fill(0);

        vec = zero_vec + vec;

        Q_ASSERT(vec.size() == new_length);
    }
}

void DataDisplayer::_set_ui_data_desc_from_member_data_desc() {
    auto index = ui->comboBox_item_select->currentIndex();
    if (index < 0)
        return;

    const auto &data = datas_[index];

    ui->pb_select_yleft->setChecked(data.data_desc.yAxis == QwtPlot::yLeft);
    ui->pb_select_yright->setChecked(data.data_desc.yAxis == QwtPlot::yRight);

    ui->pb_item_visible->setChecked(data.data_desc.visible);
}

void DataDisplayer::_update_curve_from_vec(DataDisplayer::_DataInfo &data) {
    int plotting_size = _get_plotting_points_count(data);
    int x_vec_start_offset = x_points_ - plotting_size;
    Q_ASSERT(x_vec_start_offset >= 0 && x_vec_start_offset < x_points_);
    Q_ASSERT(data.raw_data_vec.size() == plotting_size);

    data.curve->setSamples(x_vec_.data() + x_vec_start_offset,
                           data.raw_data_vec.data(), plotting_size);
}

void DataDisplayer::_resize_data_vec(DataDisplayer::_DataInfo &data) {
    int plotting_size = _get_plotting_points_count(data);
    if (plotting_size == data.raw_data_vec.size()) {
        return;
    }

    _resize_plotted_vector(data.raw_data_vec, plotting_size);

    Q_ASSERT(data.raw_data_vec.size() == plotting_size);
}

void DataDisplayer::_reset_x_vec() {
    if (x_points_ == x_vec_.size()) {
        return;
    }

    x_vec_.resize(x_points_);
    for (int i = 0; i < x_points_; ++i) {
        x_vec_[i] = i;
    }
}

int DataDisplayer::_get_plotting_points_count(
    const DataDisplayer::_DataInfo &data) const {
    if (data.data_desc.data_max_points <= 0) {
        return x_points_;
    } else {
        if (data.data_desc.data_max_points > x_points_) {
            return x_points_;
        } else {
            return data.data_desc.data_max_points;
        }
    }
}

} // namespace app
} // namespace edm
