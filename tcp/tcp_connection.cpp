#include "tcp_connection.h"

#include <stdio.h>
#include <stdlib.h>

#include "log.h"

TcpConnection::TcpConnection(int fd, EventLoop* ev_loop)
    : ev_loop_(ev_loop),
      read_buf_(new Buffer(10240)),
      write_buf_(new Buffer(10240)),
      request_(new HttpRequest),
      response_(new HttpResponse),
      name_("connection_" + std::to_string(fd)),
      channel_(new Channel(fd, FDEvent::kReadEvent, process_read, process_write, destroy, this)) {
    ev_loop_->add_task(channel_, ElemType::kAdd);
}

TcpConnection::~TcpConnection() {
    if (read_buf_ && read_buf_->readable_size() == 0 && write_buf_ &&
        write_buf_->readable_size() == 0) {
        ev_loop_->free_channel(channel_);
        
        delete read_buf_;
        delete write_buf_;
        delete request_;
        delete response_;
    }

    DEBUG("Disconnect and release resources, conn_name: %s", name_.data());
}

int TcpConnection::process_read(void* arg) {
    TcpConnection* conn = static_cast<TcpConnection*>(arg);

    int socket = conn->channel_->get_socket();
    int count = conn->read_buf_->socket_read(socket);

    DEBUG("Received http data: %s", conn->read_buf_->data());

    if (count > 0) {
#ifdef MSG_SEND_AUTO
        conn->channel_->write_event_enable(true);
        conn->ev_loop_->add_task(conn->channel_, ElemType::kModify);
#endif
        bool flag = conn->request_->parse_http_request(conn->read_buf_, conn->response_, conn->write_buf_, socket);
        if (!flag) {
            const char* err_msg = "HTTP/1.1 400 Bad Request\r\n\r\n";
            conn->write_buf_->append_string(err_msg);
        }
    } else {
#ifdef MSG_SEND_AUTO
        conn->ev_loop_->add_task(conn->channel_, ElemType::kDelete);
#endif
    }

#ifndef MSG_SEND_AUTO
    conn->ev_loop_->add_task(conn->channel_, ElemType::kDelete);
#endif

    return 0;
}

int TcpConnection::process_write(void* arg) {
    DEBUG("Start sending...");
    TcpConnection* conn = static_cast<TcpConnection*>(arg);

    int count = conn->write_buf_->send_data(conn->channel_->get_socket());
    if (count > 0) {
        if (conn->write_buf_->readable_size() == 0) {
            conn->channel_->write_event_enable(true);
            conn->ev_loop_->add_task(conn->channel_, ElemType::kModify);
            conn->ev_loop_->add_task(conn->channel_, ElemType::kDelete);
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
