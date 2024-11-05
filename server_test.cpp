#include "Socket.h"

int main()
{
    Socket server;
    server.server_init();
    server.bind(3333,"127.0.0.1");
    server.listen(5);
    int connect_fd = server.accept();
    Socket connect_1;
    connect_1.client_init(connect_fd);
    connect_1.block_write("hello I`m Server!");
    std::string msg;
    connect_1.block_read(msg);
    std::cout << msg << std::endl;
}