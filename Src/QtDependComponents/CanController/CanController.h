#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include <QCanBus>
#include <QThread>
#include <QCanBusDevice>
#include <QTimer>

namespace edm
{

namespace can
{

class CanWorker : public QObject {
    Q_OBJECT
public:
    CanWorker(const QString& can_if_name, uint32_t bitrate);
    ~CanWorker() {}

    bool is_connected() const { return connected_; };

public slots:
    // can only be called once !
    void slot_start_work();

    void slot_send_frame(const QCanBusFrame& frame);

private slots:
    void _slot_reconnect();

private:
    QString can_if_name_;
    std::string can_if_name_std_;
    uint32_t bitrate_;

    QCanBusDevice* device_ = nullptr;

    QTimer* reconnect_timer_ = nullptr;
    static constexpr const int reconnect_timeout_ = 1000; // ms

    bool connected_ = false;
};

// support one single can device in one single thread at present
// TODO is to support multi device in one thread
class CanController : public QObject {
    Q_OBJECT
public:
    CanController(const QString& can_if_name, uint32_t bitrate);
    ~CanController();

    // not fully safe method ... // TODO
    bool is_connected() const { return can_worker_->is_connected(); };

    void send_frame(const QCanBusFrame& frame);

signals:
    void _sig_worker_start_work();
    void _sig_worker_send_frame(const QCanBusFrame& frame);

private:
    QThread* worker_thread_;
    CanWorker* can_worker_;
};
    
} // namespace can

} // namespace edm
