#pragma once

#include <QObject>
#include <QThread>
#include <mutex>
#include <qaudiodeviceinfo.h>
#include <qaudioinput.h>
#include <qobjectdefs.h>

#include <QAudioInput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>

#include <QFile>
#include <QIODevice>

namespace edm {
namespace audio {

struct AudioStartRecordParam {
    QString file_path;
    QAudioDeviceInfo audio_device_info;
    QAudioFormat audio_format;
};

class AudioRecordWorker : public QObject {
    Q_OBJECT
public:
    explicit AudioRecordWorker(QObject *parent = nullptr);
    ~AudioRecordWorker();

public:
    bool is_recording() const { return is_recording_.load(); }

public slots:
    void slot_start_record(const AudioStartRecordParam& param);
    void slot_stop_record();

signals:
    void sig_record_started(bool success);
    void sig_record_stopped();

private:
    void _init();

    void _stop();

    void _slot_stopped();

private:
    std::atomic<bool> is_recording_{false};

    QAudioInput* audio_input_{nullptr};

    QFile* file_{nullptr}; // currently is a QFile
};


} // namespace audio
} // namespace edm