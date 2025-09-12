#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tcp_server.h"

int main(int argc, char* argv[]) {
    // if (argc < 3) {
    //     std::cout << "Usage: " << argv[0] << " port path" << std::endl;
    //     return -1;
    // }
    //
    // unsigned short port = atoi(argv[1]);
    //
    // chdir(argv[2]);
    //
    // TcpServer* server = new TcpServer(port, 4);
    unsigned short port = 9473;

    chdir("/home/ecs-user");

    TcpServer* server = new TcpServer(port, 4);

    server->run();

    return 0;
}