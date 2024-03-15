#include "IOPanel.h"
#include "ui_IOPanel.h"

#include "Exception/exception.h"

#include <QDebug>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

IOPanel::IOPanel(SharedCoreData *shared_core_data, QWidget *parent)
    : QWidget(parent), ui(new Ui::IOPanel), shared_core_data_(shared_core_data),
      io_ctrler_(shared_core_data->get_io_ctrler()) {
    ui->setupUi(this);

    update_io_timer_ = new QTimer(this);
    connect(update_io_timer_, &QTimer::timeout, this, &IOPanel::_update_all_io_display);
    update_io_timer_->start(350);

    _init_common_io_buttons();
}

IOPanel::~IOPanel() { delete ui; }

void IOPanel::slot_update_io_display() {
    _update_all_io_display();
}

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

    _layout_button_columes_priority(6);

    map_io_to_button_[power::EleContactorOut_BZ_JF2]->setEnabled(false);

    ui->groupBox_io->setLayout(io_button_layout_);
}

void IOPanel::_add_button_to_gridlayout(const QString &button_name,
                                        uint32_t out_num, int row, int col) {
    auto pb = new QPushButton(button_name);
    pb->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Preferred,
                                  QSizePolicy::Policy::Preferred));
    pb->setMinimumSize(QSize{60, 40});
    pb->setMaximumSize(QSize{120, 60});

    pb->setCheckable(true);

    io_button_layout_->addWidget(pb, row, col);

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

void IOPanel::_layout_button_columes_priority(uint32_t col_nums) {
    constexpr static const uint32_t total_io_num = 48;
    uint32_t row_nums = std::ceil((double)total_io_num / (double)col_nums);

    // qDebug() << "col_nums:" << col_nums << "row_nums:" << row_nums;

    for (std::size_t io = 1; io <= total_io_num; ++io) {
        uint32_t row = (io - 1) % row_nums;
        uint32_t col = (io - 1) / row_nums;

        // qDebug() << "io:" << io << "row:" << row << "col:" << col;

        auto io_name_opt = _get_io_name(io);
        QString button_name = io_name_opt
                                ? QString{"OUT%0: %1"}.arg(io).arg(*io_name_opt)
                                : QString{"OUT%0"}.arg(io);

        _add_button_to_gridlayout(button_name, io, row, col);
    }
}

void IOPanel::_update_all_io_display() {
    // get io
    auto io_1 = io_ctrler_->get_can_machineio_1_safe();
    auto io_2 = io_ctrler_->get_can_machineio_2_safe();

    // update io buttons
    for (auto& [io, btn] : map_io_to_button_ ) {
        if (io <= 32) {
            uint32_t io_bit = 1 << (io - 1);
            btn->setChecked(!!(io_1 & io_bit));
        } else {
            uint32_t io_bit = 1 << (io - 33);
            btn->setChecked(!!(io_2 & io_bit));
        }
    }

    // TODO specfic other io buttons
}

void IOPanel::_button_clicked(uint32_t io_num, bool checked) {
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

    _update_all_io_display();
}

} // namespace app
} // namespace edm
