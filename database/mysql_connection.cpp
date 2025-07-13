#include "mysql_connection.h"

MysqlConnection::MysqlConnection() : connection_(nullptr), result_(nullptr), row_(nullptr) {
    connection_ = mysql_init(nullptr);

    mysql_set_character_set(connection_, "utf8");
}

MysqlConnection::~MysqlConnection() {
    if (connection_ != nullptr) {
        mysql_close(connection_);
    }

    free_result();
}

bool MysqlConnection::connect(const std::string& user, const std::string& password, const std::string& db_name,
                              const std::string& ip, unsigned short port) const {
    MYSQL* ptr = mysql_real_connect(connection_, ip.c_str(), user.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0);

    return ptr != nullptr;
}

bool MysqlConnection::update(const std::string& sql) const {
    return mysql_query(connection_, sql.c_str()) == 0;
}

bool MysqlConnection::query(const std::string& sql) {
    free_result();

    if (mysql_query(connection_, sql.c_str()) != 0) {
        return false;
    }

    result_ = mysql_store_result(connection_);

    return true;
}

bool MysqlConnection::next() {
    if (result_ != nullptr) {
        row_ = mysql_fetch_row(result_);

        if (row_ != nullptr) {
            return true;
        }
    }

    return false;
}

std::string MysqlConnection::value(int index) {
    int fields_count = mysql_num_fields(result_);

    if (index >= fields_count || index < 0) {
        return {};
    }

    char* val = row_[index];
    unsigned long length = mysql_fetch_lengths(result_)[index];

    return {val, length};
}

bool MysqlConnection::transaction() const {
    return mysql_autocommit(connection_, false);
}

bool MysqlConnection::commit() const {
    bool success = mysql_commit(connection_);

    mysql_autocommit(connection_, true);

    return success;
}

bool MysqlConnection::rollback() const {
    bool success = mysql_rollback(connection_);

    mysql_autocommit(connection_, true);

    return success;
}

void MysqlConnection::refresh_alive_time() {
    alive_time_ = std::chrono::steady_clock::now();
}

long long MysqlConnection::get_alive_time() const {
    std::chrono::nanoseconds nanosec = std::chrono::steady_clock::now() - alive_time_;
    std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(nanosec);

    return millsec.count();
}

void MysqlConnection::free_result() {
    if (result_ != nullptr) {
        mysql_free_result(result_);

        result_ = nullptr;
    }
}
