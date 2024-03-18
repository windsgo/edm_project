#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFile>
#include <QMessageBox>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    shared_core_data_ = new SharedCoreData(this);

    task_manager_ = new task::TaskManager(shared_core_data_, this);

    test_gcode_runner_ = new task::GCodeRunner(shared_core_data_, this);

    coord_panel_ = new CoordPanel(shared_core_data_, ui->frame_coordpanel);
    info_panel_ = new InfoPanel(shared_core_data_, ui->groupBox_info);
    move_panel_ =
        new MovePanel(shared_core_data_, task_manager_, ui->groupBox_pm);
    io_panel_ = new IOPanel(shared_core_data_, ui->tab_io);
    power_panel_ = new PowerPanel(shared_core_data_, ui->tab_power);

    test_codeeditor_ = new CodeEditor(ui->test_codeeditor_widget);
    test_codeeditor_->setFixedSize(ui->test_codeeditor_widget->size());

    connect(task_manager_, &task::TaskManager::sig_switch_coordindex,
            coord_panel_, &CoordPanel::slot_change_display_coord_index);

    connect(test_gcode_runner_, &task::GCodeRunner::sig_switch_coordindex,
            coord_panel_, &CoordPanel::slot_change_display_coord_index);

    _init_test_gcode_buttons();

    connect(ui->pb_test, &QPushButton::clicked, this, [this]() {

    });
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
                // auto ret = this->task_manager_->operation_gcode_start(vec);
                auto ret = this->test_gcode_runner_->start(vec);
                

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
        // auto ret = task_manager_->operation_gcode_pause();
        auto ret = test_gcode_runner_->pause();
        if (!ret) {
            QMessageBox::critical(this, "error",
                                  "operation_gcode_pause failed");
        }
    });

    connect(ui->pb_test_resume, &QPushButton::clicked, this, [this]() {
        // auto ret = task_manager_->operation_gcode_resume();
        auto ret = test_gcode_runner_->resume();
        if (!ret) {
            QMessageBox::critical(this, "error",
                                  "operation_gcode_resume failed");
        }
    });

    connect(ui->pb_test_stop, &QPushButton::clicked, this, [this]() {
        // auto ret = task_manager_->operation_gcode_stop();
        auto ret = test_gcode_runner_->stop();
        if (!ret) {
            QMessageBox::critical(this, "error", "operation_gcode_stop failed");
        }
    });

    connect(ui->pb_test_estop, &QPushButton::clicked, this, [this]() {
        // auto ret = task_manager_->operation_emergency_stop();
        auto ret = test_gcode_runner_->estop();
        if (!ret) {
            QMessageBox::critical(this, "error",
                                  "operation_emergency_stop failed");
        }
    });

    connect(
        test_gcode_runner_, &task::GCodeRunner::sig_autogcode_switched_to_line, this,
        [this](uint32_t line) {

            qDebug() << "line: " << line;

            test_codeeditor_->moveCursorToLine(line);
        });

    connect(
        test_gcode_runner_, &task::GCodeRunner::sig_auto_started, this, [this]() {
            QMessageBox::information(this, "info", "taskmanager: auto_started");
        });

    connect(test_gcode_runner_, &task::GCodeRunner::sig_auto_paused, this, [this]() {
        QMessageBox::information(this, "info", "taskmanager: auto_paused");
    });

    connect(
        test_gcode_runner_, &task::GCodeRunner::sig_auto_resumed, this, [this]() {
            QMessageBox::information(this, "info", "taskmanager: auto_resumed");
        });

    connect(
        test_gcode_runner_, &task::GCodeRunner::sig_auto_stopped, this,
        [this](bool err_occured) {
            if (err_occured) {
                QMessageBox::critical(
                    this, "error",
                    QString("taskmanager: auto_stopped. **Error Occured:\n") +
                        test_gcode_runner_->last_error_str().c_str());
            } else {
                QMessageBox::information(this, "info",
                                         "taskmanager: auto_stopped");
            }
        });
}

} // namespace app
} // namespace edm
