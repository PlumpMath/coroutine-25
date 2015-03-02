#ifndef __CHANNEL_HPP
#define __CHANNEL_HPP

#include "Task.hpp"
#include "Logger.hpp"
#include <utility>
#include "Select.hpp"
#include <deque>

namespace Runtime{
    typedef int ChannelId;
    //static std::atomic<int>  gen_channel_id;

    template<typename T> class Select;
    template<typename T>
    class Channel{
        typedef Select<T> Selector;
        typedef std::deque<T> ChannelQ;
        typedef std::deque<Selector *> WaitQ;
    public:
        friend Selector;

        Channel(ChannelId id = -1)
            :id_(id)
        {

        }

        Channel &operator<<(const T &t){
            Put(t);
            return *this;
        }

        bool Cancel(Selector *const &slt){
            std::lock_guard<std::mutex> _(ch_lock_);
            for(auto it = recvq_.begin(); it!=recvq_.end(); ++it){
                if (*it == slt){
                    recvq_.erase(it);
                    return true;
                }
            }
            return false;
        }

        inline void Put(const T &t){
            std::lock_guard<std::mutex> _(ch_lock_);

            Selector *selector;
            while(true){
                if (recvq_.empty()){
                    q_.push_back(t);
                    return;
                }

                selector = recvq_.front();
                recvq_.pop_front();

                // lock to ensure only wakeup once.
                if (selector->Return(t)){
                    return;
                }
            }
        }

        ChannelId ID() const{
            return id_;
        }
        
        void SelectNoLock(Select<T> *selector){
            if (!q_.empty()){
                selector->FastReturn(q_.front());
                q_.pop_front();
            }
        }

        T Get(){
            Selector s(*this);
            s.Wait();
            return s.v;
        }

    private:
        ChannelId id_;

        ChannelQ q_;

        std::mutex ch_lock_;

        WaitQ recvq_;

        /* 
         * TODO: support block send.
        WaitQ sendq_;
        */
    };
}


#endif// __CHANNEL_HPP
