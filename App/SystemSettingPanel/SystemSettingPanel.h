#pragma once

#include <QWidget>

#include "SystemSettings/SystemSettings.h"

namespace Ui {
class SystemSettingPanel;
}

namespace edm {
namespace app {

class SystemSettingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SystemSettingPanel(QWidget *parent = nullptr);
    ~SystemSettingPanel();

private:
    void _init_button_cb();

private:
    void _update_ui(); // 重置未保存的输入
    bool _save(); // 保存设定, 存入文件

private:
    Ui::SystemSettingPanel *ui;
};

} // namespace app
} // namespace edm
