#pragma once

#include <QWidget>

#include "SharedCoreData/SharedCoreData.h"
#include "TaskManager/TaskManager.h"
#include "TaskManager/GCodeTaskConverter.h"
#include "TaskManager/GCodeTask.h"

#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>

#include "codeeditor/codeeditor.h"

#include "Interpreter/rs274pyInterpreter/RS274InterpreterWrapper.h"

namespace Ui {
class GCodePanel;
}

namespace edm {
namespace app {

class GCodePanel : public QWidget
{
    Q_OBJECT

public:
    explicit GCodePanel(SharedCoreData* shared_core_data, task::TaskManager* task_manager, QWidget *parent = nullptr);
    ~GCodePanel();

private:
    void _init_codeeditor_layout();
    void _init_button_slots();

    void _init_autogcode_connections();

private:
    void _slot_edit(bool checked);
    void _slot_save();
    void _slot_save_as();
    void _slot_cancel();
    void _slot_loadfile();

    void _slot_start();
    void _slot_pause();
    void _slot_resume();
    void _slot_stop();
    void _slot_estop();

    void _slot_ack();

    bool _load_from_file(const QString& filename);
    bool _save_to_file(const QString& filename);

private:
    // 设置编辑ui状态为可编辑或不可编辑, 并不检查保存状态
    void _set_ui_edit_enable(bool enable);

    // 设置edit按钮本身是否可用, 加工中不可用
    void _set_editbutton_enable(bool enable);

    // 设置加工ui状态
    void _set_machining_ui_init();
    void _set_machining_ui_started();
    void _set_machining_ui_paused();
    void _set_machining_ui_resumed();
    void _set_machining_ui_stopped();

private:
    Ui::GCodePanel *ui;

    SharedCoreData* shared_core_data_;
    task::TaskManager* task_manager_;

    CodeEditor* gcode_editor_;

    const QString gcode_root_dir_ {QStringLiteral(EDM_ROOT_DIR "/gcode/")};
};

} // namespace app
} // namespace edm

