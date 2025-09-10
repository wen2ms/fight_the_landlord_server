#### 1. 报文格式

```proto
message Information {
    string user_name = 1;
    string room_name = 2;
    string data1 = 3;
    string data2 = 4;
    string data3 = 5;
    RequestCode reqcode = 6;
    ResponseCode rescode = 7;
}
```

#### 2. 通信流程

##### 2.1 建立连接和非对称加密

首先客户端向服务端发起连接连接请求，然后服务端收到后，就会将`RSA`已经生成好的`public_key`，长度为`RsaCrypto::kBits2k = 2048`，经过数字签名的`public_key`发送给客户端，这些数据都是通过`Base64`编码之后的。

`data1 = public_key; data2 = sign; rescode = RSA_DISTRIBUTION;` 。

##### 2.2 对称加密

客户端收到服务端的消息后，解码并验证数字签名，成功后向服务端发送使用`RSA`公钥加密后的`AES`私钥，以及`AES`私钥通过`QCryptographicHash::Sha224`得到的`hash`。

`data1 = rsa.pub_key_encrypt(aes_key_); data2 = hash; reqcode = AES_DISTRIBUTION;`。

服务端收到客户端的消息后，同样使用`HashType::kSha224`验证`hash`并用`RSA private_key`解密后得到双方用于对称加密的`AES`密钥，最后回复给客户端验证成功，规定之后的加密都是用`AesCrypto::kAesCbc256`进行加密。

`rescode = AES_VERIFY_OK;`

##### 2.3 登录与注册

当建立好加密连接后，客户端将注册或登录的参数发送给服务端。注册或登录时会首先都会通过正则校验检验信息是否合法，然后将信息发送给服务端。

登录：`user_name = user_name; data1 = hash(password); reqcode = USER_LOGIN;`

注册：`user_name = user_name; data1 = hash(password); data2 = phone; reqcode = REGISTER;`

这里都是通过`QCryptographicHash::hash(password, QCryptographicHash::Sha224)`进行`hash`。

##### 2.4 更新数据库

服务端接收到注册或者登录消息，解码后更新数据库。

对于登录，首先通过

```sql
SELECT user.name FROM user JOIN information ON user.name = information.name AND information.status = 0 WHERE
user.name = user_name AND user.password = password;
```

查看玩家是否注册并没有登录，如果成功，就开启事务，更新数据库为`UPDATE information SET status = 1 WHERE name = user_name;`成功后就`commit`更新事务并发送消息`rescode = LOGIN_OK;`给客户端，否则就`rollback`，并发送`rescode = FAILED; data1 = "Login failed...";`。

对于注册，首先会查询是否已经存在这个用户对应的记录，当没有的情况下才会开启事务，向`user`表中插入用户信息，就是客户端发送来的`INSERT INTO user (name, password, phone, date) VALUES (user_name, password, phone, NOW());`，然后再向`information`中插入初次注册的信息`INSERT INTO information (name, score, status) VALUES (user_name, 0, 0);`，以上操作都成功后就会`commit`，并向客户端发送`resmsg = REGISTER_OK`。

客户端收到`LOGIN_OK`或者`REGISTER_OK`后就会跳转到选择游戏模式界面中了。

##### 2.5 加入房间

客户端的游戏模式有单机模式`GameModeType::kStandalone`和网络模式`GameModeType::kOnline`，只有在网络模式下，客户端才会和服务器进行游戏数据的通信。加入房间有三种方式，自动创建房间，手动搜索房间并加入，手动创建房间。

对于自动创建房间，客户端会发送`req_code = AUTO_CREATE_ROOM; user_name = user_name`。

对于手动创建房间和手动搜索房间，最后都会有一个通过房间名来加入房间的操作，不同点是手动创建房间不需要搜索房间名，直接输入要加入的房间名然后创建，如果这个房间存在就会直接加入。而手动搜索房间会先搜索这个房间是否存在，只有当存在的时候，加入房间的按钮才会生效，才能加入房间。最后的这个加入房间客户端会发送给服务端`reqcode = MANUAL_CREATE_ROOM; user_name = user_name; room_name = room_name;`。 所有的数据通过`DataManager::get_instance()->communication()->send_message`发送的时候都会进行`AES`对称加密的。

服务端收到`AUTO_CREATE_ROOM`或者`MANUAL_CREATE_ROOM`的消息的处理方法都是通过`handle_add_room`来处理的，它们不同的细节就是通过重载`Room::join_room`，对于自动创建房间，会通过`redis++`的`redis_->scard`依次查询`double_room_`和`single_room_`，如果某一个房间存在，然后通过`srandmember`得到随机的房间名，如果都没有的话就会重新创建一个房间`get_new_room_name`，这个函数会生成一个均值分布在`(100000, 999999)`的数字的房间名。

得到房间名后，就会调用重载的有房间名这个参数的`join_room`，它的行为就是首先判断`zcard(room_name) >= 3`当前的这个房间人数是否满了，如果不满的情况下，判断当前这个房间在`Redis`中不存在的话就会创建一个单人房间`sadd(single_room_, room_name);`，而如果它已经是一个单人间`sismember(single_room, room_name)`，那就会把这个房间从单人间`SET`移动到双人间`SET`的中，通过`smove(single_room_, double_room_, room_name);`。如果是一个双人间就会升级为三人间。

之后就会把当前用户加入到刚刚创建的房间中并设置它的当局分数为0，`zadd(room_name, user_name, 0);`，同时也把这个用户和房间的映射加入到哈希中`hset("players", user_name, room_name);`。

在`Redis`层面中创建房间并加入的操作就完成了，然后还需要针对如果这个用户有历史分数，这个`score`是保存在`mysql`中的，

`SELECT score FROM information WHERE name = user_name;`，就会重新在`Redis`中更新这个历史分数也是通过`zadd(room_name, user_name, score);`来更新的。最后更新单例`room_list->add_user(room_name, user_name, send_message_);`。

至此数据更新的部分就完成了，然后会将`rescode = JOIN_ROOM_OK; data1 = zcard(room_name); room_name = room_name;`保存到要发送的数据中，然后将这个数据通过当前房间中的每个玩家的连接对应的`Communcation`对象中的`send_message_`来发送，实现给每一个玩家进行广播。而`send_message_`实际上是`TcpConnection`中设置的`reply_->set_callbacks(send_func, delete_func);`，对应关系实际上是`tcp_server`中的线程池`ThreadPool* thread_pool_;`每接收一个新的请求时就会取出一个工作线程，并建立一个`EventLoop`和`TcpConnection`。

广播结束后，如果同时房间人数为3，代表可以开始游戏了，此时服务端就会`deal_cards(players);`，同时再次广播新的数据`rescode = START_GAME; data1 = redis_->players_order(room_name);`。

其实对于搜索房间，不同的点在于客户端发送的`reqcode = SEARCH_ROOM;`。而服务端会首先在`Redis`中`sismember(double_room_, room_name)`，然后再查询有没有对应的单人间，也就是说如果没有这个房间，或者这个房间已满，都会搜索失败。这里注意的是无论成功或失败，服务端响应都是`rescode = SEARCH_ROOM_OK`，而`data1 = success ? "true" : "false";`来表明是否查询成功。

##### 2.6 发牌与玩家顺序

`deal_cards`首先会将54张牌的对应点数和花色写入一个`std::multimap<int, int> cards_;`。然后就会将前51张牌按照

```cpp
std::string sub_card = std::to_string(suit) + "-" + std::to_string(rank) + "#";
// std::string& all_cards = message.data1;
all_cards += sub_card;
```

后面三张底牌保存到`data2`中，同时设置`rescode = DEAL_CARDS;`并广播给所有玩家。

`Room::players_order`会利用`redis_->zrevrange(room_name, 0, -1, std::back_inserter(output));`将房间中的所有玩家按照分数由高到低进行排序，保证三个玩家最终的顺序是一致的。

```cpp
for (auto& [user_name, score] : output) {
    data += user_name + "-" + std::to_string(index) + "-" + std::to_string(static_cast<int>(score)) + "#";
    ++index;
}
```

##### 2.7 开始游戏

这个阶段客户端接收到服务端的响应包括`JOIN_ROOM_OK, DEAL_CARDS, START_GAME`。

对于`JOIN_ROOM_OK`，客户端实际上只是更新`gamemode`窗口的房间名和房间人数。而`DEAL_CARDS`就会根据服务端规定的数据格式将字符串转化为`Cards`对象，保存到`DataManager`的`cards_和last_three_cards_`统一管理。最后对于`START_GAME`，客户端会打开主游戏界面`MainWindow`同时根据服务端的玩家顺序数据对不同玩家客户端的主界面的设置不同的布局，同时会断开来自`Communication::start_game`的信号。

不同布局的设置`update_player_info`是按照`mid_name`为当前玩家，逆时针排列，结合服务端发送的数据是按照分数由大到小排列的，也就是说按逆时针分数是依次变小的，这里每次第一个当前玩家一定是分数最高的玩家，也就是这一局叫地主的玩家。

然后游戏就进入了发牌阶段`GameControl::GameStatus::kDealingCard`，这里是动画效果上的发牌，实际上服务端与客户端数据的通信已经结束了，发牌动画实际上是通过`timer_->start(10);`定时器的触发完成的。当卡牌只剩三张时`game_control_->take_remaining_cards().cards_count() == 3`，游戏就会进入到`GameControl::GameStatus::kBiddingLord`叫地主阶段了。这里有一个细节是进入斗地主阶段时，三张在窗口中心的底牌会隐藏，当叫地主结束后就会显示到窗口正上方。

