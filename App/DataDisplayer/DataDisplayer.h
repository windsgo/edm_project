#pragma once

#include <QWidget>
#include <qcolor.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qvector.h>
#include <unordered_map>

#include <qwt_plot_curve.h>
#include <qwt_legend.h>

namespace Ui {
class DataDisplayer;
}

namespace edm { 
namespace app {

struct DisplayedDataDesc {
    QString data_name;
    QString data_detail_desc;
    int data_max_points {-1}; // 设置为-1(<0的数), 跟随窗口设置点数
    QColor preferred_color {QColor(Qt::green)};

    int y_zero_offset{0}; // 显示时的y轴零点偏置
    int y_scale_percent{100}; // 显示时, 实际的坐标值为 (y + y_zero_offset) * y_scale / 100 * 100%

    bool visible{true};
};

class DataDisplayer : public QWidget
{
    Q_OBJECT

public:
    explicit DataDisplayer(QWidget *parent = nullptr);
    ~DataDisplayer();

    // 添加一个数据项目, 返回注册的编号(用于push数据时传入编号).
    // 如果失败, 返回-1 (<0)
    int add_data_item(const DisplayedDataDesc& data_desc);
    
    // 压入对应编号的一个数据
    bool push_data(int index, double v);

    // 刷新显示
    void update_display();

private:
    void _init_buttons();

    void _clear(); // TODO
    void _replot();

    void _update_settings_from_ui();
    void _resize_plotted_vector(QVector<double>& vec, int new_length);

    void _set_ui_data_desc_from_member_data_desc();


private:
    Ui::DataDisplayer *ui;

    // canvas show scale:
    int x_points_;
    double y_min_;
    double y_max_;

    bool enable_ {true}; // TODO
    
private: // member datas

    QVector<double> x_vec_;

    struct _DataInfo {
        struct DisplayedDataDesc data_desc;
        QVector<double> data_vec;
        QwtPlotCurve* curve;
    };

    void _update_curve_from_vec(_DataInfo& data);

    std::vector<_DataInfo> datas_;

    QwtLegend* legend_;
};

} // namespace app
} // namespace edm
