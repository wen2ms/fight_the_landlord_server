#include "tcp_server.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

#include "log.h"
#include "rsacrypto.h"
#include "tcp_connection.h"
#include "room.h"

TcpServer::TcpServer(unsigned short port, int num_threads)
    : main_loop_(new EventLoop), num_threads_(num_threads), port_(port), thread_pool_(new ThreadPool(main_loop_, num_threads)) {
    set_listen();
}

void TcpServer::set_listen() {
    lfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd_ == -1) {
        perror("socket");
        return;
    }
    int opt = 1;
    int ret = setsockopt(lfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1) {
        perror("setsockopt");
        return;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        perror("bind");
        return;
    }
    ret = listen(lfd_, 128);
    if (ret == -1) {
        perror("listen");
        return;
    }
}

void TcpServer::run() {
    DEBUG("Server started...");

    save_rsa_key();

    thread_pool_->run();

    Channel* channel = new Channel(lfd_, FDEvent::kReadEvent, accept_connection, nullptr, nullptr, this);

    main_loop_->add_task(channel, ElemType::kAdd);
    main_loop_->run();
}

int TcpServer::accept_connection(void* arg) {
    TcpServer* server = static_cast<TcpServer*>(arg);

    int cfd = accept(server->lfd_, NULL, NULL);
    EventLoop* ev_loop = server->thread_pool_->take_worker_event_loop();
    TcpConnection* conn = new TcpConnection(cfd, ev_loop);

    return 0;
}

void TcpServer::save_rsa_key() {
    RsaCrypto* rsa = new RsaCrypto;

    rsa->generate_rsa_key(RsaCrypto::kBits2k);
    delete rsa;

    std::ifstream infile("public.pem");
    std::stringstream line_stream;

    line_stream << infile.rdbuf();

    std::string data = line_stream.str();

    infile.close();

    Room redis;

    assert(redis.init_environment());
    redis.clear();
    redis.save_rsa_key("public_key", data);

    infile.open("private.pem");
    line_stream << infile.rdbuf();
    data = line_stream.str();
    redis.save_rsa_key("private_key", data);

    unlink("public.pem");
    unlink("private.pem");
}
