#include "testpowerwidget.h"
#include "ui_testpowerwidget.h"

TestPowerWidget::TestPowerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestPowerWidget)
{
    ui->setupUi(this);
}

TestPowerWidget::~TestPowerWidget()
{
    delete ui;
}
