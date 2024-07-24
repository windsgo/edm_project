#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QThread>
#include <QTimer>

#include <cstdint>
#include <memory>
#include <mutex>
#include <qhostaddress.h>
#include <qtcpsocket.h>
#include <qthread.h>
#include <qtimer.h>
#include <qudpsocket.h>

// ZynqConnectController Controlls One Thread,
// A TcpWorker and a UdpWorker work in this thread

namespace edm {
namespace zynq {

// use listener in udp recv, faster
class ZynqUdpWorker : public QObject {
    Q_OBJECT
    friend class ZynqConnectController;
public:
    ZynqUdpWorker(uint16_t local_port);
    void add_listener(const std::function<void(const QByteArray &)> listener_cb);

private: // friend ZynqConnectController call
    void slot_start_work(); // start local server.

private:
    void _slot_data_received();

private:
    uint16_t local_port_ {0};
    QUdpSocket* udp_socket_ {nullptr};

    std::mutex listener_vec_mutex_;
    std::vector<std::function<void(const QByteArray &)>> listener_vec_;
};

// tcp receive support listener and signal
class ZynqTcpWorker : public QObject {
    Q_OBJECT
    friend class ZynqConnectController;
public:
    ZynqTcpWorker(const QHostAddress& server_ip, uint16_t server_port);
    void add_listener(const std::function<void(const QByteArray &)> listener_cb);

signals:
    void sig_tcp_msg_received(QByteArray);
    // TODO tcp connected/disconnected signal

private: // friend ZynqConnectController call
    void slot_start_work(); // start reconnect timer
    void slot_send(const QByteArray& ba); // send tcp message

private:
    void _slot_data_received();
    void _slot_do_reconnect();

private:
    QHostAddress server_ip_;
    uint16_t server_port_ {0};
    QTcpSocket* tcp_socket_ {nullptr};

    QTimer *reconnect_timer_ {nullptr};
    static constexpr const int reconnect_timeout_ms_ {1000};

    std::mutex listener_vec_mutex_;
    std::vector<std::function<void(const QByteArray &)>> listener_vec_;
};

class ZynqConnectController : public QObject {
    Q_OBJECT
public:
    using ptr = std::shared_ptr<ZynqConnectController>;

public:
    ZynqConnectController(const QHostAddress& zynq_tcpserver_ip, uint16_t zynq_tcpserver_port, uint16_t udp_port);
    ~ZynqConnectController();

    void add_udp_listener(const std::function<void(const QByteArray &)> listener_cb);
    void add_tcp_listener(const std::function<void(const QByteArray &)> listener_cb);

    void send_tcp_bytearray(const QByteArray& ba);

private:
    void _init_worker_and_thread();

private:
    QThread* worker_thread_;

    ZynqTcpWorker* tcp_worker_ {nullptr};
    ZynqUdpWorker* udp_worker_ {nullptr};

signals:
    void _sig_workers_start();
    void _sig_tcp_write(const QByteArray& ba);

private:
    QHostAddress zynq_tcpserver_ip_;
    uint16_t zynq_tcpserver_port_ {0};
    uint16_t udp_port_ {0};
};

} // namespace zynq
} // namespace edm