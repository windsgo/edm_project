#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    shared_core_data_ = new SharedCoreData(this);

    coord_panel_ = new CoordPanel(shared_core_data_, ui->frame_coordpanel);
    info_panel_ = new InfoPanel(shared_core_data_, ui->groupBox_info);
    move_panel_ = new MovePanel(shared_core_data_, ui->groupBox_pm);
    io_panel_ = new IOPanel(shared_core_data_, ui->tab_io);
    power_panel_ = new PowerPanel(shared_core_data_, ui->tab_power);

    task_manager_ = new task::TaskManager(shared_core_data_, this);

    connect(task_manager_, &task::TaskManager::sig_switch_coordindex,
            coord_panel_, &CoordPanel::slot_change_display_coord_index);

    connect(ui->pb_test, &QPushButton::clicked, this, [this]() {
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

            std::cout << ret.dumps(2) << std::endl;

            auto gcode_lists_opt =
                edm::task::GCodeTaskConverter::MakeGCodeTaskListFromJson(ret);

            if (!gcode_lists_opt) {
                s_logger->warn("MakeGCodeTaskListFromJson failed");
            } else {
                s_logger->info("MakeGCodeTaskListFromJson ok");

                auto vec = std::move(*gcode_lists_opt);

                s_logger->info("size: {}", vec.size());

                for (const auto &g : vec) {
                    if (!g)
                        continue;
                    s_logger->debug("type: {}, line: {}", (int)g->type(),
                                    g->line_number());
                }
            }

        } catch (const edm::interpreter::RS274InterpreterException &e) {
            std::cout << "error1\n";

            std::cout << e.what() << std::endl;
        } catch (const std::exception &e) {
            std::cout << "error2\n";

            std::cout << e.what() << std::endl;
        }
    });
}

MainWindow::~MainWindow() {
    s_logger->info("-----------------------------------------");
    s_logger->info("------------ MainWindow Dtor ------------");
    s_logger->info("-----------------------------------------");

    delete ui;
}

} // namespace app
} // namespace edm
