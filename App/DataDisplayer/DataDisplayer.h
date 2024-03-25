#ifndef DATADISPLAYER_H
#define DATADISPLAYER_H

#include <QWidget>

namespace Ui {
class DataDisplayer;
}

class DataDisplayer : public QWidget
{
    Q_OBJECT

public:
    explicit DataDisplayer(QWidget *parent = nullptr);
    ~DataDisplayer();

private:
    Ui::DataDisplayer *ui;
};

#endif // DATADISPLAYER_H
