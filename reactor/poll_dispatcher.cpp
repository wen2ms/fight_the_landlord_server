#include "poll_dispatcher.h"

#include <stdio.h>
#include <stdlib.h>

PollDispatcher::PollDispatcher(EventLoop* ev_loop) : Dispatcher(ev_loop), maxfd_(0) {
    name_ = "poll";
    fds_ = new struct pollfd[kMaxNode];
    for (int i = 0; i < kMaxNode; ++i) {
        fds_[i].fd = -1;
        fds_[i].events = 0;
        fds_[i].revents = 0;
    }
}

PollDispatcher::~PollDispatcher() {
    delete[] fds_;
}

int PollDispatcher::add() {
    int events = 0;
    if (channel_->get_event() & static_cast<int>(FDEvent::kReadEvent)) {
        events |= POLLIN;
    }
    if (channel_->get_event() & static_cast<int>(FDEvent::kWriteEvent)) {
        events |= POLLOUT;
    }

    int i = 0;
    for (; i < kMaxNode; ++i) {
        if (fds_[i].fd == -1) {
            fds_[i].events = events;
            fds_[i].fd = channel_->get_socket();
            maxfd_ = i > maxfd_ ? i : maxfd_;
            break;
        }
    }
    if (i >= kMaxNode) {
        return -1;
    }
    return 0;
}

int PollDispatcher::remove() {
    int i = 0;
    for (; i < kMaxNode; ++i) {
        if (fds_[i].fd == channel_->get_socket()) {
            fds_[i].events = 0;
            fds_[i].revents = 0;
            fds_[i].fd = -1;
            break;
        }
    }
    channel_->destroy_callback_(const_cast<void*>(channel_->get_arg()));
    if (i >= kMaxNode) {
        return -1;
    }
    return 0;
}

int PollDispatcher::modify() {
    int events = 0;
    if (channel_->get_event() & static_cast<int>(FDEvent::kReadEvent)) {
        events |= POLLIN;
    }
    if (channel_->get_event() & static_cast<int>(FDEvent::kWriteEvent)) {
        events |= POLLOUT;
    }

    int i = 0;
    for (; i < kMaxNode; ++i) {
        if (fds_[i].fd == channel_->get_socket()) {
            fds_[i].events = events;
            break;
        }
    }
    if (i >= kMaxNode) {
        return -1;
    }
    return 0;
}

int PollDispatcher::dispatch(int timeout) {
    int count = poll(fds_, maxfd_ + 1, timeout * 1000);
    if (count == -1) {
        perror("poll");
        exit(0);
    }
    for (int i = 0; i <= maxfd_; ++i) {
        if (fds_[i].fd == -1) {
            continue;
        }
        if (fds_[i].revents & POLLIN) {
            ev_loop_->event_activate(fds_[i].fd, static_cast<int>(FDEvent::kReadEvent));
        }
        if (fds_[i].revents & POLLOUT) {
            ev_loop_->event_activate(fds_[i].fd, static_cast<int>(FDEvent::kWriteEvent));
        }
    }

    return 0;
}