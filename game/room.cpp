#include "room.h"

#include <random>

#include "json_parse.h"
#include "log.h"

Room::Room() : redis_(nullptr), single_room_("single_room"), double_room_("double_room"), triple_room_("triple_room") {}

Room::~Room() {
    if (redis_ != nullptr) {
        delete redis_;
        redis_ = nullptr;
    }
}

bool Room::init_environment() {
    JsonParse json_parse;
    std::shared_ptr<DBInfo> info = json_parse.get_database_info(JsonParse::kRedis);
    std::string connection = "tcp://" + info->ip + ":" + std::to_string(info->port);

    redis_ = new sw::redis::Redis(connection);
    if (redis_->ping() == "PONG") {
        DEBUG("Connect to redis successfully!");
        return true;
    }

    return false;
}

void Room::clear() {
    redis_->flushdb();
}

void Room::save_rsa_key(const std::string& field, const std::string& value) {
    redis_->hset("RSA", field, value);
}

std::string Room::rsa_key(const std::string& field) {
    auto value = redis_->hget("RSA", field);

    if (value.has_value()) {
        return value.value();
    }

    return {};
}

std::string Room::join_room(const std::string& user_name) {
    std::optional<std::string> room_name;

    do {
        if (redis_->scard(double_room_) > 0) {
            room_name = redis_->srandmember(double_room_);
            break;
        }

        if (redis_->scard(single_room_) > 0) {
            room_name = redis_->srandmember(single_room_);
            break;
        }

        room_name = get_new_room_name();
    } while (false);

    join_room(user_name, room_name.value());

    return room_name.value();
}

bool Room::join_room(const std::string& user_name, const std::string& room_name) const {
    if (redis_->zcard(room_name) >= 3) {
        return false;
    }

    if (!redis_->exists(room_name)) {
        redis_->sadd(single_room_, room_name);
    } else if (redis_->sismember(single_room_, room_name)) {
        redis_->smove(single_room_, double_room_, room_name);
    } else if (redis_->sismember(double_room_, room_name)) {
        redis_->smove(double_room_, triple_room_, room_name);
    } else {
        assert(false);
    }

    redis_->zadd(room_name, user_name, 0);
    redis_->hset("players", user_name, room_name);

    return true;
}

std::string Room::get_new_room_name() {
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<int> distribution(100000, 999999);

    int random_room = distribution(generator);

    return std::to_string(random_room);
}

int Room::get_nums_players(const std::string& room_name) const {
    return redis_->zcard(room_name);
}

void Room::update_player_score(const std::string& room_name, const std::string& user_name, int score) const {
    redis_->zadd(room_name, user_name, score);
}

std::string Room::get_player_room_name(const std::string& user_name) const {
    auto room_name = redis_->hget("players", user_name);

    if (room_name.has_value()) {
        return room_name.value();
    }

    return {};
}

int Room::get_player_score(const std::string& user_name, const std::string& room_name) const {
    auto score = redis_->zscore(room_name, user_name);

    if (score.has_value()) {
        return score.value();
    }

    return 0;
}

std::string Room::players_order(const std::string& room_name) {
    int index = 1;
    std::string data;
    std::vector<std::pair<std::string, double>> output;

    redis_->zrevrange(room_name, 0, - 1, std::back_inserter(output));
    for (auto& [user_name, score] : output) {
        data += user_name + "-" + std::to_string(index) + "-" + std::to_string(static_cast<int>(score)) + "#";

        ++index;
    }

    return data;
}
