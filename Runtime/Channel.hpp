#ifndef __CHANNEL_HPP
#define __CHANNEL_HPP

#include "Queue.hpp"
#include "Task.hpp"
#include "Logger.hpp"

namespace Runtime{
    typedef int ChannelId;
    //static std::atomic<int>  gen_channel_id;

    template<typename T>
    class Channel{
        typedef typename Queue<T>::NonBlockQ ChannelQ;
    public:
        Channel(ChannelId id = -1)
            :id_(id)
        {

        }

        Channel &operator<<(const T &t){
            Put(t);
            return *this;
        }

        inline void Put(const T &t){
            std::lock_guard<std::mutex> _(ch_lock_);
            Task *task = waitq_.PopFront(NULL);
            if (task != NULL){
                returnq_.Push(t);
                task->SetReady(); 
            }else{
                q_.Push(t);
            }
        }

        ChannelId ID() const{
            return id_;
        }

        T Get(){
            ch_lock_.lock();
            if (q_.Empty()){
                Task * task = CPU::current_core->running_task;
                task->State = TaskState::WAITING;
                waitq_.Push(task);
                ch_lock_.unlock();

                Task::Yield2();

                // returnq_ 已经是thread-safe，不需要ch_lock_
                return returnq_.PopFront();
            }else{
                T t = q_.PopFront();
                ch_lock_.unlock();
                return t;
            }
        }

    private:
        ChannelId id_;

        ChannelQ q_;

        ChannelQ returnq_;
        // 用于确保q_.Empty和waitq_.Push的一致性。
        std::mutex ch_lock_;

        Queue<Task *>::NonBlockQ waitq_;
    };
}


#endif// __CHANNEL_HPP
