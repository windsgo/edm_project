#ifndef TESTPOWERWIDGET_H
#define TESTPOWERWIDGET_H

#include <QWidget>

#include "Logger/LogMacro.h"
#include "QtDependComponents/CanController/CanController.h"
#include "QtDependComponents/IOController/IOController.h"
#include "QtDependComponents/PowerController/PowerController.h"
#include "config.h"

#include <QGridLayout>
#include <QPushButton>

#include <QTimer>

#include <QLineEdit>
#include <QMap>
#include <QVector>

using namespace edm;

namespace Ui {
class TestPowerWidget;
}

class TestPowerWidget : public QWidget {
    Q_OBJECT

public:
    explicit TestPowerWidget(QWidget *parent = nullptr);
    ~TestPowerWidget();

private:
    void _init_system();

    void _add_io_button(const QString &io_name, uint32_t out_num, int row,
                        int col);
    void _init_io_buttons();
    void _init_common_buttons();

private:
    void _button_clicked(uint32_t io_num, bool clicked);
    void _update_widget_status();

    void _update_eleparam_from_ui_and_send();

private:
    can::CanController::ptr s_can_ctrler{nullptr};
    io::IOController::ptr s_io_ctrler{nullptr};
    power::PowerController::ptr s_power_ctrler{nullptr};

private:
    Ui::TestPowerWidget *ui;

    QTimer *ioboard_pulse_timer_;
    QTimer *ele_cycle_timer_;

    power::EleParam_dkd_t::ptr curr_eleparam_{nullptr};

    QMap<uint32_t, QPushButton *> io_button_map_;
    QMap<QPushButton *, uint32_t> button_io_map_;
    QGridLayout *io_button_layout_{nullptr};

    QGridLayout *eleparam_frame_layout_{nullptr};
};

#endif // TESTPOWERWIDGET_H
