#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "aescrypto.h"
#include "buffer.h"
#include "codec.h"
#include "mysql_connection.h"
#include "room.h"

class Communication {
  public:
    using send_callback = std::function<void(const std::string&)>;
    using delete_callback = std::function<void()>;

    Communication();
    ~Communication();

    void parse_request(Buffer* buf);

    void handle_aes_distribution(const Message* req_msg, Message& res_msg);
    void handle_register(const Message* req_msg, Message& res_msg);
    void handle_login(const Message* req_msg, Message& res_msg);
    void handle_add_room(const Message* req_msg, Message& res_msg);

    void set_callbacks(const send_callback& send_func, const delete_callback& delete_func) {
        send_message_ = send_func;
        disconnect_ = delete_func;
    }

  private:
    AesCrypto* aes_crypto_;

    send_callback send_message_;
    delete_callback disconnect_;

    MysqlConnection* mysql_conn_;
    Room* redis_;
};

#endif  // COMMUNICATION_H
