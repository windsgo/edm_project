#pragma once

#include <QGridLayout>
#include <QPushButton>
#include <QWidget>
#include <QString>

#include <unordered_map>

#include "SharedCoreData/SharedCoreData.h"

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

    void _add_button_to_gridlayout(const QString &button_name,
                                   uint32_t out_num, int row, int col);

    void _layout_button_columes_priority(uint32_t col_nums);

private:
    void _update_all_io_display();

    void _button_clicked(uint32_t io_num, bool checked);

private:
    Ui::IOPanel *ui;

    SharedCoreData *shared_core_data_;

    io::IOController::ptr io_ctrler_;

    // 按钮 - io 对应
    std::unordered_map<uint32_t, QPushButton *> map_io_to_button_;
    std::unordered_map<QPushButton *, uint32_t> map_button_to_io_;

    // io 按钮布局
    QGridLayout *io_button_layout_{nullptr};

    QTimer* update_io_timer_;
};

} // namespace app
} // namespace edm
