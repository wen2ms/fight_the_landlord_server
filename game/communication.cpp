#include "communication.h"

#include <netinet/in.h>

#include "json_parse.h"
#include "log.h"
#include "rsacrypto.h"

Communication::Communication() : aes_crypto_(nullptr), mysql_conn_(new MysqlConnection) {
    JsonParse json;
    std::shared_ptr<DBInfo> info = json.get_database_info(JsonParse::kMysql);

    bool success = mysql_conn_->connect(info->user, info->password, info->db_name, info->ip, info->port);

    assert(success);
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
        default:
            break;
    }

    codec.reload(&res_msg);
    send_message_(codec.encode_msg());
}
void Communication::handle_aes_distribution(const Message* req_msg, Message& res_msg) {
    RsaCrypto rsa("private.pem", RsaCrypto::kPrivateKey);

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

        res_msg.rescode = AES_VERIFY_FAILED;
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

            res_msg.rescode = REGISTER_FAILED;
            res_msg.data1 = "Insert into database failed";
        }
    } else {
        res_msg.rescode = REGISTER_FAILED;
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

    res_msg.rescode = LOGIN_FAILED;
    res_msg.data1 = "Login failed...";
}