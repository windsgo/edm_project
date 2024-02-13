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
    CanWorker(const QString& can_if_name);
    ~CanWorker() {}

    bool is_connected() const { return connected_; };

public slots:
    void slot_start_work();

private slots:
    void _slot_reconnect();

private:
    QString can_if_name_;
    std::string can_if_name_std_;
    QCanBusDevice* device_ = nullptr;

    QTimer* reconnect_timer_ = nullptr;
    static constexpr const int reconnect_timeout_ = 1000; // ms

    bool connected_ = false;
};

class CanController : public QObject {
    Q_OBJECT
public:
    CanController(const QString& can_if_name);
    ~CanController();

    // not fully safe method ... // TODO
    bool is_connected() const { return can_worker_->is_connected(); };

signals:
    void _sig_worker_start_work();

private:
    QThread* worker_thread_;
    CanWorker* can_worker_;
};
    
} // namespace can

} // namespace edm
