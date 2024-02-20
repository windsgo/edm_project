#include "testpowerwidget.h"
#include "ui_testpowerwidget.h"

#include <QStyle>
#include <QFile>

static auto s_can_ctrler = can::CanController::instance();
static auto s_io_ctrler = io::IOController::instance();
static auto s_power_ctrler = power::PowerController::instance();

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

TestPowerWidget::TestPowerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TestPowerWidget)
{
    ui->setupUi(this);

    _init_io_buttons();
    _init_common_buttons();

    // init the eleparam
    curr_eleparam_ = std::make_shared<power::EleParam_dkd_t>();
    // TODO get from gui or set to gui

    // init the power controller once by eleparam
    s_power_ctrler->update_eleparam_and_send(curr_eleparam_);

    // init timers
    ioboard_pulse_timer_ = new QTimer(this);
    QObject::connect(ioboard_pulse_timer_, &QTimer::timeout, this, [&](){
        s_power_ctrler->trigger_send_ioboard_eleparam();
    });
    ioboard_pulse_timer_->start(1000);

    ele_cycle_timer_ = new QTimer(this);
    QObject::connect(ele_cycle_timer_, &QTimer::timeout, this, [&](){
        s_power_ctrler->trigger_send_eleparam();
    });
    ele_cycle_timer_->start(1000);
    
    _update_widget_status();
}

TestPowerWidget::~TestPowerWidget()
{
    delete ui;
}

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
}

void TestPowerWidget::_update_widget_status() {
    // get io
    auto io_1 = s_io_ctrler->get_can_machineio_1_safe();
    auto io_2 = s_io_ctrler->get_can_machineio_2_safe();

    // update io buttons
    for (auto it = io_button_map_.begin();
        it != io_button_map_.end(); ++it) {
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

void TestPowerWidget::_update_eleparam_from_ui_and_send() {
    // TODO
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
    QObject::connect(ui->pb_UpdateAllStatus, &QPushButton::clicked, this, &TestPowerWidget::_update_widget_status);

    QObject::connect(ui->pb_PowerOn, &QPushButton::clicked, this, [this](bool checked){
        s_logger->trace("pb_PowerOn : {}", checked);
        s_power_ctrler->set_power_on(checked);
    });

    QObject::connect(ui->pb_HighPowerOn, &QPushButton::clicked, this, [this](bool checked){
        s_logger->trace("pb_HighPowerOn : {}", checked);
        s_power_ctrler->set_highpower_on(checked);
        s_power_ctrler->update_eleparam_and_send(this->curr_eleparam_);
    });

    QObject::connect(ui->pb_MachBitOn, &QPushButton::clicked, this, [this](bool checked){
        s_logger->trace("pb_MachBitOn : {}", checked);
        s_power_ctrler->set_machbit_on(checked);
        s_power_ctrler->update_eleparam_and_send(this->curr_eleparam_);
    });
}

static void init_system() {
    s_can_ctrler->init();

    int can0 = s_can_ctrler->add_device("can0", 115200);
    int can1 = s_can_ctrler->add_device("can1", 115200);

    while (!s_can_ctrler->is_connected("can0") ||
           s_can_ctrler->is_connected("can1"))
        ;
    
    s_io_ctrler->init(can0);
    s_power_ctrler->init(can0);

    s_can_ctrler->add_frame_received_listener(can1, [&](const QCanBusFrame& frame){
        // s_logger->debug("{}", frame.toString().toStdString());
    });
}

int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    QFile stylefile(EDM_ROOT_DIR "Conf/gui.qss");
    stylefile.open(QFile::ReadOnly);
    s_logger->info("style file: {}", EDM_ROOT_DIR "Conf/gui.qss");
    if (stylefile.isOpen()) {
        app.setStyleSheet(stylefile.readAll());
    } else {
        s_logger->error("style sheet error");
    }
    stylefile.close();

    init_system();

    TestPowerWidget w;
    w.show();


    return app.exec();
}
