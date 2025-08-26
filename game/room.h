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

    std::string join_room(const std::string& user_name);
    bool join_room(const std::string& room_name, const std::string& user_name) const;

    int get_nums_players(const std::string& room_name) const;
    void update_player_score(const std::string& room_name, const std::string& user_name, int score) const;
    std::string get_player_room_name(const std::string& user_name) const;
    int get_player_score(const std::string& room_name, const std::string& user_name) const;

    std::string players_order(const std::string& room_name);
    void leave_room(const std::string& room_name, const std::string& user_name);
    bool search_room(const std::string& room_name);

  private:
    static std::string get_new_room_name();

    sw::redis::Redis* redis_;
    const std::string single_room_;
    const std::string double_room_;
    const std::string triple_room_;
    const std::string invalid_room_;
};

#endif  // ROOM_H
