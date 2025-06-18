#include "epoll_dispatcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

EpollDispatcher::EpollDispatcher(EventLoop* ev_loop) : Dispatcher(ev_loop) {
    name_ = "epoll";
    epfd_ = epoll_create(10);
    if (epfd_ == -1) {
        perror("epoll_create");
        exit(0);
    }
    events_ = new struct epoll_event[kMaxNode];
}

EpollDispatcher::~EpollDispatcher() {
    close(epfd_);
    delete[] events_;
}

int EpollDispatcher::add() {
    int ret = epoll_op(EPOLL_CTL_ADD);
    if (ret == -1) {
        perror("epoll_add");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove() {
    int ret = epoll_op(EPOLL_CTL_DEL);
    if (ret == -1) {
        perror("epoll_remove");
        exit(0);
    }
    channel_->destroy_callback_(const_cast<void*>(channel_->get_arg()));
    return ret;
}

int EpollDispatcher::modify() {
    int ret = epoll_op(EPOLL_CTL_MOD);
    if (ret == -1) {
        perror("epoll_modify");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout) {
    int count = epoll_wait(epfd_, events_, kMaxNode, timeout * 1000);
    for (int i = 0; i < count; ++i) {
        int events = events_[i].events;
        int fd = events_[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP) {
            // epoll_remove(channel, ev_loop);
            continue;
        }
        if (events & EPOLLIN) {
            ev_loop_->event_activate(fd, static_cast<int>(FDEvent::kReadEvent));
        }
        if (events & EPOLLOUT) {
            ev_loop_->event_activate(fd, static_cast<int>(FDEvent::kWriteEvent));
        }
    }

    return 0;
}

int EpollDispatcher::epoll_op(int op) {
    struct epoll_event ev;
    ev.data.fd = channel_->get_socket();

    int events = 0;
    if (channel_->get_event() & static_cast<int>(FDEvent::kReadEvent)) {
        events |= EPOLLIN;
    }
    if (channel_->get_event() & static_cast<int>(FDEvent::kWriteEvent)) {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(epfd_, op, channel_->get_socket(), &ev);
    return ret;
}
