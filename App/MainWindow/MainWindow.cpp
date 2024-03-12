#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {
MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    shared_core_data_ = new SharedCoreData(this);

    coord_panel_ = new CoordPanel(shared_core_data_, ui->frame_coordpanel);
    info_panel_ = new InfoPanel(shared_core_data_, ui->frame_info);
    move_panel_ = new MovePanel(shared_core_data_, ui->frame_move);
}

MainWindow::~MainWindow()
{
    s_logger->info("-----------------------------------------");
    s_logger->info("------------ MainWindow Dtor ------------");
    s_logger->info("-----------------------------------------");

    delete ui;
}

} // namespace app
} // namespace edm

