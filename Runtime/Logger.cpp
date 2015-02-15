#include "Logger.hpp"
#include <unistd.h>
#include <time.h>
#include <sys/uio.h>

using namespace Runtime;
static const char *LogLevelStr[]={ "DD", "II", "WW", "EE",};

Logger::Logger(const char *path, int size, int count)
    :fd_(0),size_(size),count_(count),lev_(LogLevel::DEBUG),path_(path==nullptr?"":path)
{
    if (path!=nullptr){
        fd_ = open(path,FILE_FLAG,FILE_MODE);
        if (fd_ == -1 ){
            SYSERROR();
        }
    }

    SetFmt("[%p][%t]%v");
}

Logger::~Logger(){
    if (fd_ != -1){
        close(fd_);
    }
    fd_ = -1;
}

void Logger::SetLevel(LogLevel lev){
    std::lock_guard<std::mutex> _(lock_);
    lev_ = lev;
}


/*
 * %t => log time, only support time format: "%F %T".
 * %p => log level prompt.
 * %f => source file path.
 * %l => source line.
 * %v => log content.
 */
void Logger::SetFmt(const char *fmt){
    std::lock_guard<std::mutex> _(lock_);

    fmt_=fmt;

    fmt_vec_.clear();
    for(size_t i=0,begin=0; i < fmt_.size(); ++i){
        if(fmt_[i]=='%'){
            switch(fmt_[++i]){
                case 'p':
                case 't':
                case 'f':   //TODO
                case 'l':   //TODO
                case 'v':   //TODO
                    fmt_vec_.push_back(fmt_.substr(begin,i - begin - 1));
                    fmt_vec_.push_back(fmt_.substr(i, 1));
                    begin = i + 1;
                    break;
                default:
                    break;
            }
        }
    }
}

std::string Logger::log_meta(LogLevel lev){
    std::lock_guard<std::mutex> _(lock_);

    std::string meta;
    for (size_t i = 0; i<fmt_vec_.size(); i++){
        if ((i & 0x1)==0){
            meta += fmt_vec_[i];
            continue;
        }
        switch (fmt_vec_[i][0]){
            case 'p':
                meta += LogLevelStr[(int)lev];
                break;
            case 't':
                {
                    char strbuf[32];
                    meta += gettime(strbuf, sizeof(strbuf));
                }
                break;
            case 'f':   //TODO
            case 'l':   //TODO
            case 'v':   //TODO
                break;
            default:
                break;
        }
    }
    return meta;
}

const char *Logger::gettime(char *now_str,int size){
    const char *time_fmt="%F %T";
    time_t now;
    time(&now);
    struct tm now_tm;
    gmtime_r(&now, &now_tm);

    strftime(now_str,size,time_fmt,&now_tm);
    return now_str;
}


#ifdef __LOGGER_UNITTEST

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#if MULTI_PROCESS
int main(int argc, char **argv){
    const char *path = "/tmp/test.log";
    if (argc > 1){
        path = argv[1];
    }
    Logger log(path);

    log.SetLevel(LogLevel::INFO);
    log.SetFmt("%p%t%v");

    int pid = fork();
    if (pid < 0 ){
        exit(1);
    }
    for (int i =0 ; i< 1; ++i){
        log.Debug("DDDDDDDDDDDDDD\n");
        log.Info("12222\n");
    }

    if (pid > 0 ){
        waitpid(pid,NULL,0);
    }

    return 0;
}

#else

#include <thread>

void dolog(Logger *p){
    p->SetFmt("%p%t %v");
    p->SetLevel(LogLevel::DEBUG);

    for (int i =0 ; i< 100; ++i){
        p->Debug("DDDDDDD222\n");
        p->Info("12222\n");
        p->Info("%d  %s \n",12222,"abc","ddd");
    }
}

int main(int argc, char **argv){
    const char *path = nullptr;
    if (argc > 1){
        path = argv[1];
    }
    Logger log(path);

    std::thread t1(dolog, &log);

    log.SetLevel(LogLevel::INFO);

    t1.join();

    return 0;
}

#endif //MULTI_PROCESS

#endif //__LOGGER_UNITTEST
