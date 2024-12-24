#include "AudioRecorder.h"

#include "Logger/LogMacro.h"

#include <QDateTime>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace audio {

AudioRecorder::AudioRecorder(QObject *parent) : QObject(parent) {
    worker_thread_ = new QThread(this);
    worker_ = new AudioRecordWorker();
    worker_->moveToThread(worker_thread_);

    // To Worker
    connect(this, &AudioRecorder::_sig_start_record, worker_,
            &AudioRecordWorker::slot_start_record);
    connect(this, &AudioRecorder::_sig_stop_record, worker_,
            &AudioRecordWorker::slot_stop_record);

    // From Worker
    connect(worker_, &AudioRecordWorker::sig_record_started, this,
            [this](bool success) {
                emit sig_record_started(success);
            });
    connect(worker_, &AudioRecordWorker::sig_record_stopped, this,
            [this]() {
                emit sig_record_stopped();
            });
        
    connect(worker_thread_, &QThread::finished, worker_, &QObject::deleteLater);

    worker_thread_->start();
}

AudioRecorder::~AudioRecorder() {
    worker_thread_->quit();
    worker_thread_->wait();
}

void AudioRecorder::start_record(const AudioStartRecordParam &param) {
    emit _sig_start_record(param);
}

void AudioRecorder::stop_record() {
    emit _sig_stop_record();
}

} // namespace audio
} // namespace edm