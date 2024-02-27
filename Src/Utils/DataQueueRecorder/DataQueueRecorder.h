#pragma once

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include "Exception/exception.h"
#include "Utils/Format/edm_format.h"
#include "config.h"

namespace edm {

namespace util {

template <typename DataType, int CacheSize = 1000>
class DataQueueRecorder final {
public:
    DataQueueRecorder() {
#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
        data_cache_.reserve(CacheSize);
#endif // EDM_DATAQUEUERECORDER_ENABLE_CACHE
    }

    ~DataQueueRecorder() {
        // TODO 改为调用 stop, 有锁更安全(要注意死锁)
        stop_flag_ = true;
        cv_.notify_all();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    inline bool start_record(std::string_view filename) {
        if (running_flag_) {
            return false;
        }

        filename_ = filename;

        // init file ostream
        ofs.open(filename_, std::ios::binary);
        if (!ofs.is_open()) {
            return false;
        }

#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
        // clear cache
        data_cache_.clear();
#endif // EDM_DATAQUEUERECORDER_ENABLE_CACHE

        // clear queue
        while (!data_queue_.empty()) {
            data_queue_.pop();
        }

        // clear stop flag
        stop_flag_ = false;

        // start recording thread
        thread_ = std::thread(_ThreadEntry, this);

        return true;
    }

    inline void push_data(const DataType &data) {
        if (!running_flag_) {
            return;
        }

        std::lock_guard lg(mutex_);
        data_queue_.push(data);
        cv_.notify_all();
    }

    template <typename... _Args>
    inline decltype(auto) emplace(_Args &&...__args) {
        if (!running_flag_) {
            return;
        }

        std::lock_guard lg(mutex_);
        data_queue_.emplace(std::forward<_Args>(__args)...);
        cv_.notify_all();
    }

    inline void stop_record() {
        stop_flag_ = true;
        cv_.notify_all();
    }

    inline bool is_running() const { return running_flag_; }

private:
    static inline void _ThreadEntry(DataQueueRecorder *dqr) { dqr->_run(); }

#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
    inline void _flush_cache() {
        ofs.write((char *)(data_cache_.data()),
                  data_cache_.size() * sizeof(DataType));
        data_cache_.clear();
    }
#endif // EDM_DATAQUEUERECORDER_ENABLE_CACHE

    inline void _flush_and_close_file() {
        if (ofs.is_open()) {

#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
            if (!data_cache_.empty()) {
                _flush_cache();
            }
#endif // EDM_DATAQUEUERECORDER_ENABLE_CACHE

            ofs.flush();
            ofs.close();
        }
    }

    inline void _run() {
        running_flag_ = true;

        DataType fetched_data;
        while (!stop_flag_) {
            bool queue_flag = 0;
            {
                std::unique_lock ul(mutex_);
                cv_.wait(ul, [this]() -> bool {
                    return this->stop_flag_ || !this->data_queue_.empty();
                });

                if (!data_queue_.empty()) {
                    fetched_data = data_queue_.front();
                    data_queue_.pop();
                    queue_flag = true;
                }
            }

#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
            assert(data_cache_.size() < data_cache_.capacity());
#endif // EDM_DATAQUEUERECORDER_ENABLE_CACHE

            if (queue_flag) {
#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
                data_cache_.push_back(fetched_data);

                if (data_cache_.size() >= data_cache_.capacity()) {
                    _flush_cache();
                }
#else  // EDM_DATAQUEUERECORDER_ENABLE_CACHE
                ofs.write((char *)&fetched_data, sizeof(fetched_data));
#endif // EDM_DATAQUEUERECORDER_ENABLE_CACHE
            }

            if (stop_flag_) {
                break;
            }
        }

        _flush_and_close_file();

        running_flag_ = false;
    }

private:
    std::string filename_;
    std::ofstream ofs;
    std::queue<DataType> data_queue_;

#ifdef EDM_DATAQUEUERECORDER_ENABLE_CACHE
    std::vector<DataType>
        data_cache_; // 缓存从队列中写入的数据, 缓存区到达一定数目后一次性写入
#endif               // EDM_DATAQUEUERECORDER_ENABLE_CACHE

    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread thread_;
    std::atomic_bool stop_flag_{false};
    std::atomic_bool running_flag_{false};
};

} // namespace util

} // namespace edm
