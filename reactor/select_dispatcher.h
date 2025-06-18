#pragma once

#include <sys/select.h>

#include "dispatcher.h"
#include "event_loop.h"

struct EventLoop;
class SelectDispatcher : public Dispatcher {
  public:
    SelectDispatcher(EventLoop* ev_loop);
    ~SelectDispatcher();

    int add() override;
    int remove() override;
    int modify() override;
    int dispatch(int timeout = 2) override;

  private:
    void set_fd_set();
    void clear_fd_set();

    fd_set read_set_;
    fd_set write_set_;
    const int kMaxSize = 1024;
};