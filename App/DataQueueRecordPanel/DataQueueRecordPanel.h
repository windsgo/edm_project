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
    void _init_record_data1();

private:
    Ui::DataQueueRecordPanel *ui;

    SharedCoreData* shared_core_data_;

    const QString DataSaveRootDir = QString::fromStdString(SystemSettings::instance().get_datasave_dir());
    const QString RecordData1BinDir = DataSaveRootDir + "/MotionRecordData/Bin/";
    const QString RecordData1DecodeDir = DataSaveRootDir + "/MotionRecordData/Decode/";

    util::DataQueueRecorder<move::MotionSharedData::RecordData1>::ptr motion_record_data1_recorder_;
};


} // namespace app
} // namespace edm
