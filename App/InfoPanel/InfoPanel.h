#pragma once

#include <QWidget>
#include <qtcpsocket.h>

#include "SharedCoreData/SharedCoreData.h"

#include "TaskManager/GCodeRunner.h"
#include "TaskManager/TaskManager.h"

namespace Ui {
class InfoPanel;
}

namespace edm {
namespace app {

class InfoPanel : public QWidget {
    Q_OBJECT

public:
    explicit InfoPanel(SharedCoreData *shared_core_data,
                       QWidget *parent = nullptr);
    ~InfoPanel();

private:
    void _init_info_slot();

private: // slots
    void _update_info(const move::MotionInfo& info);
    void _do_update_ui_by_info(const move::MotionInfo& info);

private:
    Ui::InfoPanel *ui;

    SharedCoreData *shared_core_data_;

    // QTcpSocket::SocketState tcp_state_cached_{QTcpSocket::SocketState::UnconnectedState};
};

} // namespace app
} // namespace edm
