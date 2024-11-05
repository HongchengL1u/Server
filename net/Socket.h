#ifndef SERVER_NET_SOCKET_H
#define SERVER_NET_SOCKET_H


extern "C"
{
    #include <sys/types.h>          
    #include <sys/socket.h>
    #include <string.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
}

#include "util/alluse.h"
#include <string>
#include <iostream>

class Socket
{
    public:
        enum TYPE
        {
            CLIENT,
            SERVER,
            UNSEPECIFIED
        };
        ~Socket()
        {
            // 析构函数会在栈展开时运行，此时出现异常，不会被捕获
            // 但是当存在异常时，析构函数也出现异常，此时会直接非正常退出
            // 析构函数应该设计为不抛出异常
            // shutdown(socketfd,SHUT_RDWR);
            // man 2 shutdown
            // 使用close中止一个连接，但它只是减少描述符的参考数，并不直接关闭连接，只有当描述符的参考数为0时才关闭连接。
            // shutdown可直接关闭描述符，不考虑描述符的参考数，可选择中止一个方向的连接。
            LOG_INFO << "Socket free! fd: " << fd;
            close(fd);
            fd = -1;
        }
        int get_fd()
        {
            return fd;
        }
        void set_nonblock()
        {
            // https://blog.csdn.net/zxpoiu/article/details/115793012
            int flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            nonblock = true;
        }
        bool client_init()
        {
            type = TYPE::CLIENT;
            fd = socket(AF_INET, SOCK_STREAM, 0);
            if(fd == -1)
            {
                LOG_ERROR << "socket create fail!";
                return false;
            } 
            return true;
        }
        bool client_init(int _fd)
        {
            if(_fd == -1)
            {
                LOG_ERROR << "Socket init fail!";
                return false;
            }
            type = TYPE::CLIENT;
            fd = _fd;
            return true;
        }    
        bool server_init()
        {
            type = TYPE::SERVER;
            fd = socket(AF_INET, SOCK_STREAM, 0);
            if(fd == -1)
            {
                LOG_ERROR << "socket create fail!";
                return false;
            } 
            return true;
        }
        bool bind(int port,std::string ip)
        {
            if(fd == -1) return false;
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(sockaddr_in));
            addr.sin_port = htons(port);
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr= inet_addr(ip.c_str());
            if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &addr, sizeof(addr)) == -1)
            {
                LOG_ERROR << "server socket bind failed!";
            };
            int bind_flag = ::bind(fd,(sockaddr *)(&addr),sizeof(sockaddr));
            if(bind_flag == -1) 
            {
                LOG_ERROR << "socket bind fail!";
                return false;
            }
            return true;
        }
        bool connect(int port, std::string ip)
        {
            if(fd == -1) return false;
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_port = htons(port);
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr= inet_addr(ip.c_str());
            int connect_flag = ::connect(fd,(sockaddr *)(&addr),sizeof(sockaddr));
            if(connect_flag == -1)
            {
                LOG_ERROR << "connect fail!";
                return false;
            }
            return true;
        }

        bool listen(int MAX_LISTEN_NUM = 1024)
        {
            if(fd == -1) return false;
            if(::listen(fd,MAX_LISTEN_NUM) == -1)
            {
                LOG_ERROR << "listen error!";
                return false;
            }
            return true;
        }
        int accept()
        {
            if(nonblock) return nonblock_accept();
            else return block_accept();
        }
        int nonblock_accept()
        {
            if(fd==-1) return false;
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            socklen_t addr_len = sizeof(addr);
            int client_fd = ::accept(fd,(sockaddr *)(&addr),&addr_len);
            if(client_fd == -1)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return 0;
                }
                else
                {
                    LOG_ERROR << "accept fail!";
                    return -1;
                }
            }
            char* client_ip = inet_ntoa(addr.sin_addr);
            int client_port = ntohs(addr.sin_port);
            printf("connect from %s:%d\n",client_ip,client_port);        
            return client_fd;
        }
        int block_accept()
        {
            if(fd==-1) return false;
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            socklen_t addr_len = sizeof(addr);
            int client_fd = ::accept(fd,(sockaddr *)(&addr),&addr_len);
            if(client_fd == -1)
            {
                LOG_ERROR << "accept fail!";
                return -1;
            }
            char* client_ip = inet_ntoa(addr.sin_addr);
            int client_port = ntohs(addr.sin_port);
            printf("connect from %s:%d\n",client_ip,client_port);        
            return client_fd;
        }
        bool write(std::string& msg)
        {
            if(nonblock) return nonblock_write(msg);
            else return block_write(msg);
        }
        bool read(std::string& msg)
        {
            if(nonblock) return nonblock_read(msg);
            else return block_read(msg);
        }
        bool block_write(const std::string& msg)
        {
            if(fd == -1) return false;
            int write_size = ::write(fd, msg.c_str(), msg.size());
            if(write_size != msg.size())
            {
                LOG_ERROR << "send fail! send num: " << write_size;
                return false;
            }
            return true;
        }
        bool block_read(std::string& msg)
        {
            if(fd==-1) return false;
            char buf[1024];
            int read_size = ::read(fd, buf, sizeof(buf));
            if(read_size==-1) return false;
            msg.append(buf,read_size);
            return true;
        }
        bool nonblock_write(const std::string& msg)
        {
            if(fd==-1) return false;
            int bufsize = 1024;
            const char* msg_c = msg.c_str();
            int index = 0;
            char buf[bufsize];
            
            while(true)
            {
                bzero(&buf, sizeof(buf));
                const int transfer_size = std::min(sizeof(buf),msg.size()-index);
                memcpy(buf,msg_c,transfer_size);
                index+=transfer_size;
                int bytes_write = ::write(fd,buf,transfer_size);
                if(bytes_write>0)
                {

                }
                else if(bytes_write == 0)
                {
                    //EOF，客户端断开连接
                    LOG_ERROR << "EOF, client fd " << fd << " disconnected!";
                    return false;
                }
                else
                {
                    if(errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        if(index == msg.size())
                        {
                            // 表示读取完毕
                            LOG_INFO << "send over!";
                            break;
                        }
                        else
                        {
                            continue;
                            continue;
                        }
                        continue;
                        continue;
                    }
                    else if(errno == EINTR)
                    {
                        continue;
                    }
                    else
                    {
                        LOG_ERROR << "send process wrong!";
                        return false;
                    }
                }
            }
            return true;
        }
        bool nonblock_read(std::string& msg)
        {
            if(fd==-1) return false;
            int bufsize = 1024;
            char buf[bufsize];
            std::string& big_buffer = msg;
            while(true)
            {   
                bzero(&buf, sizeof(buf)); 
                //由于使用非阻塞IO，读取客户端buffer，一次读取buf大小数据，直到全部读取完毕
                int bytes_read = ::read(fd, buf, sizeof(buf));
                // printf("bytes_read: %d\n",bytes_read);
                if(bytes_read > 0)
                {
                    big_buffer.append(buf,bytes_read);                
                } 
                else if(bytes_read == 0)
                {
                    //EOF，客户端断开连接
                    LOG_ERROR << "EOF, client fd " << fd << " disconnected!";
                    return false;
                }
                else if(bytes_read == -1)
                {
                    if(errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        if(big_buffer.size()>0)
                        {
                            // 表示读取完毕
                            // write(big_buffer);
                            break;
                            break;
                        }
                        continue;
                        
                        continue;
                        
                    }
                    else if(errno == EINTR)
                    {
                        continue;
                    }
                    else
                    {
                        LOG_ERROR << "recv process wrong!";
                        return false;
                    }
                } 
            }
            return true;
        }
    private:
        TYPE type = TYPE::UNSEPECIFIED;
        int fd;
        bool nonblock = false;
};

#endif