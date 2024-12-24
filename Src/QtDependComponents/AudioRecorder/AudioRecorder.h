#pragma once

#include <QObject>
#include <QThread>
#include <optional>
#include <qobjectdefs.h>

#include "AudioRecordWorker.h"

namespace edm {
namespace audio {

// Thread Controller
class AudioRecorder : public QObject {
    Q_OBJECT
public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder();

    // void start_default_record();
    void start_record(const AudioStartRecordParam& param);
    void stop_record();

signals:
    void sig_record_started(bool success);
    void sig_record_stopped();

signals:
    void _sig_start_record(const AudioStartRecordParam& param);
    void _sig_stop_record();

private:
    QThread *worker_thread_{nullptr};

    AudioRecordWorker *worker_{nullptr};
};


} // namespace audio
} // namespace edm