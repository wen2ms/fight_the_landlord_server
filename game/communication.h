#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "aescrypto.h"
#include "buffer.h"
#include "codec.h"
#include "mysql_connection.h"
#include "room.h"
#include "room_list.h"

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

    void ready_for_play(const std::string& room_name, const std::string& data);
    void deal_cards(UserMap players);
    void init_cards();
    std::pair<int, int> take_one_card();

    void notify_other_players(const std::string& data, const std::string& room_name, const std::string& user_name);

  private:
    AesCrypto* aes_crypto_;

    send_callback send_message_;
    delete_callback disconnect_;

    MysqlConnection* mysql_conn_;
    Room* redis_;

    std::multimap<int, int> cards_;
};

#endif  // COMMUNICATION_H
