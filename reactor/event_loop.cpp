#include "event_loop.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "epoll_dispatcher.h"
#include "log.h"
#include "poll_dispatcher.h"
#include "select_dispatcher.h"

EventLoop::EventLoop() : EventLoop(std::string()) {}

EventLoop::EventLoop(const std::string thread_name)
    : is_quit_(true),
      thread_id_(std::this_thread::get_id()),
      thread_name_((thread_name == std::string() ? "main_thread" : thread_name)),
      dispatcher_(new EpollDispatcher(this)) {
    channel_map_.clear();
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair_);
    if (ret == -1) {
        perror("socketpair");
        exit(0);
    }

    auto obj = std::bind(&EventLoop::read_message, this);
    Channel* channel = new Channel(socket_pair_[1], FDEvent::kReadEvent, obj, nullptr, nullptr, this);
    add_task(channel, ElemType::kAdd);
}

EventLoop::~EventLoop() {}

int EventLoop::run() {
    is_quit_ = false;
    if (thread_id_ != std::this_thread::get_id()) {
        return -1;
    }

    while (!is_quit_) {
        dispatcher_->dispatch();
        process_task_queue();
    }

    return 0;
}

int EventLoop::event_activate(int fd, int event) {
    if (fd < 0) {
        return -1;
    }

    Channel* channel = channel_map_[fd];
    assert(channel->get_socket() == fd);
    if (event & static_cast<int>(FDEvent::kReadEvent) && channel->read_callback_) {
        channel->read_callback_(const_cast<void*>(channel->get_arg()));
    }
    if (event & static_cast<int>(FDEvent::kWriteEvent) && channel->write_callback_) {
        channel->write_callback_(const_cast<void*>(channel->get_arg()));
    }

    return 0;
}

int EventLoop::add_task(Channel* channel, ElemType type) {
    mutex_.lock();

    auto* node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    task_queue_.push(node);

    mutex_.unlock();
    if (thread_id_ == std::this_thread::get_id()) {
        process_task_queue();
    } else {
        task_wakeup();
    }

    return 0;
}

int EventLoop::process_task_queue() {
    while (!task_queue_.empty()) {
        mutex_.lock();
        ChannelElement* node = task_queue_.front();
        task_queue_.pop();
        mutex_.unlock();

        Channel* channel = node->channel;
        if (node->type == ElemType::kAdd) {
            add(channel);
        } else if (node->type == ElemType::kDelete) {
            remove(channel);
        } else if (node->type == ElemType::kModify) {
            modify(channel);
        }
        delete node;
    }
    return 0;
}

int EventLoop::add(Channel* channel) {
    int fd = channel->get_socket();
    if (channel_map_.find(fd) == channel_map_.end()) {
        channel_map_.insert(std::make_pair(fd, channel));

        dispatcher_->set_channel(channel);
        int ret = dispatcher_->add();
        return ret;
    }

    return -1;
}

int EventLoop::remove(Channel* channel) {
    int fd = channel->get_socket();
    if (channel_map_.find(fd) == channel_map_.end()) {
        return -1;
    }

    dispatcher_->set_channel(channel);
    int ret = dispatcher_->remove();
    return ret;
}

int EventLoop::modify(Channel* channel) {
    int fd = channel->get_socket();
    if (channel_map_.find(fd) == channel_map_.end()) {
        return -1;
    }

    dispatcher_->set_channel(channel);
    int ret = dispatcher_->modify();
    return ret;
}

int EventLoop::free_channel(Channel* channel) {
    auto it = channel_map_.find(channel->get_socket());
    if (it != channel_map_.end()) {
        channel_map_.erase(it);
        close(channel->get_socket());
        delete channel;
    }

    return 0;
}

int EventLoop::read_message() {
    char buf[256];
    read(socket_pair_[1], buf, sizeof(buf));
    return 0;
}

void EventLoop::task_wakeup() {
    const char* msg = "Wake Up!!!";
    write(socket_pair_[0], msg, strlen(msg));
}
