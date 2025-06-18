#pragma once

#include <sys/epoll.h>

#include "dispatcher.h"
#include "event_loop.h"

struct EventLoop;
class EpollDispatcher : public Dispatcher {
  public:
    EpollDispatcher(EventLoop* ev_loop);
    ~EpollDispatcher();

    int add() override;
    int remove() override;
    int modify() override;
    int dispatch(int timeout = 2) override;

  private:
    int epoll_op(int op);

    int epfd_;
    struct epoll_event* events_;
    const int kMaxNode = 520;
};