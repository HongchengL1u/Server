extern "C"
{
    #include <unistd.h>
    #include <string.h>
}

#include <iostream>
#include <functional>
#include <memory>
#include "net/Socket.h"
#include "ThreadPool/ThreadPool.h"
#include "util/alluse.h"
void oneClient(int msgs, int wait)
{
    Socket client;
    client.client_init();
    client.connect(3333,"127.0.0.1");
    int sockfd = client.get_fd();
    std::string readbuffer;
    std::string sendbuffer;
    // std::cout << "wait:" << wait << std::endl;
    // sleep(wait);
    int count = 0;
    while(count < msgs)
    {
        readbuffer.clear();
        sendbuffer.clear();
        sendbuffer.append("I'm client! "+std::to_string(sockfd));
        client.write(sendbuffer);
        client.read(readbuffer);
        LOG_INFO << "recv: " << readbuffer;
        count++;
    }   
}


int main(int argc, char *argv[]) {
    glog_init(argv[0]);
    int threads = 100;
    int msgs = 100;
    double wait = 0;
    int o;
    const char *optstring = "t:m:w:";
    while ((o = getopt(argc, argv, optstring)) != -1) {
        switch (o) {
            case 't':
                threads = std::stoi(optarg);
                break;
            case 'm':
                msgs = std::stoi(optarg);
                break;
            case 'w':
                wait = std::stod(optarg);
                break;
            case '?':
                printf("error optopt: %c\n", optopt);
                printf("error opterr: %d\n", opterr);
                break;
        }
    }
    printf("%d %d %f\n",threads,msgs,wait);
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(10);
    std::function<void()> func = std::bind(oneClient, msgs, wait);
    for(int i = 0; i < threads; ++i){
        pool->addfunc(func);
    }
    google::ShutdownGoogleLogging();
    return 0;
}
