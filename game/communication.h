#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "buffer.h"
#include "codec.h"
#include "aescrypto.h"

class Communication {
  public:
    using send_callback = std::function<void(const std::string&)>;
    using delete_callback = std::function<void()>;

    Communication();

    void parse_request(Buffer* buf);

    void handle_aes_distribution(const Message* req_msg, Message& res_msg);

    void set_callbacks(const send_callback& send_func, const delete_callback& delete_func) {
        send_message_ = send_func;
        disconnect_ = delete_func;
    }

  private:
    AesCrypto* aes_crypto_;

    send_callback send_message_;
    delete_callback disconnect_;
};

#endif  // COMMUNICATION_H
