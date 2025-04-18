#include "IOPanel.h"
#include "QtDependComponents/PowerController/EleparamDefine.h"
#include "ui_IOPanel.h"

#include "Exception/exception.h"

#include <QDebug>
#include <qobject.h>
#include <qpushbutton.h>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

IOPanel::IOPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::IOPanel), shared_core_data_(shared_core_data),
      io_ctrler_(shared_core_data->get_io_ctrler()) {
    ui->setupUi(this);

    update_io_timer_ = new QTimer(this);
    connect(update_io_timer_, &QTimer::timeout, this,
            &IOPanel::_update_all_io_display);
    update_io_timer_->start(350);

    _init_common_io_buttons();
    _init_handbox_pump_signal();
}

IOPanel::~IOPanel() { delete ui; }

void IOPanel::slot_update_io_display() { _update_all_io_display(); }

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
static std::unordered_map<uint32_t, QString> s_io_names_map = {
    {2, "LV1"},    {3, "LV2"},

    {21, "C0"},    {22, "C1"},   {23, "C2"},   {24, "C3"},  {25, "C4"},
    {26, "C5"},    {27, "C6"},   {28, "C7"},   {29, "C8"},  {30, "C9"},

    {31, "MACH"},  {32, "PWON"},

    {33, "BZ(x)"}, {34, "HON"},  {35, "MON"},

    {37, "PK"},    {38, "FULD"}, {39, "RVNM"}, {40, "PK0"},

    {42, "IP0"},   {43, "IP7"},  {44, "NOW"},

    {46, "IP15"},  {47, "SOF"},
};
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU)
static std::unordered_map<uint32_t, QString> s_io_names_map = {
    {power::ZHONGGU_IOOut_IOOUT1_FULD, "FULD"},
    {power::ZHONGGU_IOOut_IOOUT2_NEG, "NEG"},
    {power::ZHONGGU_IOOut_IOOUT3_MACH, "MACH"},
    {power::ZHONGGU_IOOut_IOOUT4_BZ, "BZ"},
    {power::ZHONGGU_IOOut_IOOUT5_LIGHT, "LIGHT"},
    {power::ZHONGGU_IOOut_IOOUT6_RED, "RED"},
    {power::ZHONGGU_IOOut_IOOUT7_YELLOW, "YELLOW"},
    {power::ZHONGGU_IOOut_IOOUT8_GREEN, "GREEN"},
    {power::ZHONGGU_IOOut_IOOUT9_TOOL_FIXTURE, QObject::tr("松电极")},
    {power::ZHONGGU_IOOut_IOOUT10_WORK_FIXTURE, QObject::tr("松工件")},
    {power::ZHONGGU_IOOut_IOOUT11_HP1, "HP1"},
    {power::ZHONGGU_IOOut_IOOUT12_HP2, "HP2"},
    {power::ZHONGGU_IOOut_TON1, "TON1"},
    {power::ZHONGGU_IOOut_TON2, "TON2"},
    {power::ZHONGGU_IOOut_TON4, "TON4"},
    {power::ZHONGGU_IOOut_TON8, "TON8"},
    {power::ZHONGGU_IOOut_TON16, "TON16"},
    {power::ZHONGGU_IOOut_TOFF1, "TOFF1"},
    {power::ZHONGGU_IOOut_TOFF2, "TOFF2"},
    {power::ZHONGGU_IOOut_TOFF4, "TOFF4"},
    {power::ZHONGGU_IOOut_TOFF8, "TOFF8"},
    {power::ZHONGGU_IOOut_IP1, "IP1"},
    {power::ZHONGGU_IOOut_IP2, "IP2"},
    {power::ZHONGGU_IOOut_IP4, "IP4"},
    {power::ZHONGGU_IOOut_IP8, "IP8"}};

static std::unordered_map<uint32_t, QString> s_io_input_names_map = {
    {power::ZHONGGU_IOIn_IOIN1_PRESSURE, QObject::tr("气压")},
    {power::ZHONGGU_IOIn_IOIN2, QObject::tr("IN2")},
    {power::ZHONGGU_IOIn_IOIN5_TOOL_FIX_BTN, QObject::tr("松电极按钮")},
    {power::ZHONGGU_IOIn_IOIN6_WORK_FIX_BTN, QObject::tr("松工件按钮")},
    {power::ZHONGGU_IOIn_IOIN7, QObject::tr("IN7")},
    {power::ZHONGGU_IOIn_IOIN8, QObject::tr("IN8")},
    {power::ZHONGGU_IOIn_IOIN12_TOOL_EXIST, QObject::tr("电极检测")},
    {power::ZHONGGU_IOIn_IOIN13_WORK_EXIST, QObject::tr("工件检测")},
    {power::ZHONGGU_IOIn_IOIN14_TOOL_PRESSURE, QObject::tr("电极气压")},
    {power::ZHONGGU_IOIn_IOIN15_WORK_PRESSURE, QObject::tr("工件气压")}};

static inline std::optional<QString> _get_io_input_name(uint32_t io) {
    std::optional<QString> ioname{std::nullopt};

    auto ret = s_io_input_names_map.find(io);
    if (ret != s_io_input_names_map.end()) {
        ioname = ret->second;
    }

    return ioname;
}

#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
static std::unordered_map<uint32_t, QString> s_io_names_map = {
    {power::ZHONGGU_IOOut_IOOUT1_OPUMP, QObject::tr("外冲")},
    {power::ZHONGGU_IOOut_IOOUT2_IPUMP, QObject::tr("内冲")},
    {power::ZHONGGU_IOOut_IOOUT3_MACH, "MACH"},
    {power::ZHONGGU_IOOut_IOOUT4_BZ, "BZ"},
    {power::ZHONGGU_IOOut_CAP1, "CAP1"},
    {power::ZHONGGU_IOOut_CAP2, "CAP2"},
    {power::ZHONGGU_IOOut_CAP4, "CAP4"},
    {power::ZHONGGU_IOOut_CAP8, "CAP8"},
    {power::ZHONGGU_IOOut_IOOUT5_LIGHT, "LIGHT"},
    {power::ZHONGGU_IOOut_IOOUT6_RED, "RED"},
    {power::ZHONGGU_IOOut_IOOUT7_YELLOW, "GREEN"},
    {power::ZHONGGU_IOOut_IOOUT8_GREEN, "YELLOW"}, // 灯绿和黄接反了
    {power::ZHONGGU_IOOut_IOOUT9_TOOL_FIXTURE, QObject::tr("松电极")},
    {power::ZHONGGU_IOOut_IOOUT10_FUSI_IN, QObject::tr("扶丝入")},
    {power::ZHONGGU_IOOut_IOOUT11_FUSI_OUT, QObject::tr("扶丝出")},
    {power::ZHONGGU_IOOut_IOOUT12_WORK_FIXTURE, QObject::tr("松工件")},
    {power::ZHONGGU_IOOut_TON1, "TON1"},
    {power::ZHONGGU_IOOut_TON2, "TON2"},
    {power::ZHONGGU_IOOut_TON4, "TON4"},
    {power::ZHONGGU_IOOut_TON8, "TON8"},
    {power::ZHONGGU_IOOut_TOFF1, "TOFF1"},
    {power::ZHONGGU_IOOut_TOFF2, "TOFF2"},
    {power::ZHONGGU_IOOut_TOFF4, "TOFF4"},
    {power::ZHONGGU_IOOut_TOFF8, "TOFF8"},
    {power::ZHONGGU_IOOut_IP1, "IP1"},
    {power::ZHONGGU_IOOut_IP2, "IP2"},
    {power::ZHONGGU_IOOut_IP4, "IP4"},
    {power::ZHONGGU_IOOut_IP8, "IP8"},
    {power::ZHONGGU_IOOut_WORK, QObject::tr("WORK")},
    {power::ZHONGGU_IOOut_TOOL, QObject::tr("TOOL")} //! TODO 待测试反极性，先不开放
};

static std::unordered_map<uint32_t, QString> s_io_input_names_map = {
    {power::ZHONGGU_IOIn_IOIN1_TOTAL_PRESSURE, QObject::tr("总气压")},
    {power::ZHONGGU_IOIn_IOIN2_WATER_PRESSURE, QObject::tr("水压")},
    {power::ZHONGGU_IOIn_IOIN3_WATER_QUALITY, QObject::tr("水质")},
    {power::ZHONGGU_IOIn_IOIN4_WATER_LEVEL, QObject::tr("水位")},
    {power::ZHONGGU_IOIn_IOIN5_TOOL_FIX_BTN, QObject::tr("松电极按钮")},
    {power::ZHONGGU_IOIn_IOIN6_PROTECT_DOOR, QObject::tr("防护门")},
    {power::ZHONGGU_IOIn_IOIN7_FUSI_IN, QObject::tr("扶丝收")},
    {power::ZHONGGU_IOIn_IOIN8_FUSI_OUT, QObject::tr("扶丝出")},
    {power::ZHONGGU_IOIn_IOIN9_DIRECTORT_LEFT, QObject::tr("导向左")},
    {power::ZHONGGU_IOIn_IOIN10_DIRECTORT_RIGHT, QObject::tr("导向右")},
    {power::ZHONGGU_IOIn_IOIN11_DIRECTORT_ZERO, QObject::tr("原点")},
    {power::ZHONGGU_IOIn_IOIN12_TOOL_EXIST, QObject::tr("电极检测")},
    {power::ZHONGGU_IOIn_IOIN13_WORK_EXIST, QObject::tr("工件检测")},
    {power::ZHONGGU_IOIn_IOIN14_TOOL_PRESSURE, QObject::tr("电极气压")},
    {power::ZHONGGU_IOIn_IOIN15_WORK_PRESSURE, QObject::tr("工件气压")},
    {power::ZHONGGU_IOIn_IOIN16_H_WARN, QObject::tr("H报警")},
    {power::ZHONGGU_IOIn_IOIN17_H_ZERO, QObject::tr("H原点")},
};

static inline std::optional<QString> _get_io_input_name(uint32_t io) {
    std::optional<QString> ioname{std::nullopt};

    auto ret = s_io_input_names_map.find(io);
    if (ret != s_io_input_names_map.end()) {
        ioname = ret->second;
    }

    return ioname;
}

#endif

static inline std::optional<QString> _get_io_name(uint32_t io) {
    std::optional<QString> ioname{std::nullopt};

    auto ret = s_io_names_map.find(io);
    if (ret != s_io_names_map.end()) {
        ioname = ret->second;
    }

    return ioname;
}

void IOPanel::_init_common_io_buttons() {
    io_button_layout_ = new QGridLayout();

    _layout_button_rows_priority(6);

#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    map_io_to_button_[power::EleContactorOut_BZ_JF2]->setEnabled(false);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) 
    // map_io_to_button_[power::ZHONGGU_IOOut_IOOUT4_BZ]->setEnabled(false);
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    map_io_to_button_[power::ZHONGGU_IOOut_TOOL]->setEnabled(false); //! TODO 待测试反极性，先不开放
#endif

    ui->groupBox_io->setLayout(io_button_layout_);
}

QPushButton *IOPanel::_make_button_at_gridlayout(const QString &button_name,
                                                 int row, int col) {
    auto pb = new QPushButton(button_name);
    pb->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Preferred,
                                  QSizePolicy::Policy::Preferred));
    pb->setMinimumSize(QSize{60, 40});
    pb->setMaximumSize(QSize{120, 60});

    pb->setCheckable(true);

    io_button_layout_->addWidget(pb, row, col);

    return pb;
}

void IOPanel::_set_button_output(QPushButton *pb, uint32_t out_num) {
    pb->setEnabled(true);
    map_io_to_button_.emplace(out_num, pb);
    map_button_to_io_.emplace(pb, out_num);

    QObject::connect(pb, &QPushButton::clicked, this, [this](bool checked) {
        auto sender_pb = static_cast<QPushButton *>(QObject::sender());

        // should be safe ...
        uint32_t sender_pb_out_num = this->map_button_to_io_[sender_pb];

        s_logger->debug("clicked: name: {}, io: {}, on: {}",
                        sender_pb->text().toStdString(), sender_pb_out_num,
                        checked);

        this->_button_clicked(sender_pb_out_num, checked);
    });
}

void IOPanel::_set_button_input(QPushButton *pb, uint32_t in_num) {
    pb->setEnabled(false);
    map_inputio_to_dispbtn_.emplace(in_num, pb);
}

void IOPanel::_layout_button_rows_priority(uint32_t row_nums) {
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    constexpr static const uint32_t total_io_num = 48;
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU)
    constexpr static const uint32_t total_io_num = power::ZHONGGU_IOOut_IP8;
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    constexpr static const uint32_t total_io_num = power::ZHONGGU_IOOut_TOOL;
#endif
    // qDebug() << "col_nums:" << col_nums << "row_nums:" << row_nums;

    for (std::size_t io = 1; io <= total_io_num; ++io) {
        uint32_t row = (io - 1) % row_nums;
        uint32_t col = (io - 1) / row_nums;

        // qDebug() << "io:" << io << "row:" << row << "col:" << col;

        auto io_name_opt = _get_io_name(io);
        QString button_name = io_name_opt
                                ? QString{"OUT%0:\n%1"}.arg(io).arg(*io_name_opt)
                                : QString{"OUT%0"}.arg(io);

        auto pb = _make_button_at_gridlayout(button_name, row, col);
        _set_button_output(pb, io);
    }

#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    // INPUT display
    uint32_t input_io_count = 1;
    const uint32_t output_io_cols = std::ceil((double)total_io_num / (double)row_nums); 

    for (std::size_t io = 1; io <= 17; ++io) {
        uint32_t row = (input_io_count - 1) % row_nums;
        uint32_t col = (input_io_count - 1) / row_nums + output_io_cols;

        // qDebug() << "io:" << io << "row:" << row << "col:" << col;

        auto io_name_opt = _get_io_input_name(io);

        if (io_name_opt) {
            QString button_name = QString{"IN%0:\n%1"}.arg(io).arg(*io_name_opt);

            auto pb = _make_button_at_gridlayout(button_name, row, col);
            _set_button_input(pb, io);

            ++input_io_count;
        }
    }
#endif
}

void IOPanel::_init_handbox_pump_signal() {
    connect(this->shared_core_data_, &SharedCoreData::sig_handbox_pump, this,
            [this](bool pump_on) {
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
                auto btn =
                    this->map_io_to_button_[power::EleContactorOut_FULD_JF7];
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU)
                auto btn =
                    this->map_io_to_button_[power::ZHONGGU_IOOut_IOOUT1_FULD];
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
                auto btn =
                    this->map_io_to_button_[power::ZHONGGU_IOOut_IOOUT1_OPUMP]; // 小孔机控制外冲液
#endif
                btn->setChecked(pump_on);
                emit btn->clicked(pump_on);
            });
}

void IOPanel::_update_all_io_display() {
    // get io
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    auto io_1 = io_ctrler_->get_can_machineio_1_safe();
    auto io_2 = io_ctrler_->get_can_machineio_2_safe();

    // update io buttons
    for (auto &[io, btn] : map_io_to_button_) {
        if (io <= 32) {
            uint32_t io_bit = 1 << (io - 1);
            btn->setChecked(!!(io_1 & io_bit));
        } else {
            uint32_t io_bit = 1 << (io - 33);
            btn->setChecked(!!(io_2 & io_bit));
        }
    }
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    auto io_output = io_ctrler_->get_can_machineio_output_safe();
    auto io_input = io_ctrler_->get_can_machineio_input();

    for (auto &[io, btn] : map_io_to_button_) {
        uint32_t io_bit = 1 << (io - 33);
        btn->setChecked(!!(io_output & io_bit));
    }

    // INPUT
    for (auto &[io, btn] : map_inputio_to_dispbtn_) {
        uint32_t io_bit = 1 << (io - 33);
        btn->setChecked(!!(io_input & io_bit));
    }
    // map_inputio_to_dispbtn_[power::ZHONGGU_IOIn_IOIN2]->setChecked(true);
#endif

    // TODO specfic other io buttons
}

void IOPanel::_button_clicked(uint32_t io_num, bool checked) {
#if (EDM_POWER_TYPE == EDM_POWER_DIMEN)
    if (io_num <= 32) {
        uint32_t io_bit = 1 << (io_num - 1);
        if (checked) {
            io_ctrler_->set_can_machineio_1_withmask(io_bit, io_bit);
        } else {
            io_ctrler_->set_can_machineio_1_withmask(0x00, io_bit);
        }
    } else if (io_num <= 64) {
        uint32_t io_bit = 1 << (io_num - 33);
        if (checked) {
            io_ctrler_->set_can_machineio_2_withmask(io_bit, io_bit);
        } else {
            io_ctrler_->set_can_machineio_2_withmask(0x00, io_bit);
        }
    }
#elif (EDM_POWER_TYPE == EDM_POWER_ZHONGGU) || (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    if (io_num <= 32) {
        uint32_t io_bit = 1 << (io_num - 1);
        if (checked) {
            io_ctrler_->set_can_machineio_output_withmask(io_bit, io_bit);
        } else {
            io_ctrler_->set_can_machineio_output_withmask(0x00, io_bit);
        }
    }
#endif

    _update_all_io_display();
}

} // namespace app
} // namespace edm
