#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <qpushbutton.h>
#include <unordered_map>

#include "SharedCoreData/SharedCoreData.h"

#include "config.h"

namespace Ui {
class IOPanel;
}

namespace edm {
namespace app {

class IOPanel : public QWidget {
    Q_OBJECT

public:
    explicit IOPanel(SharedCoreData *shared_core_data,
                     QWidget *parent = nullptr);
    ~IOPanel();

public: // slots
    // 用于外部调用/连接, 根据io控制器更新所有io显示
    void slot_update_io_display();

private:
    // 初始化io按钮布局和回调
    void _init_common_io_buttons();

    QPushButton *_make_button_at_gridlayout(const QString &button_name, int row,
                                            int col);

    void _set_button_output(QPushButton *pb, uint32_t out_num);

    void _set_button_input(QPushButton *pb, uint32_t out_num);

    void _layout_button_rows_priority(uint32_t row_nums);

    void _init_handbox_pump_signal();

private:
    void _update_all_io_display();

    void _button_clicked(uint32_t io_num, bool checked);

private:
#ifdef EDM_POWER_DIMEN_WITH_EXTRA_ZHONGGU_IO
    void _dimenextra_set_button_output(QPushButton *pb, uint32_t out_num);
    void _dimenextra_button_clicked(uint32_t io_num, bool checked); // 中古IO部分按钮
#endif

private:
    Ui::IOPanel *ui;

    SharedCoreData *shared_core_data_;

    io::IOController::ptr io_ctrler_;

    // 按钮 - io 对应
    std::unordered_map<uint32_t, QPushButton *> map_io_to_button_;
    std::unordered_map<QPushButton *, uint32_t> map_button_to_io_;

#ifdef EDM_POWER_DIMEN_WITH_EXTRA_ZHONGGU_IO
    std::unordered_map<uint32_t, QPushButton *> dimenextra_map_io_to_button_;
    std::unordered_map<QPushButton *, uint32_t> dimenextra_map_button_to_io_;
#endif

    // io 按钮布局
    QGridLayout *io_button_layout_{nullptr};

    //#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE ==
    //EDM_POWER_ZHONGGU_DRILL)
    // INPUT 按钮io对应
    std::unordered_map<uint32_t, QPushButton *> map_inputio_to_dispbtn_;
    //#endif

    QTimer *update_io_timer_;
};

} // namespace app
} // namespace edm
