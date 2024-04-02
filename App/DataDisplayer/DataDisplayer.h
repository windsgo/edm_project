#pragma once

#include <QWidget>
#include <qcolor.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qvector.h>
#include <unordered_map>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>

namespace Ui {
class DataDisplayer;
}

namespace edm { 
namespace app {

struct DisplayedDataDesc {
    QString data_name;
    int data_max_points {-1}; // 设置为-1(<0的数), 跟随窗口设置点数
    QColor preferred_color {QColor(Qt::green)};

    bool visible{true};

    QwtPlot::Axis yAxis{QwtPlot::yLeft};
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

    // 重新绘图: 从各个data的vec更新curve的sample, 并重新绘图
    void _replot();

    // 从ui获取绘图参数(x轴点数, y轴最大,最小值)保存到成员变量, 但是不重新绘图, 需要手动调用_replot
    void _update_settings_from_ui();

    // 调整vec的长度, 保持尾端最新数据不被擦除, 擦除和新增0数据都在数组前端进行
    void _resize_plotted_vector(QVector<double>& vec, int new_length);

    // 根据ui中combobox选定的数据index, 从成员数据描述中, 更新ui显示的数据设定(visible, ofs, scale)
    void _set_ui_data_desc_from_member_data_desc();


private:
    Ui::DataDisplayer *ui;

    // canvas show scale:
    int x_points_;
    double left_y_min_;
    double left_y_max_;
    double right_y_min_;
    double right_y_max_;

    bool enable_ {true}; // TODO
    
private: // member datas

    QVector<double> x_vec_;

    struct _DataInfo {
        struct DisplayedDataDesc data_desc;

        QVector<double> raw_data_vec;

        QwtPlotCurve* curve;
    };

    std::vector<_DataInfo> datas_;

    QwtLegend* legend_;

private:
    // 从data的data_vec数组, 更新curve的数据
    void _update_curve_from_vec(_DataInfo& data);

    // 根据x_points_, data的max_points设定, 对data_vec进行resize
    void _resize_data_vec(_DataInfo& data);

    // 根据x_points, 设定x_vec_
    void _reset_x_vec();

    // 获取当前实际的可绘制点数(考虑data设置的max点数, 以及当前x_points_指定的总点数, 总点数可能会小于data设置的max点数)
    int _get_plotting_points_count(const _DataInfo& data) const;
};

} // namespace app
} // namespace edm
