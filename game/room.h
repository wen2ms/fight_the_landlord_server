#ifndef ROOM_H
#define ROOM_H

#include <sw/redis++/redis++.h>

#include <string>

class Room {
  public:
    Room();
    ~Room();

    bool init_environment();

    void clear();

    void save_rsa_key(const std::string& field, const std::string& value);

    std::string rsa_key(const std::string& field);

  private:
    sw::redis::Redis* redis_;
};

#endif  // ROOM_H
