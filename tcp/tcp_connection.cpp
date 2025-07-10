#include "tcp_connection.h"

#include <stdio.h>

#include <fstream>
#include <sstream>

#include "codec.h"
#include "rsacrypto.h"
#include "log.h"

TcpConnection::TcpConnection(int fd, EventLoop* ev_loop)
    : ev_loop_(ev_loop),
      channel_(new Channel(fd, FDEvent::kWriteEvent, process_read, process_write, destroy, this)),
      reply_(new Communication),
      read_buf_(new Buffer(10240)),
      write_buf_(new Buffer(10240)),
      name_("connection_" + std::to_string(fd)) {
    prepare_secret_key();

    auto send_func = std::bind(&TcpConnection::add_write_task, this, std::placeholders::_1);
    auto delete_func = std::bind(&TcpConnection::add_delete_task, this);

    reply_->set_callbacks(send_func, delete_func);

    ev_loop_->add_task(channel_, ElemType::kAdd);
}

TcpConnection::~TcpConnection() {
    if (read_buf_ && read_buf_->readable_size() == 0 && write_buf_ && write_buf_->readable_size() == 0) {
        ev_loop_->free_channel(channel_);

        delete read_buf_;
        delete write_buf_;
    }

    DEBUG("Disconnect and release resources, conn_name: %s", name_.data());
}

void TcpConnection::add_write_task(const std::string& data) const {
    write_buf_->append_package(data);

    // channel_->write_event_enable(true);
    // channel_->read_event_enable(false);
    // ev_loop_->add_task(channel_, ElemType::kModify);

    write_buf_->send_data(channel_->get_socket());
}

void TcpConnection::add_delete_task() {
    ev_loop_->add_task(channel_, ElemType::kDelete);

    DEBUG("Disconnect to client, conn_name = %s", name_.data());
}

int TcpConnection::process_read(void* arg) {
    TcpConnection* conn = static_cast<TcpConnection*>(arg);

    int socket = conn->channel_->get_socket();
    int count = conn->read_buf_->socket_read(socket);

    DEBUG("Received http data: %s", conn->read_buf_->data());

    if (count > 0) {
        conn->reply_->parse_request(conn->read_buf_);
    } else {
        conn->add_delete_task();
    }

    return 0;
}

int TcpConnection::process_write(void* arg) {
    DEBUG("Start sending...");
    TcpConnection* conn = static_cast<TcpConnection*>(arg);

    int count = conn->write_buf_->send_data(conn->channel_->get_socket());
    if (count > 0) {
        if (conn->write_buf_->readable_size() == 0) {
            conn->channel_->write_event_enable(false);
            conn->channel_->read_event_enable(true);

            conn->ev_loop_->add_task(conn->channel_, ElemType::kModify);

            DEBUG("Send over...");
        }
    }

    return 0;
}

int TcpConnection::destroy(void* arg) {
    TcpConnection* conn = static_cast<TcpConnection*>(arg);
    if (conn != nullptr) {
        delete conn;
    }

    return 0;
}
void TcpConnection::prepare_secret_key() {
    std::ifstream infile("public.pem");
    std::stringstream line_stream;

    line_stream << infile.rdbuf();

    std::string data = line_stream.str();
    Message msg;

    msg.rescode = RSA_DISTRIBUTION;
    msg.data1 = data;

    RsaCrypto rsa("private.pem", RsaCrypto::kPrivateKey);

    data = rsa.sign(data);

    msg.data2 = data;

    Codec codec(&msg);

    data = codec.encode_msg();

    write_buf_->append_package(data);
}
