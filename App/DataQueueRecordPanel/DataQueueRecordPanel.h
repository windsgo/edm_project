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

private:
    void _init_dirs();

    // data1
    void _init_record_data1();
    std::string _generate_data1_header() const;
    bool _save_data1_header_to_file(const std::string& filename) const;

private:
    Ui::DataQueueRecordPanel *ui;

    SharedCoreData* shared_core_data_;

    const QString DataSaveRootDir = QString::fromStdString(SystemSettings::instance().get_datasave_dir());
    const QString RecordData1BinDir = DataSaveRootDir + "/MotionRecordData/Bin/";
    const QString RecordData1DecodeDir = DataSaveRootDir + "/MotionRecordData/Decode/";

    util::DataQueueRecorder<move::MotionSharedData::RecordData1>::ptr motion_record_data1_recorder_;
    const QString data1_header_file_ = RecordData1DecodeDir + "data1_header.txt";
};


} // namespace app
} // namespace edm
