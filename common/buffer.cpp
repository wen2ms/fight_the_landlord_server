#include "buffer.h"

#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include "log.h"

Buffer::Buffer(int size) : capacity_(size), read_pos_(0), write_pos_(0) {
    data_ = (char*)malloc(size);
    bzero(data_, size);
}

Buffer::~Buffer() {
    if (data_ != nullptr) {
        free(data_);
    }
}

void Buffer::extend_room(int size) {
    if (writable_size() >= size) {
        return;
    } else if (read_pos_ + writable_size() >= size) {
        int readable = readable_size();
        memcpy(data_, data_ + read_pos_, readable);
        read_pos_ = 0;
        write_pos_ = readable;
    } else {
        char* temp = static_cast<char*>(realloc(data_, capacity_ + size));
        if (temp == NULL) {
            return;
        }
        memset(temp + capacity_, 0, size);
        data_ = temp;
        capacity_ += size;
    }
}

int Buffer::append_string(const char* data, int size) {
    if (data == nullptr || size <= 0) {
        return -1;
    }
    extend_room(size);
    memcpy(data_ + write_pos_, data, size);
    write_pos_ += size;
    return 0;
}

int Buffer::append_string(const char* data) {
    int size = strlen(data);
    int ret = append_string(data, size);
    return ret;
}

int Buffer::append_string(const std::string& data) {
    int ret = append_string(data.c_str(), data.length());

    return ret;
}

int Buffer::append_head(const int length) {
    int len = htonl(length);

    DEBUG("Recv head len: %d", len);

    std::string head(reinterpret_cast<char*>(&len), sizeof(len));

    append_string(head);

    return 0;
}

int Buffer::append_package(const std::string& data) {
    append_head(data.length());

    append_string(data);

    return 0;
}

int Buffer::socket_read(int fd) {
    struct iovec vec[2];
    int writable = writable_size();

    vec[0].iov_base = data_ + write_pos_;
    vec[0].iov_len = writable;

    char* tmpbuf = (char*)malloc(40960);

    vec[1].iov_base = tmpbuf;
    vec[1].iov_len = 40960;

    int result = readv(fd, vec, 2);

    if (result == -1) {
        return -1;
    } else if (result <= writable) {
        write_pos_ += result;
    } else {
        write_pos_ = capacity_;
        append_string(tmpbuf, result - writable);
    }

    free(tmpbuf);
    return result;
}

char* Buffer::find_crlf() const {
    char* ptr = static_cast<char*>(memmem(data_ + read_pos_, readable_size(), "\r\n", 2));
    return ptr;
}

int Buffer::send_data(int socket) {
    int readable = readable_size();
    
    if (readable > 0) {
        int count = send(socket, data_ + read_pos_, readable, MSG_NOSIGNAL);

        if (count > 0) {
            read_pos_ += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
