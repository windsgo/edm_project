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

class CanController : public QObject {
    Q_OBJECT
public:
    CanController(const QString& can_if_name);
    ~CanController();

    // bool connect_device();
    // bool is_connected() const;

    // void disconnect();

private:
    QThread* worker_thread_;
};

class CanWorker : public QObject {
    Q_OBJECT
public:
    CanWorker(const QString& can_if_name);
    ~CanWorker() {}

    // bool is_connected() const;

public slots:
    void slot_start_work();

private:
    void _scan_devices();

private slots:
    void _slot_device_disconnected();
    void _slot_reconnect();

private:
    QString can_if_name_;
    QCanBusDevice* device_ = nullptr;

    QTimer* reconnect_timer_ = nullptr;
    static constexpr const int reconnect_timeout_ = 1000; // ms

    bool connected_ = false;
};
    
} // namespace can

} // namespace edm
