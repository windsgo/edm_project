#include "DataQueueRecordPanel.h"
#include "Motion/MotionSharedData/MotionSharedData.h"
#include "config.h"
#include "ui_DataQueueRecordPanel.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QPushButton>
#include <fstream>
#include <qfiledialog.h>
#include <qpushbutton.h>
#include <sstream>

#include "Logger/LogMacro.h"
EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace app {

DataQueueRecordPanel::DataQueueRecordPanel(SharedCoreData *shared_core_data,
                                           QWidget *parent)
    : QWidget(parent), ui(new Ui::DataQueueRecordPanel),
      shared_core_data_(shared_core_data) {
    ui->setupUi(this);

    _init_dirs();

    _init_record_data1();
}

DataQueueRecordPanel::~DataQueueRecordPanel() { delete ui; }

static void _check_and_create_dir(const QString &dir_str) {
    QDir dir;
    if (!dir.exists(dir_str)) {
        s_logger->info("creating dir: {}", dir_str.toStdString());
        dir.mkpath(dir_str);
    }
}

void DataQueueRecordPanel::_init_dirs() {
    // init dir
    _check_and_create_dir(DataSaveRootDir);
    _check_and_create_dir(RecordData1BinDir);
    _check_and_create_dir(RecordData1DecodeDir);
}

void DataQueueRecordPanel::_init_record_data1() {
    // 获取数据1记录器指针, 用于启动停止
    motion_record_data1_recorder_ =
        move::MotionSharedData::instance()->get_record_data1_queuerecorder();

    connect(ui->pb_start_record_1, &QPushButton::clicked, this,
            [this](bool checked) {
                if (checked) {
                    // generate save filename
                    auto bin_file = RecordData1BinDir +
                                    QDateTime::currentDateTime().toString(
                                        "yyyyMMdd_hhmmss_zzz") +
                                    ".bin";

                    // start record
                    auto ret = motion_record_data1_recorder_->start_record(
                        bin_file.toStdString());

                    if (ret) {
                        emit shared_core_data_->sig_info_message(
                            "Start Record Success");
                    } else {
                        emit shared_core_data_->sig_error_message(
                            "Start Record Failed");
                        ui->pb_start_record_1->setChecked(false);
                    }

                } else {
                    // stop and wait for stopped
                    motion_record_data1_recorder_->stop_record(true);

                    emit shared_core_data_->sig_info_message(
                        "Stop Record Success");
                }
            });

    connect(ui->pb_decode_1, &QPushButton::clicked, this, [this]() {
        // get input files
        auto bin_filenames = QFileDialog::getOpenFileNames(
            this, tr("Select Bin Files"), RecordData1BinDir);

        for (const auto &bin_filename : bin_filenames) {
            QFileInfo fi(bin_filename);
            auto decode_filename =
                RecordData1DecodeDir + fi.baseName() + ".txt";

            std::ifstream ifs(bin_filename.toStdString(), std::ios::binary);
            if (!ifs.is_open()) {
                emit shared_core_data_->sig_error_message(
                    QString{"decode file failed: %0, open bin failed"}.arg(
                        bin_filename));
                continue;
            }

            std::ofstream ofs(decode_filename.toStdString());
            if (!ofs.is_open()) {
                ifs.close();
                emit shared_core_data_->sig_error_message(
                    QString{"decode file failed: %0, open txt failed"}.arg(
                        bin_filename));
                continue;
                ;
            }

            ifs.seekg(0, std::ios::end);
            int size = ifs.tellg();

            int data_num = size / sizeof(move::MotionSharedData::RecordData1);
            std::cout << "size: " << size << ", datas: " << data_num
                      << std::endl;

            ifs.seekg(0, std::ios::beg);

            ofs.setf(std::ios::fixed, std::ios::floatfield);
            ofs.precision(4);

            // header
            if (ui->cb_include_header_1->isChecked()) {
                ofs << _generate_data1_header() << '\n';
            }

            for (int i = 0; i < data_num; ++i) {
                move::MotionSharedData::RecordData1 data;
                ifs.read((char *)&data, sizeof(data));

                ofs << data.thread_tick_us << '\t';

                for (int i = 0; i < EDM_AXIS_NUM; ++i) {
                    ofs << (int)data.new_cmd_axis[i] << '\t';
                }
                for (int i = 0; i < EDM_AXIS_NUM; ++i) {
                    ofs << (int)data.act_axis[i] << '\t';
                }
                for (int i = 0; i < EDM_AXIS_NUM; ++i) {
                    ofs << (int)data.following_error_axis[i] << '\t';
                }
                ofs << data.g01_servo_cmd << '\t'
                    << (int)data.is_g01_normal_servoing << '\t'
                    << (int)data.average_voltage << '\t' << (int)data.current
                    << '\t' << (int)data.normal_charge_rate << '\t'
                    << (int)data.short_charge_rate << '\t'
                    << (int)data.open_charge_rate << '\n';
            }

            ifs.close();
            ofs.close();

            emit shared_core_data_->sig_info_message(
                QString{"Decode Success, saved to: %0"}.arg(decode_filename));
        }
    });

    connect(ui->pb_print_header_1, &QPushButton::clicked, this, [this]() {
        auto ret = _save_data1_header_to_file(data1_header_file_.toStdString());

        if (ret) {
            emit shared_core_data_->sig_info_message(
                QString{"Print Header Success, saved to: %0"}.arg(data1_header_file_));
        } else {
            emit shared_core_data_->sig_warn_message(
                QString{"Print Header Failed, file: %0"}.arg(data1_header_file_));
        }
    });
}

std::string DataQueueRecordPanel::_generate_data1_header() const {
    std::stringstream ss;

    ss << "tick_us" << '\t';

    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << "cmd" << i << '\t';
    }
    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << "act" << i << '\t';
    }
    for (int i = 0; i < EDM_AXIS_NUM; ++i) {
        ss << "err" << i << '\t';
    }
    ss << "servocmd" << '\t' << "isg01" << '\t' << "avgvol" << '\t' << "current"
       << '\t' << "normal" << '\t' << "short" << '\t' << "open";

    return ss.str();
}

bool DataQueueRecordPanel::_save_data1_header_to_file(const std::string& filename) const {
    auto header_str = _generate_data1_header();

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        s_logger->error(
            "_save_data1_header_to_file failed: cannot open file : {}",
            filename);
        return false;
    }

    ofs << header_str;

    ofs.close();

    return true;
}

} // namespace app
} // namespace edm
