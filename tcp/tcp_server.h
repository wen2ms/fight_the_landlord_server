#pragma once

#include "event_loop.h"
#include "thread_pool.h"

class TcpServer {
  public:
    TcpServer(unsigned short port, int num_threads);

    void set_listen();
    void run();

    static int accept_connection(void* arg);

  private:    
    int num_threads_;
    EventLoop* main_loop_;
    ThreadPool* thread_pool_;
    int lfd_;
    unsigned short port_;
};