#pragma once

#include <poll.h>

#include "dispatcher.h"
#include "event_loop.h"

struct EventLoop;
class PollDispatcher : public Dispatcher {
  public:
    PollDispatcher(EventLoop* ev_loop);
    ~PollDispatcher();

    int add() override;
    int remove() override;
    int modify() override;
    int dispatch(int timeout = 2) override;

  private:
    int maxfd_;
    struct pollfd* fds_;
    const int kMaxNode = 1024;
};