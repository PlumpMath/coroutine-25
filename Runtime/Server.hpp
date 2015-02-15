#ifndef __SERVER_HPP
#define __SERVER_HPP

#include "TCPServer.hpp"
#include <arpa/inet.h>

#include "Protocol.hpp"
#include "Channel.hpp"
#include "Task.hpp"
#include "Queue.hpp"
#include <unordered_map>

namespace Runtime{

    std::unordered_map<ChannelId, void *> ChannelMap;

    class Server{
    public:
        Server(short port, const char *ip=NULL);
        ~Server();
        void Loop();

    public:
        static void ParseMsg(Runtime::TCPConn &conn, ReadBuf &buf) ;

    private:
        TCPServer tcp_serv_;
        Epoll  poller_;

    public:
        static Scheduler scheduler;
    };


}

#endif // __SERVER_HPP
