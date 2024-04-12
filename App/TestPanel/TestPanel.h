#pragma once

#include <QWidget>
#include "SharedCoreData/SharedCoreData.h"

namespace Ui {
class TestPanel;
}

namespace edm {
namespace app {

class TestPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TestPanel(SharedCoreData* shared_core_data, QWidget *parent = nullptr);
    ~TestPanel();

private:
    void _update_phy_touchdetect();
    void _update_servo();

    void _update_manual_voltage();

private:
    Ui::TestPanel *ui;

    SharedCoreData* shared_core_data_;
};

} // namespace app
} // namespace edm
