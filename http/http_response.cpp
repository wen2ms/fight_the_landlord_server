#include "http_response.h"

#include <stdio.h>

#include "log.h"
#include "tcp_connection.h"

HttpResponse::HttpResponse() : status_code_(StatusCode::kUnknown), file_name_(std::string()), send_data_func_(nullptr) {
    headers_.clear();
}

HttpResponse::~HttpResponse() {}

void HttpResponse::add_header(const std::string& key, const std::string& value) {
    if (key.empty() || value.empty()) {
        return;
    }

    headers_.insert(std::make_pair(key, value));
}

void HttpResponse::prepare_msg(Buffer* send_buf, int socket) {
    char tmp[1024] = {0};
    int code = static_cast<int>(status_code_);
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, info_.at(code).data());
    send_buf->append_string(tmp);

    for (auto it = headers_.begin(); it != headers_.end(); it++) {
        sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
        send_buf->append_string(tmp);
    }
    send_buf->append_string("\r\n");
#ifndef MSG_SEND_AUTO
    send_buf->send_data(socket);
#endif

    send_data_func_(file_name_, send_buf, socket);
}
