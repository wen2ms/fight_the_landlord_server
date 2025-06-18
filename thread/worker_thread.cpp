#include "worker_thread.h"

WorkerThread::WorkerThread(int index)
    : ev_loop_(nullptr), thread_(nullptr), thread_id_(std::thread::id()), name_("sub_thread_" + std::to_string(index)) {}

WorkerThread::~WorkerThread() {
    if (thread_ != nullptr) {
        delete thread_;
    }
}

void WorkerThread::run() {
    thread_ = new std::thread(&WorkerThread::sub_thread_running, this);
    std::unique_lock<std::mutex> locker(mutex_);
    while (ev_loop_ != nullptr) {
        cond_.wait(locker);
    }
}

void WorkerThread::sub_thread_running() {
    mutex_.lock();
    ev_loop_ = new EventLoop(name_);
    cond_.notify_one();
    mutex_.unlock();

    ev_loop_->run();
}