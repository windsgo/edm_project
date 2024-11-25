#include "DataRecordInstance.h"

#include <QDateTime>
#include <QFileInfo>
#include <optional>

namespace edm {
namespace move {

template <typename DataStruct>
std::optional<QString> DataRecordInstanceBase<DataStruct>::start_record() {
    auto bin_file =
        bin_dir_ + name_ + "_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_hh_mm_ss_zzz") + ".bin";

    // start record
    auto ret = record_data_queuerecorder_->start_record(bin_file.toStdString());

    if (ret) {
        return bin_file;
    } else {
        return std::nullopt;
    }
}

template <typename DataStruct>
void DataRecordInstanceBase<DataStruct>::stop_record(bool wait_for_stopped) {
    record_data_queuerecorder_->stop_record(wait_for_stopped);
}

template <typename DataStruct>
std::optional<QString>
DataRecordInstanceBase<DataStruct>::decode_one_file(const QString &bin_filename,
                                                    bool include_header) const {
    QFileInfo fi(bin_filename);
    auto decode_filename = decode_dir_ + fi.baseName() + ".txt";

    std::ifstream ifs(bin_filename.toStdString(), std::ios::binary);
    if (!ifs.is_open()) {
        logger_->error("decode file failed: {}, open bin failed",
                        bin_filename.toStdString());
        return std::nullopt;
    }

    std::ofstream ofs(decode_filename.toStdString());
    if (!ofs.is_open()) {
        ifs.close();
        logger_->error("decode file failed: {}, open txt failed",
                        bin_filename.toStdString());
        return std::nullopt;
    }

    ifs.seekg(0, std::ios::end);
    int size = ifs.tellg();

    int data_num = size / sizeof(DataStruct);
    logger_->debug("size: {}, datas: {}", size, data_num);

    ifs.seekg(0, std::ios::beg);

    ofs.setf(std::ios::fixed, std::ios::floatfield);
    ofs.precision(4);

    if (include_header) {
        ofs << generate_data_header() << '\n';
    }

    for (int i = 0; i < data_num; ++i) {
        DataStruct data;
        ifs.read(reinterpret_cast<char *>(&data), sizeof(DataStruct));

        ofs << bin_to_string(data) << '\n';
    }

    ifs.close();
    ofs.close();

    logger_->info("Decode Success, saved to: {}",
                   decode_filename.toStdString());

    return decode_filename;
}

} // namespace move
} // namespace edm