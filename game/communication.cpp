#include "communication.h"

#include <netinet/in.h>

#include "log.h"
#include "rsacrypto.h"

void Communication::parse_request(Buffer* buf) {
    std::string data = buf->data(sizeof(int));
    int length = *reinterpret_cast<int*>(data.data());

    length = ntohl(length);
    data = buf->data(length);

    Codec codec(data);
    std::shared_ptr<Message> ptr = codec.decode_msg();
    Message res_msg;

    switch (ptr->reqcode) {
        case RequestCode::USER_LOGIN:
            break;
        case RequestCode::REGISTER:
            break;
        case RequestCode::AES_DISTRIBUTION:
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

    aes_key_ = rsa.pri_key_decrypt(req_msg->data1);

    Hash hash(HashType::kSha224);

    hash.add_data(aes_key_);

    std::string res = hash.result(Hash::Type::kHex);

    res_msg.rescode = AES_VERIFY_OK;

    if (req_msg->data2 == res) {
        DEBUG("Private key verify successfully!");
    } else {
        DEBUG("Private key verify failed!");

        res_msg.rescode = AES_VERIFY_FAILED;
        res_msg.data1 = "Aes verify failed...";
    }
}