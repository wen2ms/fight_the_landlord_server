#pragma once

#include <functional>

// using handle_func = int (*)(void*);

enum class FDEvent {
    kTimeout = 0x01,
    kReadEvent = 0x02,
    kWriteEvent = 0x04
};

class Channel {
  public:
    using handle_func = std::function<int(void*)>;
    Channel(int fd, FDEvent events, handle_func read_func, handle_func write_func, handle_func destroy_func, void* arg);

    void write_event_enable(bool flag);
    bool is_write_event_enable();

    inline int get_event() {
        return events_;
    }

    inline int get_socket() {
        return fd_;
    }

    inline const void* get_arg() {
        return arg_;
    }

    handle_func read_callback_;
    handle_func write_callback_;
    handle_func destroy_callback_;

  private:
    int fd_;
    int events_;
    void* arg_;
};