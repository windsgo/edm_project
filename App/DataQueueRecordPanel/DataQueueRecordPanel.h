#pragma once

#include <QWidget>

#include "SharedCoreData/SharedCoreData.h"

#include "Utils/DataQueueRecorder/DataQueueRecorder.h"
#include "Motion/MotionSharedData/MotionSharedData.h"

#include "SystemSettings/SystemSettings.h"

namespace Ui {
class DataQueueRecordPanel;
}

namespace edm {
namespace app {

class DataQueueRecordPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DataQueueRecordPanel(SharedCoreData* shared_core_data, QWidget *parent = nullptr);
    ~DataQueueRecordPanel();

public slots: // 用于外部
    void slot_start_record_data1();
    void slot_stop_record_data1();

    void slot_start_record_data2();
    void slot_stop_record_data2();

    void slot_start_audio_record();
    void slot_stop_audio_record();

private:
    void _start_record_data1();
    void _stop_record_data1();

    void _start_record_data2();
    void _stop_record_data2();

    void _start_audio_record();
    void _stop_audio_record();

private:
    void _init_dirs();

    // data1
    void _init_record_data1();
    std::string _generate_data1_header() const;
    bool _save_data1_header_to_file(const std::string& filename) const;

    // data2
    void _init_record_data2();
#if (EDM_POWER_TYPE == EDM_POWER_ZHONGGU_DRILL)
    std::string _generate_data2_header() const;
    bool _save_data2_header_to_file(const std::string& filename) const;
#endif

    // audio record
    void _init_audio_record();

private:
    Ui::DataQueueRecordPanel *ui;

    SharedCoreData* shared_core_data_;

    // const QString DataSaveRootDir = QString::fromStdString(SystemSettings::instance().get_datasave_dir());
    // const QString RecordData1BinDir = DataSaveRootDir + "/MotionRecordData/Bin/";
    // const QString RecordData1DecodeDir = DataSaveRootDir + "/MotionRecordData/Decode/";

    // util::DataQueueRecorder<move::MotionSharedData::RecordData1>::ptr motion_record_data1_recorder_;
    // const QString data1_header_file_ = RecordData1DecodeDir + "data1_header.txt";

    const QString DataSaveRootDir =
        QString::fromStdString(SystemSettings::instance().get_datasave_dir());
    const QString AudioRecordDir = DataSaveRootDir + "/AudioRecord/";
};


} // namespace app
} // namespace edm
