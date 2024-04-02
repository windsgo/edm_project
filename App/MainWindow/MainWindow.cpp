#include "MainWindow.h"
#include "DataQueueRecordPanel/DataQueueRecordPanel.h"
#include "ui_MainWindow.h"

#include <QFile>
#include <QMessageBox>
#include <qgridlayout.h>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    _init_members();
    _init_status_bar_palette_and_connection();

    test_latency_timer_ = new QTimer(this);
    connect(test_latency_timer_, &QTimer::timeout, this, [this]() {
        const auto &i =
            this->shared_core_data_->get_info_dispatcher()->get_info();

        ui->le_test_latency_curr->setText(
            QString::number(i.latency_data.curr_latency));
        ui->le_test_latency_min->setText(
            QString::number(i.latency_data.min_latency));
        ui->le_test_latency_avg->setText(
            QString::number(i.latency_data.avg_latency));
        ui->le_test_latency_max->setText(
            QString::number(i.latency_data.max_latency));

        ui->le_timeuse_ecatavg->setText(
            QString::number(i.time_use_data.ecat_time_use_avg));
        ui->le_timeuse_infoavg->setText(
            QString::number(i.time_use_data.info_time_use_avg));
        ui->le_timeuse_smavg->setText(
            QString::number(i.time_use_data.statemachine_time_use_avg));
        ui->le_timeuse_totalavg->setText(
            QString::number(i.time_use_data.total_time_use_avg));
    });
    test_latency_timer_->start(200);

    connect(ui->pb_clear_statdata, &QPushButton::clicked, this, [this]() {
        auto cmd = std::make_shared<move::MotionCommandSettingClearStatData>();
        this->shared_core_data_->get_motion_cmd_queue()->push_command(cmd);
    });
}

MainWindow::~MainWindow() {
    s_logger->info("-----------------------------------------");
    s_logger->info("------------ MainWindow Dtor ------------");
    s_logger->info("-----------------------------------------");

    delete ui;
}

void MainWindow::_init_members() {
    shared_core_data_ = new SharedCoreData(this);

    task_manager_ = new task::TaskManager(shared_core_data_, this);

    auto coord_layout = new QGridLayout(ui->widget_coord);
    coord_panel_ = new CoordPanel(shared_core_data_);
    coord_layout->addWidget(coord_panel_);

    auto coord_setting_layout = new QGridLayout(ui->tab_coord_setting);
    coord_setting_panel_ = new CoordSettingPanel(shared_core_data_);
    coord_setting_layout->addWidget(coord_setting_panel_);

    auto info_layout = new QGridLayout(ui->widget_info);
    info_panel_ = new InfoPanel(shared_core_data_);
    info_layout->addWidget(info_panel_);

    auto move_layout = new QGridLayout(ui->widget_pointmove);
    move_panel_ = new MovePanel(shared_core_data_, task_manager_);
    move_layout->addWidget(move_panel_);

    auto io_layout = new QGridLayout(ui->tab_io);
    io_panel_ = new IOPanel(shared_core_data_);
    io_layout->addWidget(io_panel_);

    auto power_layout = new QGridLayout(ui->tab_power);
    power_panel_ = new PowerPanel(shared_core_data_);
    power_layout->addWidget(power_panel_);

    auto gcode_layout = new QGridLayout(ui->widget_gcode);
    gcode_panel_ = new GCodePanel(shared_core_data_, task_manager_);
    gcode_layout->addWidget(gcode_panel_);

    auto testpanel_layout = new QGridLayout(ui->groupBox_test);
    test_panel_ = new TestPanel(shared_core_data_);
    testpanel_layout->addWidget(test_panel_);

    auto system_setting_panel_layout = new QGridLayout(ui->tab_system_setting);
    system_setting_panel_ = new SystemSettingPanel();
    system_setting_panel_layout->addWidget(system_setting_panel_);

    auto dqr_panel_layout = new QGridLayout(ui->tab_record);
    dqr_panel_ = new DataQueueRecordPanel(shared_core_data_);
    dqr_panel_layout->addWidget(dqr_panel_);

    connect(task_manager_, &task::TaskManager::sig_switch_coordindex,
            coord_panel_, &CoordPanel::slot_change_display_coord_index);


    // test
    auto test_data_displayer_layout = new QGridLayout(ui->tab_testdisplay);
    test_data_displayer_ = new DataDisplayer();
    test_data_displayer_layout->addWidget(test_data_displayer_);

    DisplayedDataDesc desc;
    desc.data_name = "data0";
    desc.yAxis = QwtPlot::yLeft;
    desc.visible = true;
    desc.data_max_points = 300;
    desc.preferred_color = Qt::green;
    data_index0_ = test_data_displayer_->add_data_item(desc);
    desc.data_name = "data1";
    desc.yAxis = QwtPlot::yRight;
    desc.visible = true;
    desc.data_max_points = -1;
    desc.preferred_color = Qt::red;
    data_index1_ = test_data_displayer_->add_data_item(desc);

    display_timer_ = new QTimer(this);
    connect(display_timer_, &QTimer::timeout, this, [this]() {
        static int i = 0;
        ++i;

        test_data_displayer_->push_data(data_index0_, sin((double)i * 2.0 * 3.14159265 / 100) * 50 + 50);
        test_data_displayer_->push_data(data_index1_, cos((double)i * 2.0 * 3.14159265 / 100));
        test_data_displayer_->update_display();
    });
    display_timer_->start(40);
}

void MainWindow::_init_status_bar_palette_and_connection() {
    status_bar_info_palette_ = statusBar()->palette();
    status_bar_info_palette_.setColor(QPalette::WindowText, Qt::black);

    status_bar_warn_palette_ = statusBar()->palette();
    status_bar_warn_palette_.setColor(QPalette::WindowText, QColor("#FFB90F"));

    status_bar_error_palette_ = statusBar()->palette();
    status_bar_error_palette_.setColor(QPalette::WindowText, Qt::red);

    connect(this->shared_core_data_, &SharedCoreData::sig_info_message, this,
            &MainWindow::slot_info_message);
    connect(this->shared_core_data_, &SharedCoreData::sig_warn_message, this,
            &MainWindow::slot_warn_message);
    connect(this->shared_core_data_, &SharedCoreData::sig_error_message, this,
            &MainWindow::slot_error_message);
}

void MainWindow::slot_info_message(const QString &str, int timeout) {
    statusBar()->setPalette(status_bar_info_palette_);
    statusBar()->showMessage(str, timeout);

    auto f = statusBar()->font();
    f.setBold(false);
    statusBar()->setFont(f);
}

void MainWindow::slot_warn_message(const QString &str, int timeout) {
    statusBar()->setPalette(status_bar_warn_palette_);
    statusBar()->showMessage(str, timeout);

    auto f = statusBar()->font();
    f.setBold(true);
    statusBar()->setFont(f);
}

void MainWindow::slot_error_message(const QString &str, int timeout) {
    statusBar()->setPalette(status_bar_error_palette_);
    statusBar()->showMessage(str, timeout);

    auto f = statusBar()->font();
    f.setBold(true);
    statusBar()->setFont(f);
}

} // namespace app
} // namespace edm
