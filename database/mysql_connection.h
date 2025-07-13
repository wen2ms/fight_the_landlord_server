#ifndef MYSQL_CONNECTION_H
#define MYSQL_CONNECTION_H

#include <mysql/mysql.h>

#include <chrono>
#include <iostream>

class MysqlConnection {
public:
    MysqlConnection();
    ~MysqlConnection();

    bool connect(const std::string& user, const std::string& password, const std::string& db_name, const std::string& ip,
                 unsigned short port = 3306) const;

    bool update(const std::string& sql) const;
    bool query(const std::string& sql);

    bool next();
    std::string value(int index);

    bool transaction() const;
    bool commit() const;
    bool rollback() const;

    void refresh_alive_time();
    long long get_alive_time() const;

private:
    void free_result();

    MYSQL* connection_;
    MYSQL_RES* result_;
    MYSQL_ROW row_;

    std::chrono::steady_clock::time_point alive_time_;
};

#endif  // MYSQL_CONNECTION_H