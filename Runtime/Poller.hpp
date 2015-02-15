/*
 * Author      : zzn
 * Created     : 2013-12-18 16:49:25
 * Modified    : 2013-12-18 16:49:25
 * Description : 
 * License: GNU Public License.
*/

#ifndef __RUNTIME_POLLER_HPP
#define __RUNTIME_POLLER_HPP

#include "Exception.hpp"
#include <unistd.h>
#include <vector>

namespace Runtime
{

    struct Event;

    class Pollable;

    struct PollEvent{
        uint32_t events;
        Pollable *data;
    };

    class Poller{
    public:
        virtual std::vector<PollEvent> Wait(int timeout = -1) = 0;
        virtual void Loop(int timeout = -1) = 0;
        virtual void Add(Pollable *, int e)=0;
        virtual void Del(Pollable *)=0;

        virtual void DelEvent(Pollable *, int e)=0;
        virtual void AddEvent(Pollable *, int e)=0;

        virtual void SetEvent(Pollable *, int e)=0;
    };

    class Pollable{
    public:
        virtual ~Pollable(){
        }

        virtual void OnReadable(Poller &)=0;
        virtual void OnWriteable(Poller &)=0;
        virtual void OnError(Poller &)=0;

        virtual int GetFD() const = 0;
    };



    struct EventFunc{
        EventFunc(int e){
            old_event = e;
        }
        ~EventFunc(){
        }

        virtual int operator()(int event){
            return event;
        }

        int old_event;
    };

    struct EventDel:public EventFunc{
        EventDel(int e):EventFunc(e){
        }
        int operator()(int event){
            return old_event & (~event);
        }
    };
    struct EventAdd:public EventFunc{
        EventAdd(int e):EventFunc(e){
        }
        int operator()(int event){
            return old_event | (event);
        }
    };
}

#endif //__RUNTIME_POLLER_HPP
