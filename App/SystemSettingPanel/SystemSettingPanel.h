#pragma once

#include <QWidget>

#include "ADCCalcPanel/ADCCalcPanel.h"

#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class SystemSettingPanel;
}

namespace edm {
namespace app {

class SystemSettingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SystemSettingPanel(SharedCoreData* shared_core_data, QWidget *parent = nullptr);
    ~SystemSettingPanel();

private:
    void _init_button_cb();

    void _do_save();

private:
    void _update_ui(); // 重置未保存的输入
    bool _save(); // 保存设定, 存入文件

    bool _set_motion_settings_to_motion_thread();

    void _set_adc_settings_to_zynq();

private:
    Ui::SystemSettingPanel *ui;

    SharedCoreData* shared_core_data_;

    ADCCalcPanel* adc_calc_panel_;
};

} // namespace app
} // namespace edm
