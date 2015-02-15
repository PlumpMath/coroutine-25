#include "Timer.hpp"

using namespace Runtime;

#ifdef __RUNTIME_TIMER_UNITTEST

Logger Runtime::log;
int main(){
    Epoll p;
    Timer<int> timer(p);
    TimeSpec t(3,0);
    TimeSpec t2(5,0);
    TimeSpec t3(5,0);
 
    TimerId i = timer.SetTimer(t, 10);
    TimerId i2 = timer.SetTimer(t2, 11);
    TimerId i3 = timer.SetTimer(t3, 12);
    log_debug("timer id :%u \n",i);

    timer.DelTimer(i2);

    p.Loop();
}

#endif //__RUNTIME_TIMER_UNITTEST




