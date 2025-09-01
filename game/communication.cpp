#include <netinet/in.h>

#include "communication.h"
#include "information.pb.h"
#include "json_parse.h"
#include "log.h"
#include "room.h"
#include "room_list.h"
#include "rsacrypto.h"

Communication::Communication() : aes_crypto_(nullptr), mysql_conn_(new MysqlConnection) {
    JsonParse json;
    std::shared_ptr<DBInfo> info = json.get_database_info(JsonParse::kMysql);

    bool success = mysql_conn_->connect(info->user, info->password, info->db_name, info->ip, info->port);

    assert(success);

    redis_ = new Room;
    assert(redis_->init_environment());
}

Communication::~Communication() {
    delete redis_;

    delete aes_crypto_;

    delete mysql_conn_;
}

void Communication::parse_request(Buffer* buf) {
    std::string data = buf->data(sizeof(int));
    int length = *reinterpret_cast<int*>(data.data());

    length = ntohl(length);
    data = buf->data(length);

    if (aes_crypto_ != nullptr) {
        data = aes_crypto_->decrypt(data);
    }

    Codec codec(data);
    send_callback send_func = send_message_;
    std::shared_ptr<Message> ptr = codec.decode_msg();
    Message res_msg;

    switch (ptr->reqcode) {
        case USER_LOGIN:
            handle_login(ptr.get(), res_msg);
            break;
        case REGISTER:
            handle_register(ptr.get(), res_msg);
            break;
        case AES_DISTRIBUTION:
            handle_aes_distribution(ptr.get(), res_msg);
            break;
        case AUTO_CREATE_ROOM:
        case MANUAL_CREATE_ROOM:
            handle_add_room(ptr.get(), res_msg);
            send_func = std::bind(&Communication::ready_for_play, this, res_msg.room_name, std::placeholders::_1);
            break;
        case BID_LORD:
            res_msg.data1 = ptr->data1;
            res_msg.rescode = OTHER_BID_LORD;
            send_func =
                std::bind(&Communication::notify_other_players, this, std::placeholders::_1, ptr->room_name, ptr->user_name);
            break;
        case PLAY_A_HAND:
            res_msg.data1 = ptr->data1;
            res_msg.data2 = ptr->data2;
            res_msg.rescode = OTHER_PLAY_A_HAND;
            send_func =
                std::bind(&Communication::notify_other_players, this, std::placeholders::_1, ptr->room_name, ptr->user_name);
            break;
        case GAME_OVER: {
            int score = std::stoi(ptr->data1);
            redis_->update_player_score(ptr->room_name, ptr->user_name, score);
            char sql[1024];
            sprintf(sql, "UPDATE information SET score = %d WHERE name = %s", score, ptr->user_name.data());
            mysql_conn_->update(sql);
            send_func = nullptr;
            break;
        }
        case CONTINUE:
            restart_game(ptr.get());
            send_func = nullptr;
            break;
        case SEARCH_ROOM: {
            bool success = redis_->search_room(ptr->room_name);
            res_msg.rescode = SEARCH_ROOM_OK;
            res_msg.data1 = success ? "true" : "false";
            break;
        }
        case LEAVE_ROOM: {
            handle_leave_room(ptr.get(), res_msg);
            send_func = nullptr;
            break;
        }
        case EXIT:
            handle_exit(ptr.get());
            send_func = nullptr;
            break;
        default:
            break;
    }

    if (send_func != nullptr) {
        codec.reload(&res_msg);
        send_func(codec.encode_msg());
    }
}
void Communication::handle_aes_distribution(const Message* req_msg, Message& res_msg) {
    RsaCrypto rsa;

    rsa.prase_string_to_key(redis_->rsa_key("private_key"), RsaCrypto::kPrivateKey);

    std::string aes_key = rsa.pri_key_decrypt(req_msg->data1);

    Hash hash(HashType::kSha224);

    hash.add_data(aes_key);

    std::string res = hash.result(Hash::Type::kHex);

    res_msg.rescode = AES_VERIFY_OK;

    if (req_msg->data2 == res) {
        aes_crypto_ = new AesCrypto(AesCrypto::kAesCbc256, aes_key);

        DEBUG("Private key verify successfully!");
    } else {
        DEBUG("Private key verify failed!");

        res_msg.rescode = FAILED;
        res_msg.data1 = "Aes verify failed...";
    }
}

void Communication::handle_register(const Message* req_msg, Message& res_msg) {
    char sql[1024];

    sprintf(sql, "SELECT name, password, phone, date FROM user WHERE name = '%s';", req_msg->user_name.data());

    bool success = mysql_conn_->query(sql);

    if (success && !mysql_conn_->next()) {
        mysql_conn_->transaction();

        sprintf(sql, "INSERT INTO user (name, password, phone, date) VALUES ('%s', '%s', '%s', NOW());",
                req_msg->user_name.data(), req_msg->data1.data(), req_msg->data2.data());

        bool success_user = mysql_conn_->update(sql);

        sprintf(sql, "INSERT INTO information (name, score, status) VALUES ('%s', 0, 0);", req_msg->user_name.data());

        bool success_info = mysql_conn_->update(sql);

        if (success_user && success_info) {
            mysql_conn_->commit();

            res_msg.rescode = REGISTER_OK;
        } else {
            mysql_conn_->rollback();

            res_msg.rescode = FAILED;
            res_msg.data1 = "Insert into database failed";
        }
    } else {
        res_msg.rescode = FAILED;
        res_msg.data1 = "Name duplicated, register failed";
    }
}

void Communication::handle_login(const Message* req_msg, Message& res_msg) {
    char sql[1024];

    sprintf(sql,
            "SELECT user.name FROM user JOIN information ON user.name = information.name AND information.status = 0 WHERE "
            "user.name = '%s' AND user.password = '%s';",
            req_msg->user_name.data(), req_msg->data1.data());

    bool success = mysql_conn_->query(sql);

    if (success && mysql_conn_->next()) {
        mysql_conn_->transaction();

        sprintf(sql, "UPDATE information SET status = 1 WHERE name = '%s';", req_msg->user_name.data());

        success = mysql_conn_->update(sql);

        if (success) {
            mysql_conn_->commit();

            res_msg.rescode = LOGIN_OK;

            return;
        }

        mysql_conn_->rollback();
    }

    res_msg.rescode = FAILED;
    res_msg.data1 = "Login failed...";
}

void Communication::handle_add_room(const Message* req_msg, Message& res_msg) {
    std::string user_name = req_msg->user_name;
    std::string old_room_name = redis_->get_player_room_name(user_name);
    int score = redis_->get_player_score(old_room_name, user_name);
    if (!old_room_name.empty()) {
        redis_->leave_room(old_room_name, req_msg->user_name);
        RoomList::get_instance()->remove_player(old_room_name, req_msg->user_name);
    }

    bool join_success = true;
    std::string room_name;

    if (req_msg->reqcode == AUTO_CREATE_ROOM) {
        room_name = redis_->join_room(user_name);
    } else {
        room_name = req_msg->room_name;
        join_success = redis_->join_room(room_name, user_name);
    }

    if (join_success) {
        if (score == 0) {
            std::string sql = "SELECT score FROM information WHERE name = '" + user_name + "';";
            bool query_success = mysql_conn_->query(sql);

            assert(query_success);

            mysql_conn_->next();
            score = std::stoi(mysql_conn_->value(0));
        }

        redis_->update_player_score(room_name, user_name, score);

        RoomList* room_list = RoomList::get_instance();

        room_list->add_user(room_name, user_name, send_message_);

        res_msg.rescode = JOIN_ROOM_OK;
        res_msg.data1 = std::to_string(redis_->get_nums_players(room_name));
        res_msg.room_name = room_name;
    } else {
        res_msg.rescode = FAILED;
        res_msg.data1 = "Sorry, failed to join room, room is full";
    }
}

void Communication::handle_leave_room(const Message* req_msg, Message& res_msg) {
    redis_->leave_room(req_msg->room_name, req_msg->user_name);
    RoomList::get_instance()->remove_player(req_msg->room_name, req_msg->user_name);
    res_msg.rescode = OTHER_LEAVE_ROOM;
    UserMap players = RoomList::get_instance()->get_players(req_msg->room_name);
    res_msg.data1 = std::to_string(players.size());
    for (const auto& [player, func] : players) {
        Codec codec(&res_msg);
        func(codec.encode_msg());
    }
}

void Communication::handle_exit(const Message* req_msg) {
    char sql[10240] = {0};
    sprintf(sql, "UPDATE information SET status = 0 WHERE name = '%s';", req_msg->user_name.data());
    mysql_conn_->update(sql);
    disconnect_();
}

void Communication::ready_for_play(const std::string& room_name, const std::string& data) {
    RoomList* room_list_ = RoomList::get_instance();
    UserMap players = room_list_->get_players(room_name);

    for (const auto& [user_name, callback] : players) {
        callback(data);
    }
    if (players.size() == 3) {
        start_game(room_name, players);
    }
}

void Communication::deal_cards(UserMap players) {
    init_cards();

    Message message;

    std::string& all_cards = message.data1;
    for (int i = 0; i < 51; ++i) {
        auto [suit, rank] = take_one_card();
        std::string sub_card = std::to_string(suit) + "-" + std::to_string(rank) + "#";

        all_cards += sub_card;
    }

    std::string& last_cards = message.data2;
    for (const auto& [suit, rank] : cards_) {
        std::string sub_card = std::to_string(suit) + "-" + std::to_string(rank) + "#";

        last_cards += sub_card;
    }

    message.rescode = DEAL_CARDS;

    Codec codec(&message);
    for (const auto& player : players) {
        player.second(codec.encode_msg());
    }
}

void Communication::init_cards() {
    cards_.clear();

    for (int suit = 1; suit <= 4; ++suit) {
        for (int rank = 1; rank <= 13; ++rank) {
            cards_.emplace(suit, rank);
        }
    }

    cards_.emplace(0, 14);
    cards_.emplace(0, 15);
}

std::pair<int, int> Communication::take_one_card() {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<int> distribution(0, cards_.size() - 1);
    int random = distribution(generator);
    auto iter = cards_.begin();

    for (int i = 0; i < random; ++i, ++iter) {
    }

    cards_.erase(iter);

    return *iter;
}

void Communication::notify_other_players(const std::string& data, const std::string& room_name, const std::string& user_name) {
    UserMap players = RoomList::get_instance()->get_remaining_players(room_name, user_name);
    for (const auto& [user_name, callback] : players) {
        callback(data);
    }
}

void Communication::restart_game(const Message* req_msg) {
    UserMap players = RoomList::get_instance()->get_players(req_msg->room_name);
    if (players.size() == 3) {
        RoomList::get_instance()->remove_room(req_msg->room_name);
    }
    RoomList::get_instance()->add_user(req_msg->room_name, req_msg->user_name, send_message_);
    players = RoomList::get_instance()->get_players(req_msg->room_name);
    if (players.size() == 3) {
        start_game(req_msg->room_name, players);
    }
}

void Communication::start_game(const std::string& room_name, const UserMap& players) {
    deal_cards(players);

    Message message;

    message.rescode = START_GAME;
    message.data1 = redis_->players_order(room_name);

    Codec codec(&message);
    for (const auto& [user_name, callback] : players) {
        callback(codec.encode_msg());
    }
}