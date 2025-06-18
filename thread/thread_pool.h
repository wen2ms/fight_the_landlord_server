#pragma once

#include <vector>

#include "event_loop.h"
#include "worker_thread.h"

class ThreadPool {
  public:
    ThreadPool(EventLoop* main_loop, int count);
    ~ThreadPool();

    void run();
    EventLoop* take_worker_event_loop();

  private:
    EventLoop* main_loop_;
    bool is_start_;
    int num_threads_;
    std::vector<WorkerThread*> worker_threads_;
    int index_;
};