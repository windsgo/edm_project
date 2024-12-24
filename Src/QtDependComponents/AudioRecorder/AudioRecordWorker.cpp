#include "AudioRecordWorker.h"

#include "Logger/LogMacro.h"
#include <qaudio.h>
#include <qaudioformat.h>
#include <qaudioinput.h>

#include <QDataStream>
#include <qglobal.h>

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace audio {

struct metatype_register__ {
    metatype_register__() {
        qRegisterMetaType<AudioStartRecordParam>("AudioStartRecordParam");
    }
};
static struct metatype_register__ mt_register__;

AudioRecordWorker::AudioRecordWorker(QObject *parent) : QObject(parent) {
    _init();
}

void AudioRecordWorker::_init() {
    // Nothing
    file_ = new QFile(this);
}

AudioRecordWorker::~AudioRecordWorker() {
    s_logger->trace("{}: begin", __PRETTY_FUNCTION__);
    file_->close();
    s_logger->trace("{}: end", __PRETTY_FUNCTION__);
}

static bool _convert_pcm_data_to_wav(const QString &wav_filename,
                                     const QAudioFormat &format,
                                     const char *data, int64_t data_size) {

    QFile wav_file(wav_filename);
    if (!wav_file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        s_logger->error("open wav file failed: {}", wav_filename.toStdString());
        return false;
    }

    QDataStream out(&wav_file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setVersion(QDataStream::Qt_5_14);

    // Write WAV file header
    const quint16 channelCount = format.channelCount();
    const quint32 headerSize = 44;
    const quint32 dataSize = data_size;
    const quint32 fileSize = data_size + headerSize - 8;
    const quint16 audioFormat =
        format.sampleSize() * channelCount / 8;
    const quint32 sampleRate = format.sampleRate();
    const quint32 byteRate = sampleRate * audioFormat * channelCount;
    const quint16 blockAlign = audioFormat * channelCount;
    const quint16 bitsPerSample =format.sampleSize();

    // RIFF Header (8 bytes)
    out.writeRawData("RIFF", 4);
    out << fileSize;

    // format: WAVE (4 bytes)
    out.writeRawData("WAVE", 4);

    // WAVE data
    // WAVE: fmt trunk (16 Bytes)
    out.writeRawData("fmt ", 4);
    out << quint32(16);

    // byte 1
    out << quint16(1);            // Audio format (PCM)
    out << quint16(channelCount); // Number of channels

    // byte 2
    out << quint32(sampleRate);

    // byte 3
    out << quint32(byteRate);

    // byte 4
    out << quint16(blockAlign);
    out << quint16(bitsPerSample);

    // WAVE: data trunk (8 bytes + data size)
    out.writeRawData("data", 4);
    out << dataSize;

    // Write the updated PCM data with WAV file header
    out.writeRawData(data, data_size);

    return true;
}

void AudioRecordWorker::slot_start_record(const AudioStartRecordParam &param) {
    s_logger->trace("{}: begin", __PRETTY_FUNCTION__);

    if (is_recording_) {
        s_logger->error("{}: Already recording, can't start again",
                        __PRETTY_FUNCTION__);
        emit sig_record_started(false);
        return;
    }

    if (param.file_path.isEmpty()) {
        s_logger->error("{}: file path is empty", __PRETTY_FUNCTION__);
        emit sig_record_started(false);
        return;
    }

    if (!param.audio_device_info.isFormatSupported(param.audio_format)) {
        s_logger->error("{}: audio format is not supported",
                        __PRETTY_FUNCTION__);
        emit sig_record_started(false);
        return;
    }

    if (file_->isOpen()) {
        s_logger->warn("{}: file is already open, close it first",
                       __PRETTY_FUNCTION__);
        file_->close();
    }

    file_->setFileName(param.file_path);
    if (!file_->open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        s_logger->error("{}: open file failed", __PRETTY_FUNCTION__);
        emit sig_record_started(false);
        return;
    }

    audio_input_ =
        new QAudioInput(param.audio_device_info, param.audio_format, this);

    if (!audio_input_) {
        s_logger->error("{}: create audio input failed", __PRETTY_FUNCTION__);
        emit sig_record_started(false);
        return;
    }

    connect(
        audio_input_, &QAudioInput::stateChanged, this,
        [this](QAudio::State state) {
            if (state == QAudio::State::ActiveState) {
                s_logger->info("AudioRecordWorker: record started");
                emit sig_record_started(true);

                is_recording_ = true;
            } else if (state == QAudio::State::StoppedState) {
                s_logger->info("AudioRecordWorker: record stopped");

                file_->close();

                audio_input_->deleteLater();

                is_recording_ = false;

                // open again
                file_->open(QIODevice::ReadOnly);

                // Seek back to the beginning of the file
                file_->seek(0);

                auto wav_filename = file_->fileName() + ".wav";
                bool ret = _convert_pcm_data_to_wav(
                    wav_filename, audio_input_->format(),
                    file_->readAll().constData(), file_->size());
                if (!ret) {
                    s_logger->error(
                        "convert pcm to wav failed: pcm_file: {}, wav_file: {}",
                        file_->fileName().toStdString(),
                        wav_filename.toStdString());
                } else {
                    s_logger->info("convert pcm to wav success: pcm_file: {}, "
                                   "wav_file: {}",
                                   file_->fileName().toStdString(),
                                   wav_filename.toStdString());
                }

                // Close the file
                file_->close();

                emit sig_record_stopped();
            }
        });

    audio_input_->start(file_);

    s_logger->trace("{}: end", __PRETTY_FUNCTION__);
}

void AudioRecordWorker::slot_stop_record() { _stop(); }

void AudioRecordWorker::_stop() {
    s_logger->trace("{}: begin", __PRETTY_FUNCTION__);

    if (!is_recording_) {
        s_logger->warn("{}: not recording", __PRETTY_FUNCTION__);
    }

    if (!audio_input_) {
        s_logger->warn("{}: audio input is nullptr", __PRETTY_FUNCTION__);
        is_recording_ = false;
    } else {
        if (audio_input_->state() == QAudio::StoppedState) {
            s_logger->warn("{}: audio input is already stopped",
                           __PRETTY_FUNCTION__);
            is_recording_ = false;
            audio_input_->deleteLater();
        } else {
            audio_input_->stop();
        }
    }

    s_logger->trace("{}: end", __PRETTY_FUNCTION__);
}

} // namespace audio
} // namespace edm
