#include "testmotiongui.h"
#include "ui_testmotiongui.h"

#include <QFile>

#include <json.hpp>

#include <fstream>

#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

using namespace edm;

static inline QString _double_to_n_decimal_point_qstring(double value,
                                                         uint32_t n) {
    return QString::number(value, 'f', n); // NRVO
}

#define BLU_TO_STRING(blu__)            \
    _double_to_n_decimal_point_qstring( \
        (double)(blu__) / 1000.0 / EDM_BLU_PER_UM, 4)

static inline double
_qstring_to_double_with_faultdefault(const QString &qstr,
                                     double defaule_value) {
    bool ok{false};
    auto ret = qstr.toDouble(&ok);
    if (!ok) {
        return defaule_value;
    } else {
        return ret;
    }
}

#define MM_STRING_TO_BLU_DFT(str__, dft_mm__)                             \
    (_qstring_to_double_with_faultdefault((str__), (dft_mm__)) * 1000.0 * \
     EDM_BLU_PER_UM)

#define UM_STRING_TO_BLU_DFT(str__, dft_mm__) \
    (_qstring_to_double_with_faultdefault((str__), (dft_mm__)) * EDM_BLU_PER_UM)

TestMotionGui::TestMotionGui(QWidget *parent)
    : QWidget(parent), ui(new Ui::TestMotionGui) {
    ui->setupUi(this);

    //! init first
    _init_system();
}

TestMotionGui::~TestMotionGui() { delete ui; }

void TestMotionGui::_init_system() {
    _init_queues();
    _init_motion_controller();

    info_dispatcher_ =
        new InfoDispatcher(motion_signal_queue_, motion_controller_, this, 20);
    connect(info_dispatcher_, &InfoDispatcher::info_updated, this,
            &TestMotionGui::_info_slot);

    _init_button_slots();
}

void TestMotionGui::_init_queues() {
    global_command_queue_ = std::make_shared<global::GlobalCommandQueue>();
    motion_signal_queue_ = std::make_shared<move::MotionSignalQueue>();
    motion_cmd_queue_ = std::make_shared<move::MotionCommandQueue>();
}

void TestMotionGui::_init_motion_controller() {
    const char *cfg_file = EDM_ROOT_DIR "Conf/ecatdefine.json";

    std::ifstream ifs(cfg_file);

    if (!ifs.is_open()) {
        s_logger->critical("ecat cfg: {}, open failed", cfg_file);
        exit(-1);
    }
    auto parse_ret = json::parse(ifs, false);

    ifs.close();

    if (!parse_ret) {
        s_logger->critical("ecat cfg: {}, parse failed", cfg_file);
        exit(-1);
    }

    auto &&settings = (*parse_ret)["settings"];
    auto netif_name = settings["netif_name"].as_string();
    auto iomap_size = settings["iomap_size"].as_unsigned();

    motion_controller_ = std::make_shared<edm::move::MotionThreadController>(
        netif_name, motion_cmd_queue_, motion_signal_queue_, iomap_size,
        EDM_SERVO_NUM, 0);
}

void TestMotionGui::_init_button_slots() {
    _init_ecat_button();
    _init_pm_buttons();
}

void TestMotionGui::_init_ecat_button() {
    connect(ui->pb_ecat_trigger_connect, &QPushButton::clicked, this,
            [this]() { _cmd_ecat_trigger_connect(); });
}

void TestMotionGui::_init_pm_buttons() {
    connect(ui->pb_x_positive_pm, &QPushButton::pressed, this, [this]() {
        move::axis_t target{0.0};
        target[0] =
            MM_STRING_TO_BLU_DFT(ui->le_x_positive_softlimit->text(), 5000.0);
        _cmd_start_pointmove(target);
        s_logger->debug("pb_x_positive_pm pressed: target[0] = {}", target[0]);
    });

    connect(ui->pb_x_positive_pm, &QPushButton::released, this, [this]() {
        _cmd_stop_pointmove(false);
        s_logger->debug("pb_x_positive_pm released");
    });

    connect(ui->pb_x_negative_pm, &QPushButton::pressed, this, [this]() {
        move::axis_t target{0.0};
        target[0] =
            MM_STRING_TO_BLU_DFT(ui->le_x_negative_softlimit->text(), -5000.0);
        _cmd_start_pointmove(target);
        s_logger->debug("pb_x_negative_pm pressed: target[0] = {}", target[0]);
    });

    connect(ui->pb_x_negative_pm, &QPushButton::released, this, [this]() {
        _cmd_stop_pointmove(false);
        s_logger->debug("pb_x_negative_pm released");
    });
}

void TestMotionGui::_cmd_ecat_trigger_connect() {
    // create motion cmd
    auto ecat_connect_cmd =
        std::make_shared<edm::move::MotionCommandSettingTriggerEcatConnect>();

    // wrap and push to global command queue
    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [this](edm::move::MotionCommandSettingTriggerEcatConnect::ptr a) {
            this->motion_cmd_queue_->push_command(a);
        },
        ecat_connect_cmd);

    global_command_queue_->push_command(gcmd);
}

void TestMotionGui::_cmd_start_pointmove(const edm::move::axis_t &target_pos) {
    auto start_pos = info_dispatcher_->get_info().curr_cmd_axis_blu;

    edm::move::MoveRuntimePlanSpeedInput speed;
    speed.nacc =
        EDM_SERVO_PEROID_PEROID_PER_MS * ui->le_nacc_ms->text().toDouble();
    if (speed.nacc == 0) {
        speed.nacc = 60;
    }
    speed.entry_v = 0;
    speed.exit_v = 0;
    speed.cruise_v = UM_STRING_TO_BLU_DFT(ui->le_speed_um_s->text(), 1000.0);
    speed.acc0 = UM_STRING_TO_BLU_DFT(ui->le_acc_um_s2->text(), 1000.0);
    speed.dec0 = -speed.acc0;

    s_logger->debug(
        "_cmd_start_pointmove: x: {} -> {}, speed: {}, acc: {}, nacc: {}",
        start_pos[0], target_pos[0], speed.cruise_v, speed.acc0, speed.nacc);

    // create motion cmd
    auto start_pointmove_cmd =
        std::make_shared<edm::move::MotionCommandManualStartPointMove>(
            start_pos, target_pos, speed);

    // wrap and push to global command queue
    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [this](edm::move::MotionCommandManualStartPointMove::ptr a) {
            this->motion_cmd_queue_->push_command(a);
        },
        start_pointmove_cmd);

    global_command_queue_->push_command(gcmd);
}

void TestMotionGui::_cmd_stop_pointmove(bool immediate) {
    // create motion cmd
    auto stop_pointmove_command =
        std::make_shared<edm::move::MotionCommandManualStopPointMove>(
            immediate);

    // wrap and push to global command queue
    auto gcmd = edm::global::CommandCommonFunctionFactory::bind(
        [this](edm::move::MotionCommandManualStopPointMove::ptr a) {
            this->motion_cmd_queue_->push_command(a);
        },
        stop_pointmove_command);

    global_command_queue_->push_command(gcmd);
}

void TestMotionGui::_info_slot(const edm::move::MotionInfo &info) {
    ui->lb_x_cmd->setText(BLU_TO_STRING(info.curr_cmd_axis_blu[0]));
    ui->lb_x_act->setText(BLU_TO_STRING(info.curr_act_axis_blu[0]));

    ui->pbshow_ecat_connected->setChecked(info.EcatConnected());
    ui->pbshow_ecat_enabled->setChecked(info.EcatAllEnabled());
    ui->pbshow_touch_enable->setChecked(info.TouchDetectEnabled());
    ui->pbshow_touch_detected->setChecked(info.TouchDetected());
    ui->pbshow_touch_warning->setChecked(info.TouchWarning());

    switch (info.main_mode) {
    case move::MotionMainMode::Idle:
        ui->pbshow_mainmode_idle->setChecked(true);
        break;
    case move::MotionMainMode::Manual:
        ui->pbshow_mainmode_manual->setChecked(true);
        break;
    case move::MotionMainMode::Auto:
        ui->pbshow_mainmode_auto->setChecked(true);
        break;
    }

    switch (info.auto_state) {
    case move::MotionAutoState::NormalMoving:
        ui->pbshow_auto_normal->setChecked(true);
        break;
    case move::MotionAutoState::Paused:
        ui->pbshow_auto_paused->setChecked(true);
        break;
    case move::MotionAutoState::Stopped:
        ui->pbshow_auto_stopped->setChecked(true);
        break;
    default:
        break;
    }
}

int main(int argc, char **argv) {
    s_logger->info("");
    s_logger->info("***********************************");
    s_logger->info("****     New Gui Motion Test   ****");
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

    TestMotionGui m;
    m.show();

    auto ret = app.exec();

    s_logger->trace("after app.exec(), ret = {}", ret);

    return ret;
}
