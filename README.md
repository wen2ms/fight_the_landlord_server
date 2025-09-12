### fight_the_landlord_server

***

欢迎使用`fight_the_landlord_server`来作为网络斗地主的服务端，它是`epoll/pool/select`的`Reactor`网络服务器，目前已经将斗地主基本功能实现。[fight_the_landlord_client](https://github.com/wen2ms/fight_the_landlord_client)作为相应的客户端，它除了支持网络版还包含单机版。

***

#### 游戏概述

斗地主支持网络版和单机版。网络版三位玩家按照既定的斗地主规则进行扑克对战，包括游戏前阶段：注册与登录账号，自动/手动加入房间，玩家退出。游戏阶段包括抢地主，发牌，出牌，结算积分等阶段的动画和音效展示，通过`cppcheck`优化，最后打包部署。单机版斗地主两个机器人玩家根据算法抢地主与出牌。

***

#### 网络模型

通过将HTTP应用层拓展为`WebSocket`实现双端通信，通信双方通过`protobuf`进行数据序列化，定长读写数据包解决TCP粘包现象，自定义buffer实现读写缓存区，同时在表示层通过`OpenSSL`实现`SSL`握手，服务端使用`Redis`缓存当局游戏信息与密钥对，`mysql`持久化玩家个人信息与登录状态。

整个功能类设计的核心是游戏控制类负责向主界面发出信号并处理玩家对象的信号，游戏策略类中包括机器人抢地主，出牌策略与所有牌的大小关系，其余窗口类负责游戏动画与音频。

#### 通信与功能

服务端的实现是基于多 `Reactor` 模型的高并发网络服务器，支持处理 `HTTP `协议请求，会话层通过AES(Cbc256)对称加密，RSA签名(Sha224)和校验实现非对称加密。主 `Reactor` 负责监听并接收新连接，从 `Reactor` 负责具体的读写事件处理，支持`select, pool, epoll (ET)`三种IO复用与`EventLoop`机制增强高并发与吞吐能力。
客户端使用`select`进行超时的IO复用，全局数据管理器和任务队列单例负责数据共享与通信，通过线程池管理通信线程，抢地主线程和出牌线程。

***

#### 项目结构

```zsh
fight_the_landlord_server
├── CMakeLists.txt
├── README.md
├── bin
├── build                     
├── common                    
├── config
├── create_tables.sql
├── crypto
├── database
├── doc
├── game
├── http
├── main.cpp
├── reactor
├── serialize
├── tcp
└── thread
```

项目细节请看[doc/process.md](https://github.com/wen2ms/fight_the_landlord_server/blob/main/doc/process.md)中，当前整理的Bug在[doc/bugfix.md](https://github.com/wen2ms/fight_the_landlord_server/blob/main/doc/bugfix.md)中。 

***

#### 后续目标

使用`perf`对程序进行性能分析，并使用`benchmark`来测试高并发服务器性能。
