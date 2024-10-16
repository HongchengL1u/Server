#include "net/Epoll.h"
#include "net/Socket.h"
#include "util/alluse.h"
int main(int argc, char* argv[])
{
    glog_init(argv[0]);
    Socket server;
    server.server_init();
    server.bind(3333,"127.0.0.1");
    server.listen();
    Epoll epoll;
    epoll.create(server);
    epoll.loop();
    google::ShutdownGoogleLogging();
}