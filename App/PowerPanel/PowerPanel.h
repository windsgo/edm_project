#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QTimer>

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

public:
    void slot_update_eleparam_display(const edm::power::EleParam_dkd_t& eleparam);
    void slot_update_io_display();

private:
    // 主要是更新3个单独的开关按钮
    void _update_io_display();
    void _update_eleparam_display(const edm::power::EleParam_dkd_t& eleparam);

private:
    void _init_update_slots();
    void _init_button_slots();

private:
    Ui::PowerPanel *ui;

    SharedCoreData *shared_core_data_;

    PowerManager *pm_;

    QTimer* update_io_timer_; // 用于周期性地刷新面板上的3个io状态(尤其是)
};

} // namespace app
} // namespace edm
