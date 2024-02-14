#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include <QCanBus>
#include <QCanBusDevice>
#include <QEvent>
#include <QMap>
#include <QThread>
#include <QTimer>

namespace edm {

namespace can {

class CanSendFrameEvent : public QEvent {
public:
    CanSendFrameEvent(const QCanBusFrame &frame)
        : QEvent(type), frame_(frame) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(QEvent::Type::User + 1);

    const QCanBusFrame &get_frame() const { return frame_; }

private:
    QCanBusFrame frame_;
};

class CanStartEvent : public QEvent {
public:
    CanStartEvent() : QEvent(type) {}

    constexpr static const QEvent::Type type =
        QEvent::Type(QEvent::Type::User + 2);
};

class CanWorker : public QObject {
    Q_OBJECT
public:
    // Events can be accepted:
    // 1.  CanStartEvent: to start the device
    // 2.  CanSendFrameEvent: send a frame to device's canbus

    CanWorker(const QString &can_if_name, uint32_t bitrate);
    ~CanWorker();

    bool is_connected() const; // have lock guard

    void add_listener(std::function<void(const QCanBusFrame &)> listener_cb);

    // ! Not Safe Enough. Only call it when stop all things
    // ! This is only used for set device down, for test.
    void terminate();

// signals:
    // void sig_frame_received(const QCanBusFrame &frame); // received frame signal

private:
    void _start_work();

    void _process_start_event(QEvent *event);
    void _process_sendframe_event(QEvent *event);

    void _set_can_down();
    void _set_can_up_with_bitrate();

private slots:
    void _slot_reconnect();
    void _slot_framesReceived();

protected:
    void customEvent(QEvent *event) override;

private:
    QString can_if_name_;
    std::string can_if_name_std_;
    uint32_t bitrate_;

    std::string can_down_shell_cmd_;
    std::string can_up_shell_cmd_;

    QCanBusDevice *device_ = nullptr;

    QTimer *reconnect_timer_ = nullptr;
    static constexpr const int reconnect_timeout_ = 1000; // ms

    bool connected_ = false;

    std::vector<std::function<void(const QCanBusFrame &)>> listener_vec_;

    mutable std::mutex mutex_connected_;
    // mutex protect:
    // 1. connected_:
    //      outside read (is_connected() function call)
    //      inside write (inside read no need to protect i think)

    mutable std::mutex mutex_listener_vec_;
    // mutex protect:
    // 1. listener_vec_:
    //      outside write (add_listener)
    //      inside read (_slot_framesReceived)
};

// support multi devices in one thread
class CanController : public QObject {
    Q_OBJECT
public:
    static CanController *instance() {
        static CanController instance;
        return &instance;
    }

public:
    // add a device using its interface name,
    // if success, return the device index of the device
    // else, return -1
    int add_device(const QString &name, uint32_t bitrate);

    void send_frame(int index, const QCanBusFrame &frame);
    void send_frame(const QString &device_name, const QCanBusFrame &frame);

    bool is_connected(int index) const;
    bool is_connected(const QString &device_name) const;

    void add_frame_received_listener(
        int index, std::function<void(const QCanBusFrame &)> listener_cb);
    void add_frame_received_listener(
        const QString &device_name,
        std::function<void(const QCanBusFrame &)> listener_cb);

    CanController();
    ~CanController(); // TODO, may stop thread

private:
    CanWorker *_get_device(int index) const;
    CanWorker *_get_device(const QString &name) const;

private:
    QThread *worker_thread_;

    QMap<QString, QPair<int, CanWorker *>> worker_map_; // used for management
    QVector<QPair<QString, CanWorker *>> worker_vec_;   // used for index

    mutable std::mutex mutex_worker_map_and_vec_;
    // mutex protect:
    // 1. worker_map_, worker_vec_:
    //      add_device call (write)
    //      _get_device inside call (read), we assure device get by
    //      `_get_device`
    //          will not disappear or be deleted, so no need to protect it here
};

} // namespace can

} // namespace edm
