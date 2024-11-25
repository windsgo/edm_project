#pragma once

#include "Logger/LogDefine.h"
#include "Utils/DataQueueRecorder/DataQueueRecorder.h"
#include "config.h"
#include <memory>
#include <optional>
#include <string>

#include <QDateTime>
#include <QFileInfo>

#include <QString>

#include "Logger/LogMacro.h"

namespace edm {
namespace move {

// DataStruct should have `clear()` method
template <typename DataStruct> class DataRecordInstanceBase {
public:
    using ptr = std::shared_ptr<DataRecordInstanceBase<DataStruct>>;

    DataRecordInstanceBase(const QString &name, const QString &bin_dir,
                           const QString &decode_dir)
        : name_(name), bin_dir_(bin_dir), decode_dir_(decode_dir) {
        record_data_queuerecorder_ =
            std::make_shared<util::DataQueueRecorder<DataStruct>>();
    }
    virtual ~DataRecordInstanceBase() = default;

public:
    inline const auto &name() const { return name_; }
    inline const auto &bin_dir() const { return bin_dir_; }
    inline const auto &decode_dir() const { return decode_dir_; }
    inline auto header_filename() const {
        return decode_dir_ + name_ + "_header.txt";
    }

    inline void clear_data_record() { record_data_cache_.clear(); }
    inline auto &get_record_data_ref() { return record_data_cache_; }

    // 判断记录器是否在运行(使能), 如果未使能可以不记录或push数据
    inline bool is_data_recorder_running() const {
        return record_data_queuerecorder_->is_running();
    }

    // 将这一周期缓存的所有记录数据丢给记录器队列(线程)
    inline void push_data_to_recorder() {
        record_data_queuerecorder_->push_data(record_data_cache_);
    }

public:
    // 获取记录器指针(上层获取, 记录器线程安全, 直接在上层开始、结束,
    // 运动线程只需要判断是否running即可)
    inline auto get_record_data_queuerecorder() const {
        return record_data_queuerecorder_;
    }

    // 创建header字符串
    virtual std::string generate_data_header() const = 0;

    // 开始记录, 成功的话, 返回文件名
    std::optional<QString> start_record();

    // 停止记录
    void stop_record(bool wait_for_stopped = true);

    // 解码文件
    std::optional<QString> decode_one_file(const QString &bin_filename,
                                           bool include_header = true) const;

    // 实现bin_to_string
    virtual std::string bin_to_string(const DataStruct &bin) const = 0;

protected:
    DataStruct record_data_cache_;
    util::DataQueueRecorder<DataStruct>::ptr record_data_queuerecorder_;

    QString name_;

    QString bin_dir_{};
    QString decode_dir_{};

    edm::log::logger_ptr logger_ = EDM_LOGGER_ROOT();
};

} // namespace move
} // namespace edm

#include "DataRecordInstance.inl"