#pragma once

#include <QDebug>
#include <QString>
#include <QWidget>

#include <QTableView>

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlResult>
#include <QSqlTableModel>

#include "QtDependComponents/PowerController/EleparamDefine.h"

#include <optional>

namespace Ui {
class PowerDatabase;
}

namespace edm {
namespace app {

class PowerDatabase : public QWidget {
    Q_OBJECT

public:
    explicit PowerDatabase(QWidget *parent = nullptr);
    ~PowerDatabase();

public: // 接口
    // 根据电参数号, 获取电参数结构体
    // 如果index存在, 数据库会创建一个新的对应于index的电参数结构体并返回true
    // 如果index不存在, 返回false
    bool get_eleparam_from_index(uint32_t index,
                                 power::EleParam_dkd_t &output) const;

    // 获取是否存在某一个电参数号的信息, 每次调用都会触发数据库搜索
    bool exist_index(uint32_t index) const;

signals:
    // 在数据库窗口中选定了一行电参数, 并按下了 Select Param 按钮,
    // 触发此信号告诉 PowerPanel 需要更新 "当前电参数", 并发送到电源和io
    void sig_select_param(uint32_t eleparam_index);

private:
    void _init_database();
    void _init_button_slots();

    bool _check_table_exist(
        const QString &table_name) const;     // 检查eleparam表是否存在
    bool _create_table_with_default_params(); // 创建默认电参数数据表

    bool _check_table_valid() const; // 检查表格各列是否正确
    bool _drop_table(const QString &table_name); // 删除表格

private: // 表格编辑操作 槽函数
    void _slot_tableedit_confirm();
    void _slot_tableedit_add();
    void _slot_tableedit_cancel();
    void _slot_tableedit_delete();

    // 选定电参数
    void _slot_choose_eleparam_index();

    // 更新表格显示
    void _slot_table_refresh();

private:
    void _set_table_editable(bool editable);
    void _update_tableview();

private:
    static void _get_eleparam_from_record(const QSqlRecord &record,
                                          power::EleParam_dkd_t &output);

private:
    Ui::PowerDatabase *ui;

    QSqlDatabase database_;       // 数据库对象
    QSqlTableModel *table_model_; // 数据表模型

private:
    // 数据库页面编辑状态
    // bool enable_edit_{false};

private:
    const QString EleTableName{"eleparam"};
    constexpr static const auto EditTrigger =
        QTableView::EditTrigger::DoubleClicked;
};

} // namespace app
} // namespace edm
