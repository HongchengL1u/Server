#ifndef SERVER_NET_EPOLL_H
#define SERVER_NET_EPOLL_H
extern "C"
{
    #include <sys/epoll.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/types.h>          /* See NOTES */
    #include <sys/socket.h>
}
#include <iostream>
#include <functional>
#include "Socket.h"
#include <unordered_map>
#include <memory>
#include "ThreadPool/ThreadPool.h"
#include "util/alluse.h"
class Event
{
    public:
        Event()=default;
        ~Event()=default; 
        int get_fd() const
        {
            return fd;
        }
        std::function<void()>& get_func()
        {
            return func;
        }
        void bind(int _fd, std::function<void()> _func)
        {
            fd = _fd;
            func = _func;
        }
    private:
        int fd;
        std::function<void()> func;
};
class Epoll
{
    public:
        Epoll()
        {
            memset(&ev,0,sizeof(ev));
            events = new struct epoll_event[kMaxEventNums];
        }
        ~Epoll()
        {
            close(event_fd);
            delete[] events;
        }
        void make_new_connection(Socket& listen_socket)
        {
            // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
            while(1)
            {
                int connect_fd = listen_socket.accept();
                if(connect_fd<=0) break;
                LOG_INFO << "connect_fd: " << connect_fd;
                std::shared_ptr<Socket> connect_socket_ptr = std::make_shared<Socket>();
                connect_socket_ptr->client_init(connect_fd);
                connect_socket_ptr->set_nonblock();
                struct epoll_event new_ev;
                Event msg_process_event;
                auto func = [this, connect_socket_ptr]()
                {
                    // 传递智能指针，最好使用lambda函数
                    // 传递智能指针时最好不使用引用，因为这样增加不了计数
                    this->msg_callback(connect_socket_ptr);
                };
                msg_process_event.bind(connect_fd,func);
                // ET模式时，事件就绪时，假设对事件没做处理，内核不会反复通知事件就绪
                new_ev.events = EPOLLET | EPOLLIN;
                // EPOLLOUT 缓冲区可读
                map[connect_fd] = msg_process_event;
                new_ev.data.ptr = &(map[connect_fd]);
                // ev如果被挂载到epfd上，则其上的内容是会被保存的
                if(epoll_ctl(event_fd,EPOLL_CTL_ADD,connect_fd,&new_ev)==-1)
                {
                    LOG_ERROR << "epoll create fail!";
                }
            }
        }
        void msg_callback(std::shared_ptr<Socket> connect_socket)
        {
            LOG_INFO << "process msg!";
            std::string msg;
            if(connect_socket->read(msg))
            {
                LOG_INFO << "recv msg: "<< msg;
                connect_socket->write(msg);
            } 
            else
            {
                // 多线程要及时关闭，以免出现过多close_wait
                // 由于设计的智能指针存在，在函数执行完之后会自动析构
                if(epoll_ctl(event_fd,EPOLL_CTL_DEL,connect_socket->get_fd(),nullptr)==-1)
                {
                    LOG_ERROR << "epoll delete fail!";
                }
                map.erase(connect_socket->get_fd());
            }
        }   
        bool create(Socket& listen_socket)
        {
            listen_socket.set_nonblock();
            event_fd = epoll_create(kMaxEventNums);
            if(event_fd == -1)
            {
                LOG_ERROR << "epoll create fail!";
                return false;
            }
            int listen_sock_fd = listen_socket.get_fd();
            Event accept_event;
            auto func = [this,&listen_socket]()
            {
                this->make_new_connection(listen_socket);
            };
            accept_event.bind(listen_sock_fd, func);
            // 文件描述符可读
            // ET模式时，事件就绪时，假设对事件没做处理，内核不会反复通知事件就绪
            ev.events = EPOLLIN | EPOLLET; 
            map[listen_sock_fd] = accept_event;
            ev.data.ptr = &(map[listen_sock_fd]);
            listen_fd = listen_sock_fd;
            if(epoll_ctl(event_fd,EPOLL_CTL_ADD,listen_sock_fd,&ev)==-1)
            {
                LOG_ERROR << "epoll create fail!";
                return false;
            }
            return true;
        }
        bool loop()
        {
            ThreadPool pool(10);
            while(1)
            {
                
                
                int event_nums = epoll_wait(event_fd,events,kMaxEventNums,-1);
                LOG_INFO << "Thread pool left: " << pool.size();
                if(event_nums == -1)
                {
                    LOG_ERROR << "epoll wait fail!";
                    return false;
                }
                else
                {
                    for(int i=0;i<event_nums;i++)
                    {
                        LOG_INFO << "event process begin!";
                        // int pre_num = map.size();
                        int ev_fd = ((Event*)(events[i].data.ptr))->get_fd();
                        // if(ev_fd != listen_fd) pool.addfunc(((Event*)(events[i].data.ptr))->get_func());
                        // else (((Event*)(events[i].data.ptr))->get_func())();
                        // std::cout << "event_size from " << pre_num << " to " << map.size() << std::endl;
                        pool.addfunc(((Event*)(events[i].data.ptr))->get_func());
                        LOG_INFO << "event process end! fd: " << ev_fd;
                    }
                }
            }
            return true;
        }
    private:
        const int kMaxEventNums = 1024;
        struct epoll_event ev;
        struct epoll_event* events;
        int event_fd = -1;
        int listen_fd = -1;
        std::unordered_map<int,Event> map;
};

#endif