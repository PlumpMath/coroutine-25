#ifndef  __RUNTIME_SELECT_HPP
#define  __RUNTIME_SELECT_HPP

#include <mutex>
#include <algorithm>
#include <vector>
#include "Channel.hpp"
#include "Task.hpp"
#include "Timer.hpp"

namespace Runtime{

    enum class SelectState{
        NEW,
        INPROGRESS,
        TIMEOUT,
        RETURN,
    };

    template<typename T> class Channel;

    template<typename T>
    class Select{
        typedef Channel<T> CH;
    public:
        static bool chan_cmp(const CH *c1, const CH *c2){
            return c1->ID() > c2->ID();
        }

    public:
        template<typename...Args>
        Select(CH &c1, Args&...c)
            : Select(c...)
        {
            chans_.push_back(&c1);
        }

        Select(CH &c1){
            chans_.push_back(&c1);
            std::sort(chans_.begin(), chans_.end(), Select::chan_cmp);
            state = SelectState::NEW;
        }

        ~Select(){

        }

        // return -1 if timeout.
        int Wait(const struct TimeSpec *timeout=NULL){
            this_task_ = CPU::current_core->running_task;
            state = SelectState::INPROGRESS;

            for (auto it = chans_.begin(); it != chans_.end(); ++it){
                (*it)->ch_lock_.lock();

                (*it)->SelectNoLock(this);

                // select is done.
                if (state == SelectState::RETURN){
                    //++it;
                    for (auto start = chans_.begin(); start <= it; ++start){
                        (*start)->ch_lock_.unlock();
                    }

                    return 0;
                }
            }

            for (auto it = chans_.begin(); it != chans_.end(); ++it){
                (*it)->recvq_.push_back(this);
                (*it)->ch_lock_.unlock();
            }

            Yield();

            // 如果select对象已释放，此处会导致coredump
            for (auto it = chans_.begin(); it != chans_.end(); ++it){
                (*it)->Cancel(this);
            }

            return 0;
        }

        // return after yield.
        bool Return(const T &t) noexcept{
            std::lock_guard<std::mutex> _(return_lock_);

            if (state == SelectState::INPROGRESS){
                FastReturn(t);
                try{
                    this_task_->SetReady();
                }catch(TaskError &e){
                    // must reset state. 
                    state = SelectState::INPROGRESS;

                    log_warn("%s\n",e.what());
                    return false;
                }
                return true;
            }

            return false;
        }

        // return immediately, no yield.
        void FastReturn(const T &t) {
            v = t;
            state = SelectState::RETURN;
        }

    private:
        std::mutex return_lock_;
        SelectState state;
        std::vector<CH*> chans_;
        Task *this_task_;
    public:
        T v;
    };
}

#endif//  __RUNTIME_SELECT_HPP


