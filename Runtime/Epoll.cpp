#include "Epoll.hpp"
#include "Logger.hpp"

using namespace Runtime;

static int epoll_wait_event(int epfd, struct epoll_event *re, size_t max_count, int timeout){
    int nfds;
restart:
    nfds=epoll_wait(epfd, re, max_count, timeout);
    if (nfds==-1){
        if (errno == EINTR){
            goto restart;
        }
        SYSERROR();
    }

    return nfds;
}




Epoll::Epoll(EpollFlag flag)
    :flag_(flag)
{
    epfd_=epoll_create1((int)flag_);
    if (epfd_<0){
        SYSERROR();
    }
}

Epoll::~Epoll()
{
    if (epfd_!=-1){
        close(epfd_);
    }
    epfd_ = -1;
}


void Epoll::Add(Pollable *data, int e){
    struct epoll_event event;
    event.events = e;
    event.data.ptr = data;

    if(epoll_ctl(epfd_,EPOLL_CTL_ADD,data->GetFD(),&event)<0){
        SYSERROR();
    }

    event_buffer_.push_back(event);
}

void Epoll::Del(Pollable *data)
{
    // for portable to before 2.6.9. See `man 2 epoll_ctl` BUGS section.
    struct epoll_event event;
    if(epoll_ctl(epfd_,EPOLL_CTL_DEL,data->GetFD(),&event)<0)
    {
        //XXX: fd may be not a valid fild descriptor!
        if(errno!=EBADF)
        {
            SYSERROR();
        }
    }

    for(auto it = event_buffer_.begin(); it!=event_buffer_.end(); ++it){
        if (it->data.ptr == data){
            event_buffer_.erase(it);
            break;
        }
    }
}

void Epoll::mod_event(Pollable *data, int e) {
    struct epoll_event event;
    event.events = e;
    event.data.ptr = data;
    if(epoll_ctl(epfd_, EPOLL_CTL_MOD, data->GetFD(), &event)<0){
        SYSERROR();
    }
}

void Epoll::set_event(Pollable *data, EventFunc &event_func) {
    for (auto it=event_buffer_.begin(); it!=event_buffer_.end(); ++it){
        if (it->data.ptr==data) {
            int e = event_func(it->events);
            mod_event(data, e);
            it->events = e;
            return;
        }
    }

    SYSERROR1(ENOENT);
}


void Epoll::DelEvent(Pollable *data, int e){
    EventDel d(e);
    set_event(data, d);
}

void Epoll::AddEvent(Pollable *data, int e){
    EventAdd a(e);
    set_event(data, a);
}

void Epoll::SetEvent(Pollable *data, int e){
    EventFunc a(e);
    set_event(data, a);
}

std::vector<PollEvent> Epoll::Wait(int timeout) {
    size_t max_count = event_buffer_.size();
    std::vector<struct epoll_event> re;
    re.resize(max_count);

    int nfds =epoll_wait_event(epfd_, &re[0], max_count, timeout);
    
    std::vector<PollEvent> revents;
    revents.resize(nfds);
    for(auto i=0;i<nfds; ++i){
        revents[i].data = (Pollable *)re[i].data.ptr;
        revents[i].events = re[i].events;
    }
    return revents;
}

void Epoll::Loop(int timeout){
    int nfds;
    size_t max_count;
    std::vector<struct epoll_event> re;
    Pollable *data=NULL;
    int event;

    log_debug("IN:%p,OUT:%p, RH:%p , ERR:%p\n",Event::POLLIN,Event::POLLOUT,Event::POLLRDHUP , Event::POLLERR|Event::POLLHUP);
    while (true){
        max_count = event_buffer_.size();
        re.resize(max_count);

        nfds = epoll_wait_event(epfd_, &re[0], max_count, timeout);
        for (int i = 0; i< nfds; ++i){
            data=(Pollable *)(re[i].data.ptr);
            event=re[i].events;

            /*
             * 在发生错误事件时有以下情况：
             * 1. 连接发生错误   ==> 应该close socket
             * 2. 对端关闭连接   ==> 应该close socket
             * 3. 对端关闭连接写或读    ==> 由于无法判断是half-close还是close，所以直接close。
             *
             * OnReadable 中调用read函数返回0                 ==> 连接不可读。
             * OnWriteable 中write函数返回-1，errno==EPIPE    ==> 连接不可写
             */

            try{
                log_debug("Event:%p, data:%p\n",event,data);
                if (event & (Event::POLLERR | Event::POLLHUP)){
                    event |=Event::POLLIN ;
                }
                if (event & (Event::POLLRDHUP)){
                    event |= Event::POLLOUT;
                }
                if ((event & Event::POLLIN )) {
                    data->OnReadable(*this);
                    log_debug("IN DONE\n");
                }
                if ((event & Event::POLLOUT)) {
                    log_debug("Out begin \n");
                    data->OnWriteable(*this);
                }

            }catch(SysException &e){
                data->OnError(*this);
            }catch(EOFException &e){
                data->OnError(*this);
            }
        }
    }
}

#ifdef __EVENT_UNITTEST
Logger log;
int main(){

    Epoll poller;

    std::vector<PollEvent> vec = poller.Wait();

    return 0;
}

#endif // __EVENT_UNITTEST
