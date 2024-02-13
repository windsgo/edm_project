#pragma once

#include <cstdint>
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
    CanWorker(const QString &can_if_name, uint32_t bitrate);
    ~CanWorker() {}

    bool is_connected();

private:
    void _start_work();

private slots:
    void _slot_reconnect();

protected:
    void customEvent(QEvent *event) override;

private:
    QString can_if_name_;
    std::string can_if_name_std_;
    uint32_t bitrate_;

    QCanBusDevice *device_ = nullptr;

    QTimer *reconnect_timer_ = nullptr;
    static constexpr const int reconnect_timeout_ = 1000; // ms

    bool connected_ = false;
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
    void send_frame(const QString& device_name, const QCanBusFrame &frame);

    CanController() ;
    ~CanController() ; // TODO, may stop thread

private:
    QThread *worker_thread_;
    CanWorker *can_worker_;

    QMap<QString, QPair<int, CanWorker *>> worker_map_; // used for management
    QVector<QPair<QString, CanWorker *>> worker_vec_;   // used for index
};

} // namespace can

} // namespace edm
