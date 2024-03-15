#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <unordered_map>

#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class PowerPanel;
}

namespace edm {
namespace app {

class PowerPanel : public QWidget {
    Q_OBJECT

public:
    explicit PowerPanel(SharedCoreData *shared_core_data,
                        QWidget *parent = nullptr);
    ~PowerPanel();

    // 主要是更新3个单独的开关按钮
    void slot_update_io_display();

signals:
    // 触发io显示信号, 表示这里的操作可能触发io变化, 需要io显示模块更新io显示
    // 连接到: 界面中存在io显示的模块的更新槽 
    void sig_trigger_io_display_update();

private:
    void _init_button_slots();

private:
    Ui::PowerPanel *ui;

    SharedCoreData *shared_core_data_;

    power::PowerController::ptr power_ctrler_;
};

} // namespace app
} // namespace edm
