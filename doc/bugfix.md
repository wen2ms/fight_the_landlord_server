这里需要注意一定要使用`UserMap&`，而不是`UserMap`，因为之后用到的`player`是`players`的一个迭代器，如果不是引用传递，导致删除的时候`iter->second.erase(player);`，不是同一个容器的迭代器，有可能没删除到，其实是一个`Undefined Behaviour`。

```cpp
void RoomList::remove_player(const std::string& room_name, const std::string& user_name) {
    std::lock_guard locker(mutex_);
    auto iter = room_map_.find(room_name);
    if (iter != room_map_.end()) {
        UserMap& players = iter->second;
        auto player = players.find(user_name);
        if (player != players.end() && players.size() > 1) {
            iter->second.erase(player);
        } else if (player != players.end() && players.size() == 1) {
            room_map_.erase(iter);
        }
    }
}
```



