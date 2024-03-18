#include "GCodePanel.h"
#include "ui_GCodePanel.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

GCodePanel::GCodePanel(SharedCoreData *shared_core_data,
                       task::TaskManager *task_manager, QWidget *parent)
    : QWidget(parent), ui(new Ui::GCodePanel),
      shared_core_data_(shared_core_data), task_manager_(task_manager) {
    ui->setupUi(this);

    _init_codeeditor_layout();
    _init_button_slots();
    _init_autogcode_connections();

    ui->le_current_file->setReadOnly(true);

    _set_ui_edit_enable(false);
    _set_editbutton_enable(true);
    _set_machining_ui_init();
}

GCodePanel::~GCodePanel() { delete ui; }

void GCodePanel::_init_codeeditor_layout() {
    auto layout = new QGridLayout(ui->widget_editor);

    gcode_editor_ = new CodeEditor();
    layout->addWidget(gcode_editor_, 0, 0);
}

void GCodePanel::_init_button_slots() {
    connect(ui->pb_edit, &QPushButton::clicked, this, &GCodePanel::_slot_edit);
    connect(ui->pb_save, &QPushButton::clicked, this, &GCodePanel::_slot_save);
    connect(ui->pb_save_as, &QPushButton::clicked, this,
            &GCodePanel::_slot_save_as);
    connect(ui->pb_cancel, &QPushButton::clicked, this,
            &GCodePanel::_slot_cancel);
    connect(ui->pb_loadfile, &QPushButton::clicked, this,
            &GCodePanel::_slot_loadfile);
    connect(ui->pb_start, &QPushButton::clicked, this,
            &GCodePanel::_slot_start);
    connect(ui->pb_pause, &QPushButton::clicked, this,
            &GCodePanel::_slot_pause);
    connect(ui->pb_resume, &QPushButton::clicked, this,
            &GCodePanel::_slot_resume);
    connect(ui->pb_stop, &QPushButton::clicked, this, &GCodePanel::_slot_stop);
    connect(ui->pb_estop, &QPushButton::clicked, this,
            &GCodePanel::_slot_estop);
    connect(ui->pb_ack, &QPushButton::clicked, this, &GCodePanel::_slot_ack);
}

void GCodePanel::_init_autogcode_connections() {
    connect(task_manager_, &task::TaskManager::sig_auto_started, this,
            [this]() {
                this->_set_machining_ui_started();
                this->_set_editbutton_enable(false);
            });
    connect(task_manager_, &task::TaskManager::sig_auto_paused, this,
            [this]() { this->_set_machining_ui_paused(); });
    connect(task_manager_, &task::TaskManager::sig_auto_resumed, this,
            [this]() { this->_set_machining_ui_resumed(); });
    connect(task_manager_, &task::TaskManager::sig_auto_stopped, this,
            [this](bool err_occured) {
                this->_set_machining_ui_stopped();
                this->_set_editbutton_enable(true);

                if (err_occured) {
                    // TODO
                    QMessageBox::critical(
                        this, "gcode abort error",
                        QString("GCode Stoped. **Error Occured:\n") +
                            task_manager_->autogcode_last_error_str().c_str());
                } else {
                    QMessageBox::information(this, "GCode End",
                                             "GCode Normally Exit.");
                }
            });

    connect(
        task_manager_, &task::TaskManager::sig_autogcode_switched_to_line, this,
        [this](uint32_t line) { this->gcode_editor_->moveCursorToLine(line); });
}

void GCodePanel::_slot_edit(bool checked) {
    if (checked) {
        // 使能编辑
        _set_ui_edit_enable(true);
        gcode_editor_->document()->setModified(false);
    } else {
        // 等于Cancel操作
        _slot_cancel();
    }
}

void GCodePanel::_slot_save() {
    if (!gcode_editor_->document()->isModified()) {

        // 退出编辑
        _set_ui_edit_enable(false);
        return;
    }

    QString save_filename = this->gcode_root_dir_ + ui->le_current_file->text();

    if (!_save_to_file(save_filename)) {
        QMessageBox::critical(
            this, "Save Error",
            QString("Save Failed, File Open Failed!\n%0").arg(save_filename));
        return;
    }

    // 设置为未修改
    gcode_editor_->document()->setModified(false);

    // 退出编辑
    _set_ui_edit_enable(false);

    // 提示成功
    QMessageBox::information(this, "Save Success",
                             QString{"Saved File to: %0"}.arg(save_filename));
}

void GCodePanel::_slot_save_as() {

    auto save_filename =
        QFileDialog::getSaveFileName(this, "Save As", gcode_root_dir_,
                                     QString{}, nullptr, QFileDialog::ReadOnly);
    if (save_filename.isNull() || save_filename.isEmpty()) {
        return;
    }

    if (!_save_to_file(save_filename)) {
        QMessageBox::critical(this, "Save As Error",
                              QString("Save As Failed, File Open Failed!\n%0")
                                  .arg(save_filename));
        return;
    }

    // 设置为未修改
    gcode_editor_->document()->setModified(false);

    // 退出编辑
    _set_ui_edit_enable(false);

    // 将文件名改为另存为的名字 (相对路径)
    QDir gcode_root(gcode_root_dir_);
    auto save_filename_relative = gcode_root.relativeFilePath(save_filename);
    ui->le_current_file->setText(save_filename_relative);

    QMessageBox::information(this, "Save Success",
                             QString{"Saved File to: %0"}.arg(save_filename));
}

void GCodePanel::_slot_cancel() {
    if (gcode_editor_->document()->isModified()) {

        // 询问是否保存
        int ret = QMessageBox::warning(this, "Save ?", "Not saved yet. Save ?",
                                       QMessageBox::Yes, QMessageBox::No,
                                       QMessageBox::Cancel);

        if (ret == QMessageBox::Yes) {
            _slot_save();
        } else if (ret == QMessageBox::No) {
            _load_from_file(ui->le_current_file->text());
            _set_ui_edit_enable(false);
        } else {
            // Cancel or other ...
            return;
        }

    } else {
        _set_ui_edit_enable(false);
    }
}

void GCodePanel::_slot_loadfile() {
    auto load_filename =
        QFileDialog::getOpenFileName(this, "Load GCode File", gcode_root_dir_,
                                     QString{}, nullptr, QFileDialog::ReadOnly);
    if (load_filename.isNull() || load_filename.isEmpty()) {
        return;
    }

    if (!_load_from_file(load_filename)) {
        QMessageBox::critical(this, "Load File Error",
                              QString("Load File Failed, File Open Failed!\n%0")
                                  .arg(load_filename));
        return;
    }

    // 将文件名改为另存为的名字 (相对路径)
    QDir gcode_root(gcode_root_dir_);
    auto save_filename_relative = gcode_root.relativeFilePath(load_filename);
    ui->le_current_file->setText(save_filename_relative);
}

void GCodePanel::_slot_start() {
    auto filename = ui->le_current_file->text();
    if (filename.isEmpty() || filename.isNull()) {
        QMessageBox::critical(this, "start error",
                              QString("Start Gcode Failed, No File choosed"));
        return;
    }

    auto whole_filename = gcode_root_dir_ + filename;

    if (!_load_from_file(whole_filename)) {
        QMessageBox::critical(
            this, "start error",
            QString("Start Gcode Failed, File Open Failed: \n%0")
                .arg(whole_filename));
        return;
    }

    std::string filename_stdstr = whole_filename.toUtf8().toStdString();

    try {
        edm::interpreter::RS274InterpreterWrapper::instance()
            ->set_rs274_py_module_dir(
                EDM_ROOT_DIR + this->shared_core_data_->get_system_settings()
                                   .get_interp_module_path_relative_to_root());

        auto parse_json_ret =
            edm::interpreter::RS274InterpreterWrapper::instance()
                ->parse_file_to_json(filename_stdstr);

        try {
            auto gcode_lists_opt =
                edm::task::GCodeTaskConverter::MakeGCodeTaskListFromJson(
                    parse_json_ret);

            if (!gcode_lists_opt) {
                s_logger->warn("MakeGCodeTaskListFromJson failed");
                QMessageBox::critical(
                    this, "start error",
                    "Start Gcode Failed: Generate GCode List Failed.");
                return;
            } else {
                s_logger->info("MakeGCodeTaskListFromJson ok");

                auto vec = std::move(*gcode_lists_opt);

                // give to task manager
                auto ret = this->task_manager_->operation_gcode_start(vec);

                if (!ret) {
                    QMessageBox::critical(
                        this, "start error",
                        "Start Gcode Failed: TaskManager Start Failed");
                    return;
                }
            }
        } catch (const std::exception &e) {
            QMessageBox::critical(
                this, "start error",
                QString("Start Gcode Failed, MakeGCodeTaskListFromJson "
                        "Exception: \n%0")
                    .arg(e.what()));
            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::critical(
            this, "start error",
            QString("Start Gcode Failed, Parse Error: \n%0").arg(e.what()));
        return;
    }

    // start success

    // 设置编辑按钮不可用
    this->_set_editbutton_enable(false);
    this->_set_ui_edit_enable(false);
    this->_set_machining_ui_started();
}

void GCodePanel::_slot_pause() {
    auto ret = task_manager_->operation_gcode_pause();

    if (!ret) {
        // pause operation can be failed, if pause when initing a node
        // so if failed, do not messagebox

        // QMessageBox::critical(this, "Pause Failed",
        //                       QString("Pause Gcode Failed"));

        // instead
        s_logger->warn("GCodePanel: _slot_pause: pause failed");

        // TODO find somewhere to log on the screen
    }
}

void GCodePanel::_slot_resume() {
    auto ret = task_manager_->operation_gcode_resume();

    if (!ret) {
        QMessageBox::critical(this, "Resume Failed",
                              QString("Resume Gcode Failed"));
    }
}

void GCodePanel::_slot_stop() {

    auto ret = task_manager_->operation_gcode_stop();

    if (!ret) {
        QMessageBox::critical(this, "Stop Failed",
                              QString("Stop Gcode Failed"));
    }

    // 设置编辑按钮可用
    // this->_set_editbutton_enable(true);
}

void GCodePanel::_slot_estop() {
    task_manager_->operation_emergency_stop();

    // _set_ui_edit_enable(false);
    // _set_editbutton_enable(true);
    // _set_machining_ui_stopped();
}

void GCodePanel::_slot_ack() {
    auto ack_cmd =
        std::make_shared<edm::move::MotionCommandSettingClearWarning>(0);

    shared_core_data_->get_motion_cmd_queue()->push_command(ack_cmd);

    bool ret = task::TaskHelper::WaitforCmdTobeAccepted(ack_cmd, 200);

    if (!ret) {
        QMessageBox::critical(this, "ACK Failed", QString("Send ACK Failed"));
    }
}

bool GCodePanel::_load_from_file(const QString &filename) {
    gcode_editor_->clear();

    QFile file(filename);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    gcode_editor_->setPlainText(file.readAll());
    gcode_editor_->document()->setModified(false);

    file.close();

    return true;
}

bool GCodePanel::_save_to_file(const QString &filename) {
    QFile savefile(filename);
    if (!savefile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&savefile);

    QString text = gcode_editor_->toPlainText();

    // 将锁进替换为空格
    text.replace('\t', "    ");

    out << text.toUtf8();

    out.flush();

    savefile.close();

    return true;
}

void GCodePanel::_set_ui_edit_enable(bool enable) {
    gcode_editor_->setReadOnly(!enable);
    ui->pb_edit->setChecked(enable);
    ui->pb_save->setEnabled(enable);
    ui->pb_save_as->setEnabled(enable);
    ui->pb_cancel->setEnabled(enable);
    ui->pb_loadfile->setEnabled(!enable);

    ui->pb_start->setEnabled(!enable);
}

void GCodePanel::_set_editbutton_enable(bool enable) {
    ui->pb_edit->setEnabled(enable);
}

void GCodePanel::_set_machining_ui_init() {
    ui->pb_start->setEnabled(true);
    ui->pb_pause->setEnabled(false);
    ui->pb_resume->setEnabled(false);
    ui->pb_stop->setEnabled(false);
}

void GCodePanel::_set_machining_ui_started() {
    ui->pb_start->setEnabled(false);
    ui->pb_pause->setEnabled(true);
    ui->pb_resume->setEnabled(false);
    ui->pb_stop->setEnabled(true);
}

void GCodePanel::_set_machining_ui_paused() {
    ui->pb_start->setEnabled(false);
    ui->pb_pause->setEnabled(false);
    ui->pb_resume->setEnabled(true);
    ui->pb_stop->setEnabled(true);
}

void GCodePanel::_set_machining_ui_resumed() {
    ui->pb_start->setEnabled(false);
    ui->pb_pause->setEnabled(true);
    ui->pb_resume->setEnabled(false);
    ui->pb_stop->setEnabled(true);
}

void GCodePanel::_set_machining_ui_stopped() { _set_machining_ui_init(); }

} // namespace app
} // namespace edm
