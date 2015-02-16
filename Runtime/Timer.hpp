#ifndef __RUNTIME_TIMER_HPP
#define __RUNTIME_TIMER_HPP

#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <functional>
#include <queue>
#include "Epoll.hpp"
#include <mutex>
#include <unordered_map>
#include "Logger.hpp"

namespace Runtime{

    enum class ClockType{ REALTIME = CLOCK_REALTIME , MONOTONIC = CLOCK_MONOTONIC};
    typedef size_t TimerId;

    static inline void GetTime(ClockType clk_id, struct timespec *curr){
        if (clock_gettime((clockid_t)clk_id, curr) == -1){
            SYSERROR();
        }
    }

    class TimeSpec{
    public:
        bool operator<(const struct timespec &rh)const noexcept{
            return ((this->time_value.tv_sec < rh.tv_sec) ||
                    (this->time_value.tv_sec == rh.tv_sec && this->time_value.tv_nsec < rh.tv_nsec));
        }

        bool operator!=(const struct timespec &rh)const noexcept{
            return (!this->operator==(rh));
        }

        bool operator==(const struct timespec &rh)const noexcept{
            return (this->time_value.tv_sec == rh.tv_sec && this->time_value.tv_nsec == rh.tv_nsec);
        }

        bool operator>(const struct timespec &rh)const noexcept{
            return ((this->time_value.tv_sec > rh.tv_sec) ||
                    (this->time_value.tv_sec == rh.tv_sec && this->time_value.tv_nsec > rh.tv_nsec));
        }

        bool operator<(const TimeSpec &rh)const noexcept{
            return this->operator<(rh.time_value);
        }

        bool operator!=(const TimeSpec &rh)const noexcept{
            return this->operator!=(rh.time_value);
        }

        bool operator==(const TimeSpec &rh)const noexcept{
            return this->operator==(rh.time_value);
        }

        bool operator>(const TimeSpec &rh)const noexcept{
            return this->operator>(rh.time_value);
        }


        bool Reload(){
            if (time_interval.tv_sec == 0 && time_interval.tv_nsec == 0){
                return false;
            }

            time_value.tv_sec += time_interval.tv_sec;
            time_value.tv_nsec += time_interval.tv_nsec;

            return true;
        }

        TimeSpec()
            :time_value({}), time_interval({}), timer_id(-1)
        {
        }

        TimeSpec(time_t sec, long nsec, time_t interval_sec = 0, long interval_nsec = 0)
            :timer_id(-1)
        {
            log_debug("call 4 args constructor\n");
            time_value.tv_sec = sec;
            time_value.tv_nsec = nsec;

            time_interval.tv_sec = interval_sec;
            time_interval.tv_nsec = interval_nsec;
        }

        TimeSpec(const struct timespec &value, const struct timespec &interval = default_spec)
            :timer_id(-1)
        {
            time_value.tv_sec = value.tv_sec;
            time_value.tv_nsec = value.tv_nsec;
            time_interval.tv_sec = interval.tv_sec;
            time_interval.tv_nsec = interval.tv_nsec;
        }

        TimeSpec(const TimeSpec &value) = default;
        TimeSpec &operator=(const TimeSpec &value) = default;
    public:
        struct timespec time_value;      //nsec不能到s的级别
        struct timespec time_interval;   //nsec不能到s的级别
        TimerId timer_id;
    public:
        static const struct timespec default_spec;
    };

    const struct timespec TimeSpec::default_spec = {0,0};

    //最小堆数据结构
    typedef std::priority_queue<TimeSpec, std::vector<TimeSpec>, std::greater<TimeSpec>> TimerBuffer;

    template<typename T>
    class Timer:public Pollable{
    public:
        // TODO: only support Clock::REALTIME now, support ClockType::MONOTONIC.
        Timer(ClockType timer_type = ClockType::REALTIME)
            : id_(0)
        {
            timer_fd_ = timerfd_create((clockid_t)timer_type,TFD_NONBLOCK|TFD_CLOEXEC);
            if (timer_fd_ < 0 ){
                SYSERROR();
            }

            timeout_handler = [](const TimeSpec &top, T const &t){
                log_debug("timer:%u , userdata:%p\n",top.timer_id, t);
            };
        }

        ~Timer() noexcept{
            if (timer_fd_!=-1){
                close(timer_fd_);
                timer_fd_ = -1;
            }
        }


        void DelTimer(const TimerId &id){
            std::lock_guard<std::mutex> _(buf_lock_);
            userdata_map.erase(id);
        }

        int GetFD() const override{
            return timer_fd_;
        }

        void OnWriteable(Poller &) override{
            ERROR("unwriteable");
        }

        void OnError(Poller &) override{
            log_warn("error....\n");
        }

        void OnReadable(Poller &) override{
            //TODO: 用ET模式可以多次timer超时后才调用read。
            uint64_t exp;
            if (read(timer_fd_, &exp, sizeof(exp))!=sizeof(exp)){
                ERROR("Timer Fatal Error");
            }

            struct timespec now;
            GetTime(ClockType::REALTIME, &now);

            std::lock_guard<std::mutex> _(buf_lock_);
            while (!buf_.empty()){
                TimeSpec top = buf_.top();
                if (top > now){
                    SetTime(top);
                    break;
                }
                log_debug("top: sec:%u, nsec:%ld ,isec:%u, insec:%ld \n",top.time_value.tv_sec, top.time_value.tv_nsec, top.time_interval.tv_sec, top.time_interval.tv_nsec);

                buf_.pop();

                try{
                    timeout_handler(top, userdata_map.at(top.timer_id));
                }catch(...){
                    log_debug("error timer:%u \n",top.timer_id);

                }

                if (top.Reload()){
                    log_debug("timer(%u) reload sec:%u, nsec:%ld\n",top.timer_id, top.time_value.tv_sec, top.time_value.tv_nsec);
                    buf_.push(top);
                }
            }
        }

        TimerId SetTimer(const TimeSpec &t, T const &userdata){

            std::lock_guard<std::mutex> _(buf_lock_);

            TimeSpec e = t;
            struct timespec curr;
            GetTime(ClockType::REALTIME, &curr);
            e.time_value.tv_sec += curr.tv_sec;
            e.time_value.tv_nsec += curr.tv_nsec;

            e.timer_id = id_;
            if (buf_.empty()){
                buf_.push(e);
                SetTime(e);
            }else{
                const TimeSpec &old_top = buf_.top();
                buf_.push(e);
                const TimeSpec &new_top = buf_.top();
                if (old_top != new_top){
                    SetTime(new_top);
                }
            }

            userdata_map[id_] = userdata;
            return id_++;
        }

    private:
        void SetTime(const TimeSpec &new_top){
            struct itimerspec new_value;
            new_value.it_value.tv_sec = new_top.time_value.tv_sec;
            new_value.it_value.tv_nsec = new_top.time_value.tv_nsec;
            new_value.it_interval.tv_sec = 0;
            new_value.it_interval.tv_nsec = 0;

            log_debug("settime sec :%u, nsec:%ld \n",new_value.it_value.tv_sec, new_value.it_value.tv_nsec);
            if (timerfd_settime(timer_fd_, TFD_TIMER_ABSTIME, &new_value, NULL) == -1){
                SYSERROR();
            }
        }
    private:
        int timer_fd_;
        TimerBuffer buf_;
        TimerId id_;
        std::mutex buf_lock_;
        std::unordered_map<TimerId, T> userdata_map;

    public:
        std::function<void(const TimeSpec &, T const &)> timeout_handler;
    };

}//namespace Runtime
#endif // __RUNTIME_TIMER_HPP


