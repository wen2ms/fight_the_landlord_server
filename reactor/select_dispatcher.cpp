#include "select_dispatcher.h"

#include <stdio.h>
#include <stdlib.h>

SelectDispatcher::SelectDispatcher(EventLoop* ev_loop) : Dispatcher(ev_loop) {
    name_ = "select";
    FD_ZERO(&read_set_);
    FD_ZERO(&write_set_);
}

SelectDispatcher::~SelectDispatcher() {}

int SelectDispatcher::add() {
    if (channel_->get_socket() >= kMaxSize) {
        return -1;
    }
    set_fd_set();

    return 0;
}

int SelectDispatcher::remove() {
    clear_fd_set();
    channel_->destroy_callback_(const_cast<void*>(channel_->get_arg()));
    return 0;
}

int SelectDispatcher::modify() {
    if (channel_->get_event() & static_cast<int>(FDEvent::kReadEvent)) {
        FD_SET(channel_->get_socket(), &read_set_);
        FD_CLR(channel_->get_socket(), &write_set_);
    }
    if (channel_->get_event() & static_cast<int>(FDEvent::kWriteEvent)) {
        FD_SET(channel_->get_socket(), &write_set_);
        FD_CLR(channel_->get_socket(), &read_set_);
    }
    
    return 0;
}

int SelectDispatcher::dispatch(int timeout) {
    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = read_set_;
    fd_set wrtmp = write_set_;
    int count = select(kMaxSize, &rdtmp, &wrtmp, NULL, &val);
    if (count == -1) {
        perror("select");
        exit(0);
    }
    for (int i = 0; i < kMaxSize; ++i) {
        if (FD_ISSET(i, &rdtmp)) {
            ev_loop_->event_activate(i, static_cast<int>(FDEvent::kReadEvent));
        }
        if (FD_ISSET(i, &wrtmp)) {
            ev_loop_->event_activate(i, static_cast<int>(FDEvent::kWriteEvent));
        }
    }

    return 0;
}

void SelectDispatcher::set_fd_set() {
    if (channel_->get_event() & static_cast<int>(FDEvent::kReadEvent)) {
        FD_SET(channel_->get_socket(), &read_set_);
    }
    if (channel_->get_event() & static_cast<int>(FDEvent::kWriteEvent)) {
        FD_SET(channel_->get_socket(), &write_set_);
    }
}

void SelectDispatcher::clear_fd_set() {
    if (channel_->get_event() & static_cast<int>(FDEvent::kReadEvent)) {
        FD_CLR(channel_->get_socket(), &read_set_);
    }
    if (channel_->get_event() & static_cast<int>(FDEvent::kWriteEvent)) {
        FD_CLR(channel_->get_socket(), &write_set_);
    }
}
