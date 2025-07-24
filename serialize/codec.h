#ifndef CODEC_H
#define CODEC_H

#include <memory>
#include <string>

#include "information.pb.h"

struct Message {
    std::string user_name;
    std::string room_name;
    std::string data1;
    std::string data2;
    std::string data3;
    RequestCode reqcode;
    ResponseCode rescode;
};

class Codec {
  public:
    explicit Codec(Message *msg);

    explicit Codec(std::string msg);

    std::string encode_msg();

    std::shared_ptr<Message> decode_msg();

    void reload(const std::string &msg);
    void reload(Message *msg);

  private:
    std::string msg_;
    Information obj_;
};

#endif  // CODEC_H
