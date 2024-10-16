#include "net/Socket.h"
#include "util/alluse.h"
int main(int argc,char* argv[])
{
    glog_init(argv[0]);
    Socket client;
    client.client_init();
    client.connect(3333,"127.0.0.1");
    std::string msg;
    client.block_write("hello I`m client!");
    client.block_read(msg);
    std::cout << msg << std::endl;
    google::ShutdownGoogleLogging();
}