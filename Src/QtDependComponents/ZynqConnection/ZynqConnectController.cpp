#include "ZynqConnectController.h"
#include <cstring>
#include <mutex>
#include <qhostaddress.h>
#include <qobject.h>
#include <qtcpsocket.h>
#include <qthread.h>
#include <qtimer.h>
#include <qudpsocket.h>

#include <QNetworkDatagram>

#include "Exception/exception.h"
#include "Logger/LogMacro.h"
#include "Utils/Format/edm_format.h"

EDM_STATIC_LOGGER(s_logger, EDM_LOGGER_ROOT());

namespace edm {
namespace zynq {

ZynqUdpWorker::ZynqUdpWorker(uint16_t local_port) : local_port_(local_port) {}

void ZynqUdpWorker::add_listener(
    const std::function<void(const QByteArray &)> listener_cb) {
    std::lock_guard lg(listener_vec_mutex_);

    if (listener_cb)
        listener_vec_.push_back(listener_cb);
}

void ZynqUdpWorker::slot_start_work() {
    if (udp_socket_) {
        s_logger->warn("do not call ZynqUdpWorker::slot_start_work again");
        return;
    }

    udp_socket_ = new QUdpSocket(this);
    connect(udp_socket_, &QUdpSocket::readyRead, this,
            &ZynqUdpWorker::_slot_data_received);
    udp_socket_->bind(QHostAddress::AnyIPv4, local_port_);
}

void ZynqUdpWorker::_slot_data_received() {
    while (udp_socket_->hasPendingDatagrams()) {
        QNetworkDatagram dg = udp_socket_->receiveDatagram();

        std::lock_guard lg{listener_vec_mutex_};
        for (const auto &listener_cb : listener_vec_) {
            listener_cb(dg.data());
        }
    }
}

ZynqTcpWorker::ZynqTcpWorker(const QHostAddress &server_ip,
                             uint16_t server_port)
    : server_ip_(server_ip), server_port_(server_port) {}

void ZynqTcpWorker::add_listener(
    const std::function<void(const QByteArray &)> listener_cb) {
    std::lock_guard lg(listener_vec_mutex_);

    if (listener_cb)
        listener_vec_.push_back(listener_cb);
}

void ZynqTcpWorker::slot_start_work() {
    if (tcp_socket_) {
        s_logger->warn("do not call ZynqTcpWorker::slot_start_work again");
        return;
    }

    tcp_socket_ = new QTcpSocket(this);
    connect(tcp_socket_, &QTcpSocket::readyRead, this,
            &ZynqTcpWorker::_slot_data_received);
    connect(tcp_socket_, &QTcpSocket::connected, this,
            &ZynqTcpWorker::sig_tcp_connected);
    connect(tcp_socket_, &QTcpSocket::stateChanged, this, &ZynqTcpWorker::sig_tcp_socket_state_changed);

    // debug log
    connect(tcp_socket_, &QTcpSocket::connected, this, [this]() {
        s_logger->info("zynq tcp server connected");

        // const char *testmsg = "hi server";
        // QByteArray ba = QByteArray::fromRawData(testmsg, strlen(testmsg));
        // tcp_socket_->write(ba);
    });
    connect(tcp_socket_, &QTcpSocket::disconnected, this,
            [this]() { s_logger->warn("zynq tcp server disconnected"); });

    tcp_socket_->connectToHost(server_ip_, server_port_);

    reconnect_timer_ = new QTimer(this);
    connect(reconnect_timer_, &QTimer::timeout, this,
            &ZynqTcpWorker::_slot_do_reconnect);
    reconnect_timer_->start(reconnect_timeout_ms_);
}

void ZynqTcpWorker::slot_send(const QByteArray &ba) {
    if (!tcp_socket_) {
        s_logger->error("call {} when tcp_socket_ is not initialized!",
                        __PRETTY_FUNCTION__);
        return;
    }

    if (tcp_socket_->state() != QTcpSocket::SocketState::ConnectedState) {
        return;
    }

    tcp_socket_->write(ba);

    // bool ret = tcp_socket_->waitForBytesWritten(200);
    // if (!ret) {
    //     tcp_socket_->abort();
    //     s_logger->warn("ZynqTcpWorker: tcp write timeout, abort");
    //     return;
    // }

    // ret = tcp_socket_->waitForReadyRead(200);
    // if (!ret) {
    //     tcp_socket_->abort();
    //     s_logger->warn("ZynqTcpWorker: tcp read timeout, abort");
    //     return;
    // }
}

void ZynqTcpWorker::_slot_data_received() {
    while (tcp_socket_->bytesAvailable()) {
        // FIXME
        QByteArray ba = tcp_socket_->readAll(); // simple way, may have bug

        emit sig_tcp_msg_received(ba);

        {
            std::lock_guard lg{listener_vec_mutex_};
            for (const auto &listener_cb : listener_vec_) {
                listener_cb(ba);
            }
        }
    }
}

void ZynqTcpWorker::_slot_do_reconnect() {
    if (tcp_socket_->state() == QTcpSocket::SocketState::UnconnectedState) {
        tcp_socket_->connectToHost(server_ip_, server_port_);

        s_logger->trace("zynq tcp server reconnecting ...");
    }
}

ZynqConnectController::ZynqConnectController(
    const QHostAddress &zynq_tcpserver_ip, uint16_t zynq_tcpserver_port,
    uint16_t udp_port)
    : zynq_tcpserver_ip_(zynq_tcpserver_ip),
      zynq_tcpserver_port_(zynq_tcpserver_port), udp_port_(udp_port) {

    if (zynq_tcpserver_ip_.isNull()) {
        throw exception{
            EDM_FMT::format("invalid tcpserver ip: {}",
                            zynq_tcpserver_ip_.toString().toStdString())};
    }

    _init_worker_and_thread();
}

ZynqConnectController::~ZynqConnectController() {
    worker_thread_->quit();
    worker_thread_->wait();
}

void ZynqConnectController::add_udp_listener(
    const std::function<void(const QByteArray &)> listener_cb) {
    udp_worker_->add_listener(listener_cb);
}

void ZynqConnectController::add_tcp_listener(
    const std::function<void(const QByteArray &)> listener_cb) {
    tcp_worker_->add_listener(listener_cb);
}

void ZynqConnectController::send_tcp_bytearray(const QByteArray &ba) {
    emit _sig_tcp_write(ba);
}

void ZynqConnectController::_init_worker_and_thread() {
    worker_thread_ = new QThread(this);

    tcp_worker_ = new ZynqTcpWorker(zynq_tcpserver_ip_, zynq_tcpserver_port_);
    tcp_worker_->moveToThread(worker_thread_);
    connect(worker_thread_, &QThread::finished, tcp_worker_,
            &QObject::deleteLater);
    connect(this, &ZynqConnectController::_sig_workers_start, tcp_worker_,
            &ZynqTcpWorker::slot_start_work);
    connect(this, &ZynqConnectController::_sig_tcp_write, tcp_worker_,
            &ZynqTcpWorker::slot_send);
    connect(tcp_worker_, &ZynqTcpWorker::sig_tcp_connected, this,
            &ZynqConnectController::sig_zynq_tcp_connected);
    connect(tcp_worker_, &ZynqTcpWorker::sig_tcp_socket_state_changed, this,
            &ZynqConnectController::sig_zynq_tcp_socket_state_changed);

    udp_worker_ = new ZynqUdpWorker(udp_port_);
    udp_worker_->moveToThread(worker_thread_);
    connect(worker_thread_, &QThread::finished, tcp_worker_,
            &QObject::deleteLater);
    connect(this, &ZynqConnectController::_sig_workers_start, udp_worker_,
            &ZynqUdpWorker::slot_start_work);

    // start thread
    worker_thread_->start();

    // start workers
    emit _sig_workers_start();
}

QByteArray ZynqConnectController::MakeTcpPackage(uint8_t frame_id,
                                                 const void *data_ptr,
                                                 std::size_t data_size) {
    if (data_size > 250) {
        s_logger->warn("in {}: data_size ({}) > {}", __FUNCTION__, data_size,
                       250);
        data_size = 250;
    }

    if (data_ptr == nullptr && data_size != 0) {
        s_logger->warn("in {}: data_size ({}) != 0, while data_ptr is nullptr",
                       __FUNCTION__, data_size);
        data_size = 0;
    }

    uint8_t buffer[256];
    buffer[0] = 0x7E;
    buffer[1] = 6 + data_size; // total length
    buffer[2] = frame_id;
    buffer[3] = 0x00;

    if (data_ptr != nullptr) {
        memcpy(&buffer[4], data_ptr, data_size);
    }

    uint16_t crc = util::tcp_crc_table_calc(&buffer[1], buffer[1] - 3);
    buffer[buffer[1] - 2] = crc >> 8;
    buffer[buffer[1] - 1] = crc & 0xFF;

    QByteArray ba{(const char *)buffer, buffer[1]};

    return ba;
}

} // namespace zynq
} // namespace edm