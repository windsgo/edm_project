#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QRegExp>
#include <QRegExpValidator>
#include <QString>

#include "Utils/Format/edm_format.h"

namespace edm {

namespace app {

class InputHelper final {

public:
    // 设置LineEdit的坐标模式正则合法检查器
    static inline void SetLineeditCoordValidator(QLineEdit *le,
                                                 QObject *parent) {
        // (+/-)123(.(1234))
        static QRegExp reg_exp{R"([+-]?[0-9]+\.?[0-9]{0,4})"};
        le->setValidator(new QRegExpValidator(reg_exp, parent));
    }

    // 毫米值转换为 含显式正负的QString [+-]1234.0000
    static inline QString MMDoubleToExplictlyPosNegFormatedQString(double mm) {
        return QString::fromStdString(EDM_FMT::format("{:+.4f}", mm));
    }

    // 毫米值转换为 含隐式正负的QString [-]1234.0000 (不含正号, 但有空格)
    static inline QString MMDoubleToImplictlyPosNegFormatedQString(double mm) {
        return QString::fromStdString(EDM_FMT::format("{: .4f}", mm));
    }

    // 将QString转换为double值, 如果为empty str返回0, 如果错误返回std::nullopt
    static inline std::optional<double> QStringToDouble(const QString &qstr) {
        if (qstr.isEmpty() || qstr.isNull()) {
            return 0.0;
        }

        bool ok {false};
        double ret = qstr.toDouble(&ok);
        if (!ok) {
            return std::nullopt;
        } else {
            return ret;
        }
    }
};

} // namespace app

} // namespace edm
