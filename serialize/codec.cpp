#include "codec.h"

Codec::Codec(Message *msg) {
    reload(msg);
}

Codec::Codec(std::string msg) {
    reload(msg);
}

std::string Codec::encode_msg() {
    obj_.SerializeToString(&msg_);
    
    return msg_;
}

std::shared_ptr<Message> Codec::decode_msg() {
    obj_.ParseFromString(msg_);
    
    Message* msg = new Message;
    
    msg->user_name = obj_.user_name();
    msg->data1 = obj_.data1();
    msg->data2 = obj_.data2();
    msg->data3 = obj_.data3();
    msg->reqcode = obj_.reqcode();
    msg->rescode = obj_.rescode();
    
    std::shared_ptr<Message> ptr(msg);
    
    return ptr;
}

void Codec::reload(const std::string& msg) {
    msg_ = msg;
}

void Codec::reload(Message *msg) {
    obj_.set_user_name(msg->user_name);
    obj_.set_data1(msg->data1);
    obj_.set_data2(msg->data2);
    obj_.set_data3(msg->data3);
    obj_.set_reqcode(msg->reqcode);
    obj_.set_rescode(msg->rescode);
}
