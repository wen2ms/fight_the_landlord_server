#pragma once

#include "event_loop.h"
#include "buffer.h"
#include "channel.h"
#include "communication.h"

#define MSG_SEND_AUTO

class TcpConnection {
  public:
    TcpConnection(int fd, EventLoop* ev_loop);
    ~TcpConnection();

    void add_write_task(const std::string& data) const;
    void add_delete_task();

    static int process_read(void* arg);
    static int process_write(void* arg);
    static int destroy(void* arg);

    void prepare_secret_key();

  private:
    EventLoop* ev_loop_;
    Channel* channel_;
    Buffer* read_buf_;
    Buffer* write_buf_;
    std::string name_;

    Communication* reply_;
};