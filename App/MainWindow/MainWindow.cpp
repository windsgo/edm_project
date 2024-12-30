#include "MainWindow.h"
#include "CanReceiveBuffer/CanReceiveBuffer.h"
#include "CoordSettingPanel/CoordSettingPanel.h"
#include "DataDisplayer/DataDisplayer.h"
#include "DataQueueRecordPanel/DataQueueRecordPanel.h"
#include "InputHelper/InputHelper.h"
#include "LogListPanel/LogListPanel.h"
#include "Motion/MoveDefines.h"
#include "QtDependComponents/InfoDispatcher/InfoDispatcher.h"
#include "QtDependComponents/ZynqConnection/UdpMessageDefine.h"
#include "SystemSettings/SystemSettings.h"
#include "qwt_axis.h"
#include "qwt_plot.h"
#include "ui_MainWindow.h"

#include <QFileDialog>

#include "qwt_dial_needle.h"
#include <QFile>
#include <QMessageBox>
#include <qfiledialog.h>
#include <qgridlayout.h>
#include <qnamespace.h>
#include <qpushbutton.h>
#include <qstringliteral.h>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    _init_members();
    _init_status_bar_palette_and_connection();

    {
        // 启动一次file dialog, 系统会分配缓存, 防止后续影响整个进程的实时性
        QFileDialog *dummy_f = new QFileDialog();
        dummy_f->show();
        dummy_f->hide();
        delete dummy_f;
    }

    test_latency_timer_ = new QTimer(this);
    connect(test_latency_timer_, &QTimer::timeout, this, [this]() {
        const auto &i =
            this->shared_core_data_->get_info_dispatcher()->get_info();

        ui->le_test_latency_curr->setText(
            QString::number(i.latency_data.curr_latency));
        ui->le_test_latency_avg->setText(
            QString::number(i.latency_data.avg_latency));
        ui->le_test_latency_max->setText(
            QString::number(i.latency_data.max_latency));
        ui->le_test_latency_warning_count->setText(
            QString::number(i.latency_data.warning_count));

        ui->le_timeuse_ecatavg->setText(
            QString::number(i.time_use_data.ecat_time_use_avg));
        ui->le_timeuse_ecatmax->setText(
            QString::number(i.time_use_data.ecat_time_use_max));

        ui->le_timeuse_infoavg->setText(
            QString::number(i.time_use_data.info_time_use_avg));
        ui->le_timeuse_infomax->setText(
            QString::number(i.time_use_data.info_time_use_max));

        ui->le_timeuse_smavg->setText(
            QString::number(i.time_use_data.statemachine_time_use_avg));
        ui->le_timeuse_smmax->setText(
            QString::number(i.time_use_data.statemachine_time_use_max));

        ui->le_timeuse_totalavg->setText(
            QString::number(i.time_use_data.total_time_use_avg));
        ui->le_timeuse_totalmax->setText(
            QString::number(i.time_use_data.total_time_use_max));
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

    delete test_panel_;

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

    // auto testpanel_layout = new QGridLayout(ui->groupBox_test);
    // test_panel_ = new TestPanel(shared_core_data_);
    // testpanel_layout->addWidget(test_panel_);
    test_panel_ = new TestPanel(shared_core_data_);
    test_panel_->setWindowFlags(test_panel_->windowFlags() | Qt::Tool);
    test_panel_->show();
    test_panel_->hide();
    connect(ui->pb_testpanel, &QPushButton::clicked, this,
            [this]() { test_panel_->show(); });

    auto system_setting_panel_layout = new QGridLayout(ui->tab_system_setting);
    system_setting_panel_ = new SystemSettingPanel(shared_core_data_);
    system_setting_panel_layout->addWidget(system_setting_panel_);

    auto dqr_panel_layout = new QGridLayout(ui->tab_record);
    dqr_panel_ = new DataQueueRecordPanel(shared_core_data_);
    dqr_panel_layout->addWidget(dqr_panel_);

    auto loglist_panel_layout = new QGridLayout(ui->tab_log);
    loglist_panel_ = new LogListPanel(shared_core_data_);
    loglist_panel_layout->addWidget(loglist_panel_);

    _init_tab_monitor();

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    _init_tab_breakout_monitor();
#else
    ui->tab_breakout_monitor->hide();
#endif

    connect(task_manager_, &task::TaskManager::sig_switch_coordindex,
            coord_panel_, &CoordPanel::slot_change_display_coord_index);
    connect(task_manager_, &task::TaskManager::sig_switch_coordindex,
            coord_setting_panel_,
            &CoordSettingPanel::slot_change_display_coord_index);
    connect(task_manager_, &task::TaskManager::sig_coord_offset_changed,
            coord_setting_panel_, &CoordSettingPanel::slot_update_display);
    connect(task_manager_, &task::TaskManager::sig_record_data,
            this, [this](int index, bool start) {
                if (index == 1) {
                    if (start) {
                        dqr_panel_->slot_start_record_data1();
                    } else {
                        dqr_panel_->slot_stop_record_data1();
                    }
                } else if (index == 2) {
                    if (start) {
                        dqr_panel_->slot_start_record_data2();
                    } else {
                        dqr_panel_->slot_stop_record_data2();
                    }
                } else if (index == 0xAE) {
                    if (start) {
                        dqr_panel_->slot_start_audio_record();
                    } else {
                        dqr_panel_->slot_stop_audio_record();
                    }
                }
            });

    // // test
    // auto test_data_displayer_layout = new QGridLayout(ui->tab_testdisplay);
    // test_data_displayer_ = new DataDisplayer();
    // auto dummy = new DataDisplayer();
    // test_data_displayer_layout->addWidget(test_data_displayer_, 0, 0);
    // test_data_displayer_layout->addWidget(dummy, 1, 0);
    // test_data_displayer_->set_axis_title(QwtPlot::yLeft, "D0", Qt::green);
    // test_data_displayer_->set_axis_scale(QwtPlot::yLeft, 0, 100);
    // test_data_displayer_->set_axis_title(QwtPlot::yRight, "D1", Qt::red);
    // test_data_displayer_->set_axis_scale(QwtPlot::yRight, -2, 2);

    // DisplayedDataDesc desc;
    // desc.data_name = "data0";
    // desc.yAxis = QwtPlot::yLeft;
    // desc.visible = true;
    // desc.data_max_points = 300;
    // desc.preferred_color = Qt::green;
    // desc.line_width = 2.0;
    // data_index0_ = test_data_displayer_->add_data_item(desc);
    // desc.data_name = "data1";
    // desc.yAxis = QwtPlot::yRight;
    // desc.visible = true;
    // desc.data_max_points = -1;
    // desc.preferred_color = Qt::red;
    // desc.line_width = 2.0;
    // data_index1_ = test_data_displayer_->add_data_item(desc);

    // display_timer_ = new QTimer(this);
    // connect(display_timer_, &QTimer::timeout, this, [this]() {
    //     static int i = 0;
    //     ++i;

    //     test_data_displayer_->push_data(
    //         data_index0_, sin((double)i * 2.0 * 3.14159265 / 100) * 50 + 50);
    //     test_data_displayer_->push_data(
    //         data_index1_, cos((double)i * 2.0 * 3.14159265 / 100));
    //     test_data_displayer_->update_display();
    // });
    // display_timer_->start(40);
}

void MainWindow::_init_tab_monitor() {
    // layout
    auto vol_cur_displayer_layout = new QGridLayout(ui->tab_monitor);
    vol_cur_displayer_ = new DataDisplayer();
    mach_rate_displayer_ = new DataDisplayer();

    vol_cur_displayer_layout->addWidget(vol_cur_displayer_, 0, 0);
    vol_cur_displayer_layout->addWidget(mach_rate_displayer_, 1, 0);

    // add datas
    DisplayedDataDesc desc;
    desc.visible = true;
    desc.line_width = 2.0;
    // vol_cur
    vol_cur_displayer_->set_axis_title(QwtPlot::yLeft, "V", Qt::red);
    vol_cur_displayer_->set_axis_scale(QwtPlot::yLeft, 0, 120);
       vol_cur_displayer_->set_axis_title(QwtPlot::yRight, "V", Qt::green);
    vol_cur_displayer_->set_axis_scale(QwtPlot::yRight, 0, 120);

    desc.data_max_points = -1;
    // vol
    desc.data_name = "Voltage";
    desc.yAxis = QwtPlot::yLeft;
    desc.preferred_color = Qt::red;
    monitor_voltage_index_ = vol_cur_displayer_->add_data_item(desc);
    // a vol
    desc.data_name = "AvgVolt";
    desc.yAxis = QwtPlot::yRight;
    desc.preferred_color = Qt::green;
    monitor_current_index_ = vol_cur_displayer_->add_data_item(desc);

    // cur
    // desc.data_name = "I";
    // desc.yAxis = QwtPlot::yRight;
    // desc.preferred_color = Qt::green;
    // monitor_current_index_ = vol_cur_displayer_->add_data_item(desc);

    // mach rate
    desc.yAxis = QwtPlot::yLeft;
    desc.data_max_points = -1;
    mach_rate_displayer_->set_axis_title(QwtPlot::yLeft, "Speed", Qt::black);
    mach_rate_displayer_->set_axis_scale(QwtPlot::yLeft, -30, 30);
    // normal
    desc.data_name = "svspeed";
    desc.preferred_color = Qt::green;
    sv_speed_index_ = mach_rate_displayer_->add_data_item(desc);
    // // short
    // desc.data_name = "short";
    // desc.preferred_color = Qt::red;
    // monitor_short_rate_index_ = mach_rate_displayer_->add_data_item(desc);
    // // normal
    // desc.data_name = "open";
    // desc.preferred_color = Qt::yellow;
    // monitor_open_rate_index_ = mach_rate_displayer_->add_data_item(desc);

    // init dial and dial's needle
    dial_needle_ = new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Style::Arrow,
                                           true, Qt::green, Qt::gray);
    dial_needle_->setWidth(10);
    ui->Dial_Voltage->setNeedle(dial_needle_);
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    ui->Dial_Voltage->setUpperBound(150);
#else
    ui->lb_dir_has_kn->hide();
    ui->lb_dis_is_breakout->hide();
#endif

    // init timer
    monitor_timer_ = new QTimer(this);
    connect(monitor_timer_, &QTimer::timeout, this,
            &MainWindow::_slot_monitor_timer_doit);
    monitor_timer_->start(SystemSettings::instance().get_monitor_peroid_ms());
}

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
void MainWindow::_init_tab_breakout_monitor() {
    connect(shared_core_data_->get_info_dispatcher(),
            &InfoDispatcher::info_updated, this,
            [this](const edm::move::MotionInfo &info) {
                if (info.KnDetected()) {
                    ui->lb_dir_has_kn->setText(QStringLiteral("有峭度"));
                    ui->lb_dir_has_kn->setStyleSheet(
                        QStringLiteral("QLabel{background-color:green;}"));
                } else {
                    ui->lb_dir_has_kn->setText(QStringLiteral("无峭度"));
                    ui->lb_dir_has_kn->setStyleSheet(
                        QStringLiteral("QLabel{background-color:white;}"));
                }

                if (info.BreakoutDetected()) {
                    ui->lb_dis_is_breakout->setText(QStringLiteral("已穿透"));
                    ui->lb_dis_is_breakout->setStyleSheet(
                        QStringLiteral("QLabel{background-color:green;}"));
                } else {
                    ui->lb_dis_is_breakout->setText(QStringLiteral("未穿透"));
                    ui->lb_dis_is_breakout->setStyleSheet(
                        QStringLiteral("QLabel{background-color:white;}"));
                }
            });

    // layout
    auto layout = new QGridLayout(ui->tab_breakout_monitor);
    bo_displayer_1_ = new DataDisplayer();
    bo_displayer_2_ = new DataDisplayer();

    layout->addWidget(bo_displayer_1_, 0, 0);
    layout->addWidget(bo_displayer_2_, 1, 0);

    // add datas
    DisplayedDataDesc desc;
    desc.visible = true;
    desc.line_width = 2.0;
    // 1
    bo_displayer_1_->set_axis_title(QwtPlot::yLeft, "V", Qt::green);
    bo_displayer_1_->set_axis_scale(QwtPlot::yLeft, 0, 120);
    bo_displayer_1_->set_axis_title(QwtPlot::yRight, "KNR", Qt::white);
    bo_displayer_1_->set_axis_scale(QwtPlot::yRight, 0, 2.0);

    desc.data_max_points = -1;
    // vol
    desc.data_name = "V";
    desc.yAxis = QwtPlot::yLeft;
    desc.preferred_color = Qt::red;
    bo_v_index_ = bo_displayer_1_->add_data_item(desc);
    // vol
    desc.data_name = "VF";
    desc.yAxis = QwtPlot::yLeft;
    desc.preferred_color = Qt::green;
    bo_av_index_ = bo_displayer_1_->add_data_item(desc);
    // knr
    desc.data_name = "KNR";
    desc.yAxis = QwtPlot::yRight;
    desc.preferred_color = Qt::yellow;
    bo_knr_index_ = bo_displayer_1_->add_data_item(desc);

    // 2
    bo_displayer_2_->set_axis_title(QwtPlot::yLeft, "KN", Qt::green);
    bo_displayer_2_->set_axis_scale(QwtPlot::yLeft, 0, 80.0);
    bo_displayer_2_->set_axis_title(QwtPlot::yRight, "CNT", Qt::yellow);
    bo_displayer_2_->set_axis_scale(QwtPlot::yRight, 0, 1000);

    desc.data_max_points = -1;
    // KN
    desc.data_name = "KN";
    desc.yAxis = QwtPlot::yLeft;
    desc.preferred_color = Qt::green;
    bo_kn_index_ = bo_displayer_2_->add_data_item(desc);
    // KNCNT
    desc.data_name = "CNT";
    desc.yAxis = QwtPlot::yRight;
    desc.preferred_color = Qt::yellow;
    bo_kn_cnt_index_ = bo_displayer_2_->add_data_item(desc);

    connect(shared_core_data_->get_info_dispatcher(),
            &InfoDispatcher::info_updated, this,
            &MainWindow::_slot_breakout_monitor_timer_doit);
}
#endif

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

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
void MainWindow::_slot_breakout_monitor_timer_doit(
    const move::MotionInfo &info) {
    bo_displayer_1_->push_data(bo_v_index_, info.breakout_data.realtime_voltage);
    bo_displayer_1_->push_data(bo_av_index_, info.breakout_data.averaged_voltage);
    bo_displayer_1_->push_data(bo_knr_index_, info.breakout_data.kn_valid_rate);
    bo_displayer_1_->update_display();

    bo_displayer_2_->push_data(bo_kn_index_, info.breakout_data.kn);
    bo_displayer_2_->push_data(bo_kn_cnt_index_, info.breakout_data.kn_cnt);
    bo_displayer_2_->update_display();
}
#endif

void MainWindow::_slot_monitor_timer_doit() {
#ifdef EDM_USE_ZYNQ_SERVOBOARD // use zynq board to do servo things
    zynq::servo_return_converted_data_t sd;
    shared_core_data_->get_zynq_udpmessage_holder()->get_udp_message(sd);

    vol_cur_displayer_->push_data(monitor_voltage_index_,
                                  (double)sd.realtime_voltage);
    vol_cur_displayer_->push_data(monitor_current_index_,
                                  (double)sd.averaged_voltage);
                                  
    vol_cur_displayer_->update_display();

    mach_rate_displayer_->push_data(sv_speed_index_,
                                    (double)sd.servo_calced_speed_mm_min);
    mach_rate_displayer_->update_display();

    ui->Dial_Voltage->setValue((double)sd.averaged_voltage);
    ui->sb_show_test_voltage->setValue((int)sd.averaged_voltage);
    ui->dsb_show_test_voltage->setValue(sd.averaged_voltage);
#else
    // TODO
    Can1IOBoard407ServoData sd;
    shared_core_data_->get_can_recv_buffer()->load_servo_data(sd);

    vol_cur_displayer_->push_data(monitor_voltage_index_,
                                  (double)sd.average_voltage);
    vol_cur_displayer_->push_data(monitor_current_index_, (double)sd.current);
    vol_cur_displayer_->update_display();

    mach_rate_displayer_->push_data(monitor_normal_rate_index_, sd.normal_rate);
    mach_rate_displayer_->push_data(monitor_short_rate_index_, sd.short_rate);
    mach_rate_displayer_->push_data(monitor_open_rate_index_, sd.open_rate);
    mach_rate_displayer_->update_display();

    ui->Dial_Voltage->setValue((double)sd.average_voltage);
    ui->sb_show_test_voltage->setValue((int)sd.average_voltage);
    ui->dsb_show_test_voltage->setValue(sd.average_voltage);
#endif
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
