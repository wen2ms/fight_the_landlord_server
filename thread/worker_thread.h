#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

#include "event_loop.h"

class WorkerThread {
  public:
    WorkerThread(int index);
    ~WorkerThread();

    void run();

    EventLoop* get_event_loop() {
        return ev_loop_;
    }

  private:
    void sub_thread_running();
  
    std::thread* thread_;
    std::thread::id thread_id_;
    std::string name_;
    std::mutex mutex_;
    std::condition_variable cond_;
    EventLoop* ev_loop_;
};