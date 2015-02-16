#ifndef __SERVER_HPP
#define __SERVER_HPP

#include "TCPServer.hpp"
#include <arpa/inet.h>

#include "Protocol.hpp"
#include "Channel.hpp"
#include "Task.hpp"
#include "Queue.hpp"
#include <unordered_map>
#include "Timer.hpp"

namespace Runtime{

    std::unordered_map<ChannelId, void *> ChannelMap;

    class TCPService{
    public:
        TCPService(Poller &p, short port, const char *ip=NULL);
        ~TCPService();

    public:
        static void ParseMsg(Runtime::TCPConn &conn, ReadBuf &buf) ;

    private:
        TCPServer tcp_server_;
        Poller  &poller_;
    };

    class TimerService{
    public:
        TimerService(Poller &p);
        ~TimerService();

        void Sleep(time_t sec, long msec = 0);
    public:
        static void OnTimeout (const TimeSpec &, Task *const &t);
    private:
        Timer<Task *> timer_;
        Poller  &poller_;
    };

    class Server{
    public:
        Server(short port, const char *ip=NULL);
        ~Server();
        void Loop();

        void Sleep(time_t sec, long msec = 0){
            timer_serv_.Sleep(sec, msec);
        }

    private:
        Epoll  poller_;
        TCPService tcp_serv_;
        TimerService timer_serv_;

    public:
        static Scheduler scheduler;
    };

}

#endif // __SERVER_HPP
