#include "dispatcher.h"

Dispatcher::Dispatcher(EventLoop* ev_loop) : ev_loop_(ev_loop), name_(std::string()) {}

Dispatcher::~Dispatcher() {}

int Dispatcher::add() {
    return 0;
}

int Dispatcher::remove() {
    return 0;
}

int Dispatcher::modify() {
    return 0;
}

int Dispatcher::dispatch(int timeout) {
    return 0;
}
