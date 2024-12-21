#pragma once

#include <cstdint>
#include <memory>

#include <QObject>
#include <QTimer>

#include "Motion/MotionSignalQueue/MotionSignalQueue.h"
#include "Motion/MotionThread/MotionThreadController.h"
#include "Motion/MoveDefines.h"

namespace edm {

class InfoDispatcher final : public QObject {
    Q_OBJECT
public:
    InfoDispatcher(move::MotionSignalQueue::ptr signal_queue,
                   move::MotionThreadController::ptr motion_controller,
                   QObject *parent = nullptr, int info_get_time_peroid_ms = 10);

    ~InfoDispatcher() = default;

    // 目前只需要返回常引用就好了, 因为都是主线程的操作, 不需要整个拷贝出去
    const auto &get_info() const { return info_cache_; }

private:
    void _info_get_timer_slot();

    void _handle_signal_callback(const move::MotionSignal &signal);

signals: // info 获取完成信号, 用于那些需要自动刷新的界面
    void info_updated(const edm::move::MotionInfo &info);

signals: // motion 信号
    void sig_manual_pointmove_started();
    void sig_manual_pointmove_stopped();

    void sig_auto_started();
    void sig_auto_paused();
    void sig_auto_resumed();
    void sig_auto_stopped();

    void sig_auto_notify();
    
    // TODO other signals

private:
    move::MotionSignalQueue::ptr signal_queue_;           /* consume signal */
    move::MotionThreadController::ptr motion_controller_; /* get info */

    //! 要注意如果改为多线程需要保护, 或使用信号槽
    move::MotionInfo info_cache_; /* main thread info cache */

    QTimer *info_get_timer_{nullptr};

    std::function<void(const move::MotionSignal &signal)> consumer_func_{
        std::bind_front(&InfoDispatcher::_handle_signal_callback, this)};
};

} // namespace edm

Q_DECLARE_METATYPE(edm::move::MotionInfo)
