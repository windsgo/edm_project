#include "DataDisplayer.h"
#include "ui_DataDisplayer.h"

DataDisplayer::DataDisplayer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataDisplayer)
{
    ui->setupUi(this);
}

DataDisplayer::~DataDisplayer()
{
    delete ui;
}
