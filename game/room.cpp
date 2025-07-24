#include "room.h"

#include "json_parse.h"
#include "log.h"

Room::Room() : redis_(nullptr) {}

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
