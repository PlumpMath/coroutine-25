#ifndef __RUNTIME_CHANNEL_HPP
#define __RUNTIME_CHANNEL_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include "Exception.hpp"

namespace Runtime{
    class EmptyError:public Exception{
    public:
        EmptyError(const char *func,int line)
            :Exception(func,line,"Empty Queue")
        {
        }
    };

    template<typename QueueType>
    class BlockQueue{
        typedef typename QueueType::value_type T;
    public:
        BlockQueue(){
            wait_ = 0;
        }

        virtual ~BlockQueue(){

        }

        /*
         * 调用任何会切换上下文的函数会导致死锁，即不能调用Task::Yield()
         * 因为Yield之前没有unlock，此时push会阻塞在lock上。
         */
        virtual inline void OnEmpty(){

        }

        T PopFront()noexcept{
            std::unique_lock<std::mutex> lk(lock_);
            if(msg_queue_.empty()){
                OnEmpty();
            }
            ++wait_;
            while (msg_queue_.empty()){
                cond_.wait(lk);
            }
            --wait_;

            T t = msg_queue_.front();
            msg_queue_.pop();
            return t;
        }

        size_t Size() const{
            std::lock_guard<std::mutex> _(lock_);
            return msg_queue_.size();
        }

        void Push(const T &s) noexcept{
            std::lock_guard<std::mutex> _(lock_);
            msg_queue_.push(s);
            if (wait_>0){
                cond_.notify_one();
            }
        }

    private: 
        QueueType msg_queue_;
        mutable std::mutex lock_;
        mutable std::condition_variable cond_;
        size_t wait_;
    };

    template<typename QueueType>
    class NonBlockQueue{
        typedef typename QueueType::value_type T;
    public:
        NonBlockQueue(){
        }

        ~NonBlockQueue(){
        }

        T PopFront(T default_value)noexcept{
            std::lock_guard<std::mutex> _(lock_);
            if (msg_queue_.empty()){
                return default_value;
            }

            T t = msg_queue_.front();
            msg_queue_.pop();
            return t;
        }

        T PopFront(){
            std::lock_guard<std::mutex> _(lock_);
            if (msg_queue_.empty()){
                THROW(EmptyError);
            }

            T t = msg_queue_.front();
            msg_queue_.pop();
            return t;
        }

        bool Empty()const{
            std::lock_guard<std::mutex> _(lock_);
            return msg_queue_.empty();
        }
        size_t Size()const{
            std::lock_guard<std::mutex> _(lock_);
            return msg_queue_.size();
        }

        void Push(const T &s)noexcept{
            std::lock_guard<std::mutex> _(lock_);
            msg_queue_.push(s);
        }

    private: 
        QueueType msg_queue_;
        mutable std::mutex lock_;
    };

    template<typename T>
    struct Queue{
        typedef BlockQueue<std::queue<T>> BlockQ;
        typedef NonBlockQueue<std::queue<T>> NonBlockQ;
    };
}

#endif// __RUNTIME_CHANNEL_HPP
