#ifndef JSON_PARSE_H
#define JSON_PARSE_H

#include <jsoncpp/json/json.h>

#include <string>
#include <memory>

struct DBInfo {
    std::string ip;
    unsigned short port;
    std::string user;
    std::string password;
    std::string db_name;
};

class JsonParse {
  public:
    enum DBType {
        kMysql,
        kRedis
    };

    explicit JsonParse(const std::string& file_name = "/home/ecs-user/Codes/fight_the_landlord_server/config/config.json");

    std::shared_ptr<DBInfo> get_database_info(DBType type);

  private:
    Json::Value root_;
};

#endif  // JSON_PARSE_H
