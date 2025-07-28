#ifndef ROOM_LIST_H
#define ROOM_LIST_H

#include <map>
#include <string>
#include <functional>
#include <mutex>

using callback = std::function<void(const std::string&)>;
using UserMap = std::map<std::string, callback>;

class RoomList {
  public:
    RoomList(const RoomList& other) = delete;
    RoomList& operator=(const RoomList& other) = delete;

    static RoomList* get_instance();

    void add_user(const std::string& room_name, const std::string& user_name, callback send_message);

    UserMap get_players(const std::string& room_name);

  private:
    RoomList() = default;

    std::map<std::string, UserMap> room_map_;
    std::mutex mutex_;
};

#endif  // ROOM_LIST_H
