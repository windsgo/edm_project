#pragma once

#include <QWidget>
#include <SharedCoreData/SharedCoreData.h>

namespace Ui {
class MovePanel;
}

namespace edm {
namespace app {

class MovePanel : public QWidget {
    Q_OBJECT

public:
    explicit MovePanel(SharedCoreData *shared_core_data, QWidget *parent = nullptr);
    ~MovePanel();

private:
    Ui::MovePanel *ui;

    SharedCoreData *shared_core_data_;
};

} // namespace app
} // namespace edm
