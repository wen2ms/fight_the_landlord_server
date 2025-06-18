#include "http_request.h"

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "tcp_connection.h"

HttpRequest::HttpRequest() {
    reset();
}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset() {
    cur_status_ = ProcessingStatus::kParseReqLine;
    method_ = url_ = version_ = std::string();
    req_headers_.clear();
}

void HttpRequest::add_header(const std::string& key, const std::string& value) {
    if (key.empty() || value.empty()) {
        return;
    }
    req_headers_.insert(std::make_pair(key, value));
}

std::string HttpRequest::get_header(const std::string& key) {
    auto it = req_headers_.find(key);
    if (it == req_headers_.end()) {
        return std::string();
    }

    return it->second;
}

bool HttpRequest::parse_request_line(Buffer* read_buf) {
    char* end = read_buf->find_crlf();
    char* start = read_buf->data();
    int line_size = end - start;

    if (line_size > 0) {
        auto method_func = std::bind(&HttpRequest::set_method, this, std::placeholders::_1);
        start = split_request_line(start, end, " ", method_func);

        auto url_func = std::bind(&HttpRequest::set_url, this, std::placeholders::_1);
        start = split_request_line(start, end, " ", url_func);

        auto version_func = std::bind(&HttpRequest::set_version, this, std::placeholders::_1);
        split_request_line(start, end, nullptr, version_func);

        read_buf->read_pos_increase(line_size + 2);
        set_status(ProcessingStatus::kParseReqHeaders);
        return true;
    }

    return false;
}

bool HttpRequest::parse_request_header(Buffer* read_buf) {
    char* end = read_buf->find_crlf();

    if (end != nullptr) {
        char* start = read_buf->data();
        int line_size = end - start;
        char* middle = static_cast<char*>(memmem(start, line_size, ": ", 2));

        if (middle != nullptr) {
            int key_length = middle - start;
            int value_length = end - middle - 2;
            if (key_length > 0 && value_length > 0) {
                std::string key(start, key_length);
                std::string value(start, value_length);
                add_header(key, value);
            }

            read_buf->read_pos_increase(line_size + 2);
        } else {
            read_buf->read_pos_increase(2);
            set_status(ProcessingStatus::kParseReqDone);
        }
        return true;
    }
    return false;
}

bool HttpRequest::parse_http_request(Buffer* read_buf, HttpResponse* response, Buffer* send_buf, int socket) {
    bool flag = true;
    while (cur_status_ != ProcessingStatus::kParseReqDone) {
        switch (cur_status_) {
            case ProcessingStatus::kParseReqLine:
                flag = parse_request_line(read_buf);
                break;
            case ProcessingStatus::kParseReqHeaders:
                flag = parse_request_header(read_buf);
                break;
            case ProcessingStatus::kParseReqBody:
                break;
            default:
                break;
        }

        if (!flag) {
            return flag;
        }
        if (cur_status_ == ProcessingStatus::kParseReqDone) {
            process_http_request(response);

            response->prepare_msg(send_buf, socket);
        }
    }

    cur_status_ = ProcessingStatus::kParseReqLine;
    return flag;
}

bool HttpRequest::process_http_request(HttpResponse* response) {
    if (strcasecmp(method_.data(), "get") != 0) {
        return false;
    }

    url_ = decode_msg(url_);

    const char* file = NULL;
    if (strcmp(url_.data(), "/") == 0) {
        file = "./";
    } else {
        file = url_.data() + 1;
    }

    struct stat st;
    int ret = stat(file, &st);
    if (ret == -1) {
        response->set_file_name("404.html");
        response->set_status_code(StatusCode::kNotFound);
        response->add_header("Content-type", get_content_type(".html"));
        response->send_data_func_ = send_file;
        return true;
    }

    response->set_file_name(file);
    response->set_status_code(StatusCode::kOK);
    if (S_ISDIR(st.st_mode)) {
        response->add_header("Content-type", get_content_type(".html"));
        response->send_data_func_ = send_dir;
    } else {
        response->add_header("Content-type", get_content_type(file));
        response->add_header( "Content-length", std::to_string(st.st_size));
        response->send_data_func_ = send_file;
    }

    return true;
}

void HttpRequest::send_dir(const std::string& dir_name, Buffer* send_buf, int cfd) {
    char buf[4096] = {0};
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dir_name.data());

    struct dirent** namelist;
    int num = scandir(dir_name.data(), &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i) {
        char* name = namelist[i]->d_name;
        struct stat st;
        char sub_path[1024] = {0};
        sprintf(sub_path, "%s/%s", dir_name.data(), name);
        stat(sub_path, &st);
        if (S_ISDIR(st.st_mode)) {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        } else {
            sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", name, name, st.st_size);
        }

        send_buf->append_string(buf);
#ifndef MSG_SEND_AUTO
        send_buf->send_data(cfd);
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }

    sprintf(buf, "</table></body></html>");
    send_buf->append_string(buf);
#ifndef MSG_SEND_AUTO
    send_buf->send_data(cfd);
#endif
    free(namelist);
}

void HttpRequest::send_file(const std::string& file_name, Buffer* send_buf, int cfd) {
    int fd = open(file_name.data(), O_RDONLY);
    assert(fd > 0);

    while (1) {
        char buf[1024];
        int len = read(fd, buf, sizeof(buf));
        if (len > 0) {
            send_buf->append_string(buf, len);
#ifndef MSG_SEND_AUTO
            send_buf->send_data(cfd);
#endif
        } else if (len == 0) {
            break;
        } else {
            close(fd);
            perror("read");
        }
    }
    close(fd);
}

char* HttpRequest::split_request_line(const char* start, const char* end, const char* sub,
                                      std::function<void(const std::string&)> callback) {
    char* space = const_cast<char*>(end);
    if (sub != nullptr) {
        space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
        assert(space != nullptr);
    }
    int length = space - start;
    callback(std::string(start, length));

    return space + 1;
}

std::string HttpRequest::decode_msg(std::string& msg) {
    std::string str = std::string();
    const char* from = msg.data();
    for (; *from != '\0'; ++from) {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
            str.append(1, hex_to_dec(from[1]) * 16 + hex_to_dec(from[2]));
            from += 2;
        } else {
            str.append(1, *from);
        }
    }
    str.append(1, '\0');
    return str;
}

int HttpRequest::hex_to_dec(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return 0;
}

const std::string HttpRequest::get_content_type(const std::string& file_name) {
    const char* dot = strrchr(file_name.data(), '.');
    if (dot == NULL) {
        return "text/plain; charset=utf-8";
    } else if (strcasecmp(dot, ".html") == 0 || strcasecmp(dot, ".htm") == 0) {
        return "text/html; charset=utf-8";
    } else if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(dot, ".gif") == 0) {
        return "image/gif";
    } else if (strcasecmp(dot, ".png") == 0) {
        return "image/png";
    } else if (strcasecmp(dot, ".css") == 0) {
        return "text/css";
    } else if (strcasecmp(dot, ".au") == 0) {
        return "audio/basic";
    } else if (strcasecmp(dot, ".wav") == 0) {
        return "audio/wav";
    } else if (strcasecmp(dot, ".avi") == 0) {
        return "video/x-msvideo";
    } else if (strcasecmp(dot, ".midi") == 0 || strcasecmp(dot, ".mid") == 0) {
        return "audio/midi";
    } else if (strcasecmp(dot, ".mp3") == 0) {
        return "audio/mpeg";
    } else if (strcasecmp(dot, ".mov") == 0 || strcasecmp(dot, ".qt") == 0) {
        return "video/quicktime";
    } else if (strcasecmp(dot, ".mpeg") == 0 || strcasecmp(dot, ".mpe") == 0) {
        return "video/mpeg";
    } else if (strcasecmp(dot, ".vrml") == 0 || strcasecmp(dot, ".vrl") == 0) {
        return "model/vrml";
    } else if (strcasecmp(dot, ".ogg") == 0) {
        return "application/ogg";
    } else if (strcasecmp(dot, ".pac") == 0) {
        return "application/x-ns-proxy-autoconfig";
    } else {
        return "text/plain; charset=utf-8";
    }
}
