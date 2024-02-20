#ifndef TESTPOWERWIDGET_H
#define TESTPOWERWIDGET_H

#include <QWidget>

namespace Ui {
class TestPowerWidget;
}

class TestPowerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestPowerWidget(QWidget *parent = nullptr);
    ~TestPowerWidget();

private:
    Ui::TestPowerWidget *ui;
};

#endif // TESTPOWERWIDGET_H
