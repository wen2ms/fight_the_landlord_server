#include "thread_pool.h"

#include <assert.h>
#include <stdlib.h>

ThreadPool::ThreadPool(EventLoop* main_loop, int count) : index_(0), is_start_(false), main_loop_(main_loop), num_threads_(count) {
    worker_threads_.clear();
}

ThreadPool::~ThreadPool() {
    for (auto item : worker_threads_) {
        delete item;
    }
}

void ThreadPool::run() {
    assert(!is_start_);
    if (main_loop_->get_thread_id() != std::this_thread::get_id()) {
        exit(0);
    }

    is_start_ = true;
    if (num_threads_ > 0) {
        for (int i = 0; i < num_threads_; ++i) {
            WorkerThread* sub_thread = new WorkerThread(i);
            sub_thread->run();
            
            worker_threads_.push_back(sub_thread);
        }
    }
}

EventLoop* ThreadPool::take_worker_event_loop() {
    assert(is_start_);
    if (main_loop_->get_thread_id() != std::this_thread::get_id()) {
        exit(0);
    }

    EventLoop* ev_loop = main_loop_;
    if (num_threads_ > 0) {
        ev_loop = worker_threads_[index_]->get_event_loop();
        index_ = ++index_ % num_threads_;
    }

    return ev_loop;
}
