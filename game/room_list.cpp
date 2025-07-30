#include "room_list.h"

RoomList* RoomList::get_instance() {
    static RoomList room_list;

    return &room_list;
}

void RoomList::add_user(const std::string& room_name, const std::string& user_name, callback send_message) {
    std::lock_guard locker(mutex_);
    auto iter = room_map_.find(room_name);

    if (iter != room_map_.end()) {
        iter->second.emplace(user_name, send_message);
    } else {
        UserMap user_map = {{user_name, send_message}};
        room_map_.emplace(room_name, user_map);
    }
}

UserMap RoomList::get_players(const std::string& room_name) {
    std::lock_guard locker(mutex_);
    const auto iter = room_map_.find(room_name);

    if (iter == room_map_.end()) {
        return {};
    }

    return iter->second;
}

UserMap RoomList::get_remaining_players(const std::string& room_name, const std::string& user_name) {
    UserMap players = get_players(room_name);
    if (players.size() > 1) {
        auto iter = players.find(user_name);
        if (iter != players.end()) {
            players.erase(iter);
            return players;
        }
    }
    return {};
}
