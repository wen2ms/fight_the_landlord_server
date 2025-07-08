#pragma once

#include <functional>
#include <iostream>
#include <map>

#include "buffer.h"
#include "http_response.h"

enum class ProcessingStatus : char {
    kParseReqLine,
    kParseReqHeaders,
    kParseReqBody,
    kParseReqDone
};

class HttpRequest {
  public:
    HttpRequest();
    ~HttpRequest();

    void reset();

    ProcessingStatus get_status() {
        return cur_status_;
    }

    void add_header(const std::string& key, const std::string& value);
    std::string get_header(const std::string& key);
    bool parse_request_line(Buffer* read_buf);
    bool parse_request_header(Buffer* read_buf);
    bool parse_http_request(Buffer* read_buf, HttpResponse* response, Buffer* send_buf, int socket);
    bool process_http_request(HttpResponse* response);
    static void send_dir(const std::string& dir_name, Buffer* send_buf, int cfd);
    static void send_file(const std::string& file_name, Buffer* send_buf, int cfd);

    void set_method(const std::string& method) {
        method_ = method;
    }

    void set_url(const std::string& url) {
        url_ = url;
    }

    void set_version(const std::string& version) {
        version_ = version;
    }

    void set_status(ProcessingStatus status) {
        cur_status_ = status;
    }

  private:
    char* split_request_line(const char* start, const char* end, const char* sub,
                             std::function<void(const std::string&)> callback);

    std::string decode_msg(std::string& msg);
    int hex_to_dec(char c);
    const std::string get_content_type(const std::string& file_name);

    std::string method_;
    std::string url_;
    std::string version_;
    std::map<std::string, std::string> req_headers_;
    ProcessingStatus cur_status_;
};