#include "channel.h"

Channel::Channel(int fd, FDEvent events, handle_func read_func, handle_func write_func, handle_func destroy_func, void* arg)
    : fd_(fd),
      events_(static_cast<int>(events)),
      read_callback_(read_func),
      write_callback_(write_func),
      destroy_callback_(destroy_func),
      arg_(arg) {}

void Channel::write_event_enable(bool flag) {
    if (flag) {
        events_ |= static_cast<int>(FDEvent::kWriteEvent);
    } else {
        events_ &= ~static_cast<int>(FDEvent::kReadEvent);
    }
}

bool Channel::is_write_event_enable() {
    return events_ & static_cast<int>(FDEvent::kWriteEvent);
}
