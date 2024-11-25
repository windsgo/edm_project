#pragma once

#include <QWidget>
#include <optional>
#include <qobjectdefs.h>

namespace Ui {
class ADCCalcPanel;
}

namespace edm {
namespace app {

class ADCCalcPanel : public QWidget {
    Q_OBJECT
public:
    struct ADCCalcResult {
        double new_k;
        double new_b;
    };

    struct ADCCalcInput {
        double current_k;
        double current_b;

        double display_v_1; // 采样电压值
        double real_v_1;    // 实际的电压

        double display_v_2;
        double real_v_2;
    };

public:
    explicit ADCCalcPanel(QWidget *parent = nullptr);
    ~ADCCalcPanel();

public slots:
    void slot_set_current_k_and_b_to_ui(double k, double b);

signals:
    void sig_result_calced(std::optional<ADCCalcResult> result_opt);
    void sig_result_applied(const ADCCalcResult& result_opt);

public:
    static std::optional<ADCCalcResult> CalcADC(const ADCCalcInput &input);

private:
    Ui::ADCCalcPanel *ui;
};

} // namespace app
} // namespace edm
