/*
 * Author      : zzn
 * Created     : 2013-12-18 16:49:25
 * Modified    : 2013-12-18 16:49:25
 * Description : 
 * License: GNU Public License.
*/

#ifndef __RUNTIME_EPOLL_HPP
#define __RUNTIME_EPOLL_HPP

#include <sys/epoll.h>
#include "Exception.hpp"
#include <unistd.h>
#include <vector>
#include "Poller.hpp"

namespace Runtime
{
    struct Event{
        static const int POLLIN = EPOLLIN;       //There is data to read.
        static const int POLLPRI = EPOLLPRI;     //There is urgent data to read (e.g., out-of-band data on TCP socket; pseudo-terminal master in packet mode has seen state change in slave).
        static const int POLLOUT = EPOLLOUT;     //Writing now will not block.
        static const int POLLRDHUP = EPOLLRDHUP; //Stream socket peer closed connection, or shut down writing half of connection. The _GNU_SOURCE feature test macro must be defined in order to obtain this definition.
        static const int POLLERR = EPOLLERR;     //Error condition (output only).
        static const int POLLHUP = EPOLLHUP;     //Hang up (output only).
        static const int POLLNVAL = EPOLLERR;   //Invalid request: fd not open (output only).
        static const int POLLET = EPOLLET;
    };

    enum class EpollFlag{
        EF_DEFAULT = 0,
        EF_CLOEXEC = EPOLL_CLOEXEC,
    };

    typedef std::vector<struct epoll_event> EventBuffer;

    class Epoll:public Poller{
    public:
        Epoll(EpollFlag flag = EpollFlag::EF_DEFAULT);   
        ~Epoll();
        std::vector<PollEvent> Wait(int timeout = -1) override;
        void Loop(int timeout = -1) override;
        void Add(Pollable *, int e) override;
        void Del(Pollable *) override;

        void DelEvent(Pollable *, int e) override;
        void AddEvent(Pollable *, int e) override;

        void SetEvent(Pollable *, int e) override;

    private:
        void mod_event(Pollable *, int e);
        void set_event(Pollable *, EventFunc &event_func);
    private:
        EventBuffer  event_buffer_;
        int epfd_;
        EpollFlag flag_;
    };
}

#endif //__RUNTIME_EPOLL_HPP
