#pragma once

#include "event_loop.h"
#include "buffer.h"
#include "channel.h"
#include "http_request.h"
#include "http_response.h"

#define MSG_SEND_AUTO

class TcpConnection {
  public:
    TcpConnection(int fd, EventLoop* ev_loop);
    ~TcpConnection();

    static int process_read(void* arg);
    static int process_write(void* arg);
    static int destroy(void* arg);

  private:
    EventLoop* ev_loop_;
    Channel* channel_;
    Buffer* read_buf_;
    Buffer* write_buf_;
    std::string name_;
    HttpRequest* request_;
    HttpResponse* response_;
};