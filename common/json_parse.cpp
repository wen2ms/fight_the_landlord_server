#include "json_parse.h"

#include <cassert>
#include <fstream>

JsonParse::JsonParse(std::string file_name) {
    std::ifstream infile_stream(file_name);

    assert(infile_stream.is_open());

    Json::Reader reader;

    reader.parse(infile_stream, root_);

    assert(root_.isObject());
}

std::shared_ptr<DBInfo> JsonParse::get_database_info(DBType type) {
    std::string db_type = (type == kMysql ? "mysql" : "redis");
    Json::Value node = root_[db_type];
    DBInfo* info = new DBInfo;

    info->ip = node["ip"].asString();
    info->port = node["port"].asInt();

    if (type == kMysql) {
        info->user = node["user"].asString();
        info->password = node["password"].asString();
        info->dbname = node["dbname"].asString();
    }

    return std::shared_ptr<DBInfo>(info);
}