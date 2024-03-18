#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFile>
#include <QMessageBox>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    _init_members();
    _init_status_bar_palette_and_connection();
}

MainWindow::~MainWindow() {
    s_logger->info("-----------------------------------------");
    s_logger->info("------------ MainWindow Dtor ------------");
    s_logger->info("-----------------------------------------");

    delete ui;
}

void MainWindow::_init_test_gcode_buttons() {
    connect(ui->pb_test_start, &QPushButton::clicked, this, [this]() {
        try {
            edm::interpreter::RS274InterpreterWrapper::instance()
                ->set_rs274_py_module_dir(
                    EDM_ROOT_DIR +
                    this->shared_core_data_->get_system_settings()
                        .get_interp_module_path_relative_to_root());

            auto ret =
                edm::interpreter::RS274InterpreterWrapper::instance()
                    ->parse_file_to_json(EDM_ROOT_DIR
                                         "tests/Interpreter/pytest/test1.py");

            QFile file(EDM_ROOT_DIR "tests/Interpreter/pytest/test1.py");
            file.open(QIODevice::OpenModeFlag::ReadOnly);

            test_codeeditor_->clear();
            test_codeeditor_->setPlainText(file.readAll());

            file.close();

            auto gcode_lists_opt =
                edm::task::GCodeTaskConverter::MakeGCodeTaskListFromJson(ret);

            if (!gcode_lists_opt) {
                s_logger->warn("MakeGCodeTaskListFromJson failed");
                QMessageBox::critical(this, "error",
                                      "MakeGCodeTaskListFromJson failed");
            } else {
                s_logger->info("MakeGCodeTaskListFromJson ok");

                auto vec = std::move(*gcode_lists_opt);

                // give to task manager
                auto ret = this->task_manager_->operation_gcode_start(vec);

                if (!ret) {
                    QMessageBox::critical(this, "error",
                                          "operation_gcode_start failed");
                }
            }

        } catch (const std::exception &e) {
            std::cout << "error\n";

            QMessageBox::critical(this, "error",
                                  QString("start gcode exception:") + e.what());

            std::cout << e.what() << std::endl;
        }
    });

    connect(ui->pb_test_pause, &QPushButton::clicked, this, [this]() {
        auto ret = task_manager_->operation_gcode_pause();
        // auto ret = test_gcode_runner_->pause();
        if (!ret) {
            QMessageBox::critical(this, "error",
                                  "operation_gcode_pause failed");
        }
    });

    connect(ui->pb_test_resume, &QPushButton::clicked, this, [this]() {
        auto ret = task_manager_->operation_gcode_resume();
        // auto ret = test_gcode_runner_->resume();
        if (!ret) {
            QMessageBox::critical(this, "error",
                                  "operation_gcode_resume failed");
        }
    });

    connect(ui->pb_test_stop, &QPushButton::clicked, this, [this]() {
        auto ret = task_manager_->operation_gcode_stop();
        // auto ret = test_gcode_runner_->stop();
        if (!ret) {
            QMessageBox::critical(this, "error", "operation_gcode_stop failed");
        }
    });

    connect(ui->pb_test_estop, &QPushButton::clicked, this, [this]() {
        auto ret = task_manager_->operation_emergency_stop();
        // auto ret = test_gcode_runner_->estop();
        if (!ret) {
            QMessageBox::critical(this, "error",
                                  "operation_emergency_stop failed");
        }
    });

    connect(task_manager_, &task::TaskManager::sig_autogcode_switched_to_line,
            this, [this](uint32_t line) {
                qDebug() << "line: " << line;

                test_codeeditor_->moveCursorToLine(line);
            });

    connect(task_manager_, &task::TaskManager::sig_auto_started, this,
            [this]() {
                // QMessageBox::information(this, "info", "taskmanager:
                // auto_started");
            });

    connect(task_manager_, &task::TaskManager::sig_auto_paused, this, [this]() {
        // QMessageBox::information(this, "info", "taskmanager: auto_paused");
    });

    connect(task_manager_, &task::TaskManager::sig_auto_resumed, this,
            [this]() {
                // QMessageBox::information(this, "info", "taskmanager:
                // auto_resumed");
            });

    connect(
        task_manager_, &task::TaskManager::sig_auto_stopped, this,
        [this](bool err_occured) {
            if (err_occured) {
                QMessageBox::critical(
                    this, "error",
                    QString("taskmanager: auto_stopped. **Error Occured:\n") +
                        task_manager_->autogcode_last_error_str().c_str());
            } else {
                QMessageBox::information(this, "info",
                                         "taskmanager: auto_stopped");
            }
        });
}

void MainWindow::_init_members() {
    shared_core_data_ = new SharedCoreData(this);

    task_manager_ = new task::TaskManager(shared_core_data_, this);

    auto coord_layout = new QGridLayout(ui->frame_coordpanel);
    coord_panel_ = new CoordPanel(shared_core_data_);
    coord_layout->addWidget(coord_panel_);

    auto info_layout = new QGridLayout(ui->groupBox_info);
    info_panel_ = new InfoPanel(shared_core_data_);
    info_layout->addWidget(info_panel_);

    auto move_layout = new QGridLayout(ui->groupBox_pm);
    move_panel_ = new MovePanel(shared_core_data_, task_manager_);
    move_layout->addWidget(move_panel_);

    auto io_layout = new QGridLayout(ui->tab_io);
    io_panel_ = new IOPanel(shared_core_data_);
    io_layout->addWidget(io_panel_);

    auto power_layout = new QGridLayout(ui->tab_power);
    power_panel_ = new PowerPanel(shared_core_data_);
    power_layout->addWidget(power_panel_);

    auto gcode_layout = new QGridLayout(ui->groupBox_gcode);
    gcode_panel_ = new GCodePanel(shared_core_data_, task_manager_);
    gcode_layout->addWidget(gcode_panel_);

    auto testpanel_layout = new QGridLayout(ui->groupBox_test);
    test_panel_ = new TestPanel(shared_core_data_);
    testpanel_layout->addWidget(test_panel_);

    test_codeeditor_ = new CodeEditor(ui->test_codeeditor_widget);
    test_codeeditor_->setFixedSize(ui->test_codeeditor_widget->size());

    connect(task_manager_, &task::TaskManager::sig_switch_coordindex,
            coord_panel_, &CoordPanel::slot_change_display_coord_index);
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
