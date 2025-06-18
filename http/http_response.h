#pragma once

#include <functional>
#include <map>
#include <string>

#include "buffer.h"

enum class StatusCode {
    kUnknown,
    kOK = 200,
    kMovedPermanently = 301,
    kMovedTemporarily = 302,
    kBadRequest = 400,
    kNotFound = 404
};

class HttpResponse {
  public:
    HttpResponse();
    ~HttpResponse();

    void add_header(const std::string& key, const std::string& value);
    void prepare_msg(Buffer* send_buf, int socket);

    inline void set_status_code(StatusCode code) {
        status_code_ = code;
    }

    inline void set_file_name(const std::string& name) {
        file_name_ = name;
    }

    std::function<void(const std::string&, Buffer*, int)> send_data_func_;

  private:
    StatusCode status_code_;
    std::string status_msg_;
    std::string file_name_;
    std::map<std::string, std::string> headers_;
    const std::map<int, std::string> info_ = {
        {200, "OK"}, {301, "Moved Permanently"}, {302, "Moved Temporarily"}, {400, "Bad Request"}, {404, "Not Found"}};
};