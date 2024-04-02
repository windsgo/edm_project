#include "DataDisplayer.h"
#include "qwt_legend.h"
#include "qwt_plot_curve.h"
#include "ui_DataDisplayer.h"
#include <cassert>
#include <qcombobox.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qvector.h>

#include "qwt_plot_grid.h"

namespace edm {
namespace app {

DataDisplayer::DataDisplayer(QWidget *parent)
    : QWidget(parent), ui(new Ui::DataDisplayer) {
    ui->setupUi(this);

    // init title
    ui->qwtPlot->setAxisTitle(QwtPlot::xBottom, "x");
    ui->qwtPlot->setAxisTitle(QwtPlot::yLeft, "y");
    ui->qwtPlot->setAxisAutoScale(QwtPlot::xBottom, true);
    ui->qwtPlot->setStyleSheet("color: white;");

    // init legend
    legend_ = new QwtLegend();
    ui->qwtPlot->insertLegend(legend_);

    // init grid
    QwtPlotGrid* grid = new QwtPlotGrid();
    grid->setPen(Qt::gray, 0.0, Qt::DotLine);
    grid->enableX(true);
    grid->enableY(true);
    grid->attach(ui->qwtPlot);

    _init_buttons();
    _update_settings_from_ui();
    _replot();
}

int DataDisplayer::add_data_item(const DisplayedDataDesc &data_desc) {
    int next_index = datas_.size();

    _DataInfo data;
    data.data_desc = data_desc;

    // init vec
    if (data.data_desc.data_max_points <= 0) {
        data.data_vec.resize(x_points_);
    } else {
        if (data.data_desc.data_max_points > x_points_) {
            data.data_desc.data_max_points = x_points_;
        }

        data.data_vec.resize(data.data_desc.data_max_points);
    }
    data.data_vec.fill(0.0);

    if (data.data_desc.y_scale_percent <= 0 ||
        data.data_desc.y_scale_percent >
            ui->verticalSlider_scale_percent->maximum()) {
        data.data_desc.y_scale_percent = 100;
    }
    if (data.data_desc.y_zero_offset <
            ui->verticalSlider_zero_offset->minimum() ||
        data.data_desc.y_zero_offset >
            ui->verticalSlider_zero_offset->maximum()) {
        data.data_desc.y_zero_offset = 0;
    }

    ui->comboBox_item_select->addItem(data.data_desc.data_name);

    // init ui data if first (scale zero visible)
    _set_ui_data_desc_from_member_data_desc();

    // init curve
    data.curve = new QwtPlotCurve(data.data_desc.data_name);
    if (data.data_desc.visible) {
        data.curve->attach(ui->qwtPlot);
        data.curve->setPen(data.data_desc.preferred_color);
    }

    // set curve init samples
    _update_curve_from_vec(data);

    datas_.push_back(data);
    Q_ASSERT(ui->comboBox_item_select->maxCount() == datas_.size());
    return next_index;
}

bool DataDisplayer::push_data(int index, double v) {
    if (!enable_) {
        return false;
    }

    if (index < 0 || index >= datas_.size()) {
        return false;
    }

    auto& data = datas_[index];

    data.data_vec.pop_front();
    data.data_vec.push_back(v);

    return true;
}

void DataDisplayer::update_display() {
    if (!this->isVisible()) {
        return;
    }

    _replot();
}

DataDisplayer::~DataDisplayer() { delete ui; }

void DataDisplayer::_init_buttons() {
    connect(ui->pb_update, &QPushButton::clicked, this, [this]() {
        _update_settings_from_ui();
        _replot();
    });

    connect(ui->pb_item_visible, &QPushButton::clicked, this, [this](bool checked) {
        auto index = ui->comboBox_item_select->currentIndex();
        if (index < 0) {
            return;
        }

        auto& data = datas_[index];

        if (!data.data_desc.visible && checked) {
            data.curve->attach(ui->qwtPlot);
        } else if (data.data_desc.visible && !checked) {
            data.curve->detach();
        }

        data.data_desc.visible = checked;

        _replot();
    });

    connect(ui->comboBox_item_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) return;

        _set_ui_data_desc_from_member_data_desc();
    }); 

    connect(ui->verticalSlider_zero_offset, &QSlider::valueChanged, this, [this](int value) {
        auto index = ui->comboBox_item_select->currentIndex();
        if (index < 0) {
            return;
        }
        auto& data = datas_[index];

        data.data_desc.y_zero_offset = value;
    });

    connect(ui->verticalSlider_scale_percent, &QSlider::valueChanged, this, [this](int value) {
        auto index = ui->comboBox_item_select->currentIndex();
        if (index < 0) {
            return;
        }
        auto& data = datas_[index];

        data.data_desc.y_scale_percent = value;
    });
}

void DataDisplayer::_replot() {
    if (!enable_) {
        return;
    }

    for (auto& data : datas_) {
        if (!data.data_desc.visible) {
            continue;
        }

        _update_curve_from_vec(data);
    }

    ui->qwtPlot->replot();
}

void DataDisplayer::_update_settings_from_ui() {
    int old_x_points = x_points_;

    x_points_ = ui->sb_points->value();
    if (x_points_ < 1) {
        x_points_ = 1;
        ui->sb_points->setValue(x_points_);
    }

    if (old_x_points != x_points_) {
        _resize_plotted_vector(x_vec_, x_points_);
        ui->qwtPlot->setAxisScale(QwtPlot::xBottom, 0, x_points_ - 1);

        for (auto &data : datas_) {
            if (data.data_desc.data_max_points <= 0 ||
                data.data_desc.data_max_points > x_points_) {
                _resize_plotted_vector(data.data_vec, x_points_);
            } else {
                _resize_plotted_vector(data.data_vec,
                                       data.data_desc.data_max_points);
            }
        }
    }

    // deal with ymin ymax
    if (ui->sb_ymin->value() >= ui->sb_ymax->value()) {
        // reset min and max
        ui->sb_ymin->setValue(y_min_);
        ui->sb_ymax->setValue(y_max_);
    }

    // get min and max from ui
    y_min_ = ui->sb_ymin->value();
    y_max_ = ui->sb_ymax->value();

    ui->qwtPlot->setAxisScale(QwtPlot::yLeft, y_min_, y_max_);
}

void DataDisplayer::_resize_plotted_vector(QVector<double> &vec,
                                           int new_length) {
    if (vec.size() == new_length || new_length <= 0) {
        return;
    }

    if (vec.size() > new_length) {
        // 删除vec靠前的一部分, 以达到new_length长度
        vec = vec.mid(vec.size() - new_length);
    } else {
        // vec前面增加一部分0, 以达到new_length长度
        QVector<double> zero_vec;
        zero_vec.resize(new_length - vec.size());
        zero_vec.fill(0);

        vec = zero_vec + vec;
    }
}

void DataDisplayer::_set_ui_data_desc_from_member_data_desc() {
    auto index = ui->comboBox_item_select->currentIndex();
    if (index < 0) return;

    const auto& data = datas_[index];

    ui->verticalSlider_zero_offset->setValue(data.data_desc.y_zero_offset);
    ui->verticalSlider_scale_percent->setValue(data.data_desc.y_scale_percent);

    ui->pb_item_visible->setChecked(data.data_desc.visible);
}

void DataDisplayer::_update_curve_from_vec(DataDisplayer::_DataInfo& data) {
    if (data.data_desc.data_max_points <= 0) {
        data.curve->setSamples(x_vec_, data.data_vec);  
    } else {
        data.curve->setSamples(x_vec_.mid(x_points_ - data.data_desc.data_max_points), data.data_vec);
    }
}

} // namespace app
} // namespace edm
