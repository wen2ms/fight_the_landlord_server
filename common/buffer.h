#pragma once

#include <string>

class Buffer {
  public:
    explicit Buffer(int size);
    ~Buffer();

    void extend_room(int size);

    int readable_size() const {
        return write_pos_ - read_pos_;
    }

    int writable_size() const {
        return capacity_ - write_pos_;
    }

    int append_string(const char* data, int size);
    int append_string(const char* data);
    int append_string(const std::string& data);

    int append_head(const int length);

    int append_package(const std::string& data);

    int socket_read(int fd);
    char* find_crlf() const;
    int send_data(int socket);

    char* data() const {
        return data_ + read_pos_;
    }

    std::string data(const int length) {
        std::string msg(data(), length);

        read_pos_ += length;

        return msg;
    }

    int read_pos_increase(const int count) {
        read_pos_ += count;
        return read_pos_;
    }

  private:
    char* data_;
    int capacity_;
    int read_pos_;
    int write_pos_;
};