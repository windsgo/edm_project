#include "MainWindow.h"
#include "ui_MainWindow.h"


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
}

MainWindow::~MainWindow()
{
    delete ui;
}

} // namespace app
} // namespace edm

