#include "ADCCalcPanel.h"
#include "ui_ADCCalcPanel.h"

#include "Logger/LogMacro.h"
#include <utility>
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

ADCCalcPanel::ADCCalcPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ADCCalcPanel)
{
    ui->setupUi(this);

    connect(ui->pb_calc, &QPushButton::clicked, [this] {
        ADCCalcInput input;
        input.current_k = ui->dsb_current_k->value();
        input.current_b = ui->dsb_current_b->value();
        input.display_v_1 = ui->dsb_display_v_1->value();
        input.real_v_1 = ui->dsb_real_v_1->value();
        input.display_v_2 = ui->dsb_display_v_2->value();
        input.real_v_2 = ui->dsb_real_v_2->value();

        auto result_opt = CalcADC(input);
        emit sig_result_calced(result_opt);
        if (!result_opt) {
            s_logger->error("CalcADC failed");
            return;
        }

        auto result = std::move(*result_opt);
        ui->dsb_new_k->setValue(result.new_k);
        ui->dsb_new_b->setValue(result.new_b);
    });

    connect(ui->pb_apply, &QPushButton::clicked, [this] {
        ADCCalcResult result;
        result.new_k = ui->dsb_new_k->value();
        result.new_b = ui->dsb_new_b->value();

        emit sig_result_applied(result);
    });
}

ADCCalcPanel::~ADCCalcPanel()
{
    delete ui;
}

void ADCCalcPanel::slot_set_current_k_and_b_to_ui(double k, double b)
{
    ui->dsb_current_k->setValue(k);
    ui->dsb_current_b->setValue(b);
}

std::optional<ADCCalcPanel::ADCCalcResult> ADCCalcPanel::CalcADC(const ADCCalcInput &input)
{
    if (input.current_k == 0) {
        return std::nullopt;
    }

    double d1 = (input.display_v_1 - input.current_b) / input.current_k;
    double d2 = (input.display_v_2 - input.current_b) / input.current_k;

    double delta_d = d2 - d1;
    if (delta_d == 0) {
        return std::nullopt;
    }

    ADCCalcResult result;
    result.new_k = (input.real_v_2 - input.real_v_1) / delta_d;
    result.new_b = input.real_v_1 - result.new_k * d1;

    return result;
}

}
}
