#include "testpowerwidget.h"
#include "ui_testpowerwidget.h"

#include <QFile>
#include <QStyle>

static auto s_can_ctrler = can::CanController::instance();
static auto s_io_ctrler = io::IOController::instance();
static auto s_power_ctrler = power::PowerController::instance();

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

TestPowerWidget::TestPowerWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestPowerWidget) {
    ui->setupUi(this);

    _init_io_buttons();
    _init_common_buttons();

    // init the eleparam
    // init the power controller once by eleparam
    curr_eleparam_ = std::make_shared<power::EleParam_dkd_t>();
    //  get from gui or set to gui
    _update_eleparam_from_ui_and_send();

    // init timers
    ioboard_pulse_timer_ = new QTimer(this);
    QObject::connect(ioboard_pulse_timer_, &QTimer::timeout, this, [&]() {
        s_power_ctrler->trigger_send_ioboard_eleparam();
    });
    ioboard_pulse_timer_->start(1000);

    ele_cycle_timer_ = new QTimer(this);
    QObject::connect(ele_cycle_timer_, &QTimer::timeout, this,
                     [&]() { s_power_ctrler->trigger_send_eleparam(); });
    ele_cycle_timer_->start(1000);

    _update_widget_status();
}

TestPowerWidget::~TestPowerWidget() { delete ui; }

void TestPowerWidget::_add_io_button(const QString &io_name, uint32_t out_num,
                                     int row, int col) {
    auto pb = new QPushButton(io_name);
    pb->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Preferred,
                                  QSizePolicy::Policy::Preferred));
    pb->setCheckable(true);

    io_button_layout_->addWidget(pb, row, col);

    io_button_map_.insert(out_num, pb);
    button_io_map_.insert(pb, out_num);

    QObject::connect(pb, &QPushButton::clicked, this, [this](bool checked) {
        auto sender_pb = static_cast<QPushButton *>(QObject::sender());

        uint32_t sender_pb_out_num = this->button_io_map_[sender_pb];

        s_logger->debug("clicked: name: {}, io: {}, on: {}",
                        sender_pb->text().toStdString(), sender_pb_out_num,
                        checked);

        this->_button_clicked(sender_pb_out_num, checked);
    });
}

void TestPowerWidget::_button_clicked(uint32_t io_num, bool checked) {
    // s_logger->debug("_button_clicked: {}, {}", io_num, checked);
    if (io_num <= 32) {
        uint32_t io_bit = 1 << (io_num - 1);
        if (checked) {
            s_io_ctrler->set_can_machineio_1_withmask(io_bit, io_bit);
        } else {
            s_io_ctrler->set_can_machineio_1_withmask(0x00, io_bit);
        }
    } else if (io_num <= 64) {
        uint32_t io_bit = 1 << (io_num - 33);
        if (checked) {
            s_io_ctrler->set_can_machineio_2_withmask(io_bit, io_bit);
        } else {
            s_io_ctrler->set_can_machineio_2_withmask(0x00, io_bit);
        }
    }

    _update_widget_status();
}

void TestPowerWidget::_update_widget_status() {
    // get io
    auto io_1 = s_io_ctrler->get_can_machineio_1_safe();
    auto io_2 = s_io_ctrler->get_can_machineio_2_safe();

    // update io buttons
    for (auto it = io_button_map_.begin(); it != io_button_map_.end(); ++it) {
        if (it.key() <= 32) {
            uint32_t io_bit = 1 << (it.key() - 1);
            it.value()->setChecked(!!(io_1 & io_bit));
        } else {
            uint32_t io_bit = 1 << (it.key() - 33);
            it.value()->setChecked(!!(io_2 & io_bit));
        }
    }

    // update common buttons
    ui->pb_PowerOn->setChecked(s_power_ctrler->is_power_on());
    ui->pb_HighPowerOn->setChecked(s_power_ctrler->is_highpower_on());
    ui->pb_MachBitOn->setChecked(s_power_ctrler->is_machbit_on());
}

template <typename T>
T GetAndCorrectLineEditValue(QLineEdit *le, const T &default_value) {
    bool ok = false;
    auto value = static_cast<T>(le->text().toUInt(&ok));
    if (!ok) {
        value = default_value;
    }

    le->setText(QString::number(value));

    return value;
}

#define ASSIGN_FROM_LE(assigned_to__, le__, default_value__)                   \
    {                                                                          \
        (assigned_to__) = GetAndCorrectLineEditValue<decltype(assigned_to__)>( \
            (le__), (default_value__));                                        \
    }

void TestPowerWidget::_update_eleparam_from_ui_and_send() {
    // TODO
    ASSIGN_FROM_LE(curr_eleparam_->pulse_on, ui->le_ele_on, 10);
    ASSIGN_FROM_LE(curr_eleparam_->pulse_off, ui->le_ele_off, 10);
    ASSIGN_FROM_LE(curr_eleparam_->ip, ui->le_ele_ip, 10);
    ASSIGN_FROM_LE(curr_eleparam_->hp, ui->le_ele_hp, 10);
    ASSIGN_FROM_LE(curr_eleparam_->pp, ui->le_ele_pp, 10);
    ASSIGN_FROM_LE(curr_eleparam_->lv, ui->le_ele_lv, 1);
    ASSIGN_FROM_LE(curr_eleparam_->c, ui->le_ele_c, 1);
    ASSIGN_FROM_LE(curr_eleparam_->ma, ui->le_ele_ma, 1);
    ASSIGN_FROM_LE(curr_eleparam_->al, ui->le_ele_al, 1);
    ASSIGN_FROM_LE(curr_eleparam_->oc, ui->le_ele_oc, 1);
    ASSIGN_FROM_LE(curr_eleparam_->pl, ui->le_ele_pl, 1);
    ASSIGN_FROM_LE(curr_eleparam_->upper_index, ui->le_ele_upperindex, 100);

    ASSIGN_FROM_LE(curr_eleparam_->sv, ui->le_ele_sv, 6);
    ASSIGN_FROM_LE(curr_eleparam_->servo_sensitivity,
                   ui->le_ele_servosensitivity, 100);
    ASSIGN_FROM_LE(curr_eleparam_->UpperThreshold, ui->le_ele_upperthreshold,
                   1);
    ASSIGN_FROM_LE(curr_eleparam_->LowerThreshold, ui->le_ele_lowerthreshold,
                   1);
    ASSIGN_FROM_LE(curr_eleparam_->servo_speed, ui->le_ele_servospeed, 3);

    s_power_ctrler->update_eleparam_and_send(curr_eleparam_);
}

void TestPowerWidget::_init_io_buttons() {
    io_button_layout_ = new QGridLayout();

    _add_io_button(QStringLiteral("OUT2/LV1"), 2, 0, 0);
    _add_io_button(QStringLiteral("OUT3/LV2"), 3, 1, 0);

    _add_io_button(QStringLiteral("OUT21/C0"), 21, 0, 1);
    _add_io_button(QStringLiteral("OUT22/C1"), 22, 1, 1);
    _add_io_button(QStringLiteral("OUT23/C2"), 23, 2, 1);

    _add_io_button(QStringLiteral("OUT31/MACH"), 31, 0, 2);
    _add_io_button(QStringLiteral("OUT32/PWON"), 32, 1, 2);

    _add_io_button(QStringLiteral("OUT34/HON"), 34, 0, 3);
    _add_io_button(QStringLiteral("OUT35/MON"), 35, 1, 3);
    _add_io_button(QStringLiteral("OUT37/PK"), 37, 2, 3);
    _add_io_button(QStringLiteral("OUT38/FULD"), 38, 3, 3);
    _add_io_button(QStringLiteral("OUT39/极性"), 39, 4, 3);

    _add_io_button(QStringLiteral("OUT42/IP0"), 42, 0, 4);
    _add_io_button(QStringLiteral("OUT43/IP7"), 43, 1, 4);
    _add_io_button(QStringLiteral("OUT44/NOW"), 44, 2, 4);
    _add_io_button(QStringLiteral("OUT46/IP15"), 46, 3, 4);
    _add_io_button(QStringLiteral("OUT47/SOF"), 47, 4, 4);

    ui->frame_io->setLayout(io_button_layout_);
}

void TestPowerWidget::_init_common_buttons() {
    QObject::connect(ui->pb_UpdateAllStatus, &QPushButton::clicked, this,
                     &TestPowerWidget::_update_widget_status);

    QObject::connect(ui->pb_PowerOn, &QPushButton::clicked, this,
                     [this](bool checked) {
                         s_logger->trace("pb_PowerOn : {}", checked);
                         s_power_ctrler->set_power_on(checked);
                         _update_widget_status();
                     });

    QObject::connect(
        ui->pb_HighPowerOn, &QPushButton::clicked, this, [this](bool checked) {
            s_logger->trace("pb_HighPowerOn : {}", checked);
            s_power_ctrler->set_highpower_on(checked);
            s_power_ctrler->update_eleparam_and_send(this->curr_eleparam_);
            _update_widget_status();
        });

    QObject::connect(
        ui->pb_MachBitOn, &QPushButton::clicked, this, [this](bool checked) {
            s_logger->trace("pb_MachBitOn : {}", checked);
            s_power_ctrler->set_machbit_on(checked);
            s_power_ctrler->update_eleparam_and_send(this->curr_eleparam_);
            _update_widget_status();
        });

    QObject::connect(ui->pb_SetEleParam, &QPushButton::clicked, this, [this]() {
        s_logger->trace("pb_SetEleParam");
        _update_eleparam_from_ui_and_send();
        _update_widget_status();
    });

    QObject::connect(ui->pb_TriggleSendIO, &QPushButton::clicked, this,
                     [this]() {
                         s_logger->trace("pb_TriggleSendIO");
                         s_power_ctrler->trigger_send_contactors_io();
                         _update_widget_status();
                     });
}

static void init_system() {
    s_can_ctrler->init();

    int can0 = s_can_ctrler->add_device("can0", 500000);
    // int can1 = s_can_ctrler->add_device("can1", 115200);

    // while (!s_can_ctrler->is_connected("can0") ||
    //        s_can_ctrler->is_connected("can1"))
    //     ;

    while (!s_can_ctrler->is_connected(can0))
        ;

    s_io_ctrler->init(can0);
    s_power_ctrler->init(can0);

    // s_can_ctrler->add_frame_received_listener(can1,
    //                                           [&](const QCanBusFrame &frame)
    //                                           {
    //                                               // s_logger->debug("{}",
    //                                               //
    //                                               frame.toString().toStdString());
    //                                           });
}

int main(int argc, char **argv) {
    s_logger->info("");
    s_logger->info("***********************************");
    s_logger->info("****     New Gui Power Test    ****");
    s_logger->info("***********************************");
    s_logger->info("");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    QFile stylefile(EDM_ROOT_DIR "Conf/gui.qss");
    stylefile.open(QFile::ReadOnly);
    // s_logger->info("style file: {}", EDM_ROOT_DIR "Conf/gui.qss");
    if (stylefile.isOpen()) {
        app.setStyleSheet(stylefile.readAll());
    } else {
        s_logger->error("style sheet error");
    }
    stylefile.close();

    init_system();

    TestPowerWidget w;
    w.show();

    int ret = app.exec();

    s_can_ctrler->terminate();

    return ret;
}
