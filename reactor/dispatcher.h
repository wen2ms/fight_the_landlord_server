#pragma once

#include <iostream>

#include "channel.h"
#include "event_loop.h"

class EventLoop;
class Dispatcher {
  public:
    Dispatcher(EventLoop* ev_loop);
    virtual ~Dispatcher();

    virtual int add();
    virtual int remove();
    virtual int modify();
    virtual int dispatch(int timeout = 2);

    void set_channel(Channel* channel) {
        channel_ = channel;
    }

  protected:
    std::string name_;
    Channel* channel_;
    EventLoop* ev_loop_;
};