#ifndef __RUNTIME_LOGGER_HPP
#define __RUNTIME_LOGGER_HPP

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Exception.hpp"
#include <sys/uio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <stdarg.h>

namespace Runtime{

    enum class LogLevel{
        DEBUG = 0,
        INFO,
        WARN,
        ERROR,
    };

    class Logger{
    public:
    public:
        static const int SIZE = 1024*1024*128;
        static const int COUNT = 16;
        static const int FILE_MODE = 0644;
        static const int FILE_FLAG = O_APPEND|O_CREAT|O_WRONLY;
        static const int MAX_LENGTH = 2048;
    public:
        //TODO: support function and line.
        Logger(const char *path = nullptr, int size=SIZE, int count=COUNT);
        ~Logger();

        void Debug(const char *fmt, ...){
            va_list ap;
            va_start(ap, fmt);
            SendLog(LogLevel::DEBUG,fmt, ap); 
            va_end(ap);
        }
        void Info(const char *fmt, ...){
            va_list ap;
            va_start(ap, fmt);
            SendLog(LogLevel::INFO,fmt, ap); 
            va_end(ap);
        }
        void Warn(const char *fmt, ...){
            va_list ap;
            va_start(ap, fmt);
            SendLog(LogLevel::WARN,fmt, ap); 
            va_end(ap);
        }
        void Error(const char *fmt, ...){
            va_list ap;
            va_start(ap, fmt);
            SendLog(LogLevel::ERROR,fmt, ap); 
            va_end(ap);
        }


        void SetFmt(const char *fmt);
        void SetLevel(LogLevel lev);
    private:
        void SendLog(LogLevel lev,const char *fmt, va_list ap){

            {
                std::lock_guard<std::mutex> _(lock_);
                if ((int)lev < (int)lev_ ){
                    return;
                }
            }

            char buf[MAX_LENGTH];
            int size = vsnprintf(buf, sizeof(buf), fmt, ap);

            //确保以\n结尾
            if (size < MAX_LENGTH - 1){
                size++;
            }
            buf[size] = '\n';

            std::string meta = log_meta(lev);
            struct iovec v[2];
            v[0].iov_base = (void *)&meta[0];
            v[0].iov_len = meta.size();

            v[1].iov_base = (void *)buf;
            v[1].iov_len = size;

            if(writev(fd_, &v[0], sizeof(v)/sizeof(v[0]))<0){
                SYSERROR();
            }
        }

        std::string log_meta(LogLevel lev);
        const char *gettime(char *buf,int size);
    private:
        int fd_;
        int size_; 
        int count_; 
        LogLevel lev_;
        std::string path_;
        std::string fmt_; 
        std::vector<std::string> fmt_vec_;
    private:
        std::mutex lock_;
    };



#ifdef RUNTIME_LOG_ENABLE
extern Runtime::Logger log;
#define log_debug(fmt,...) log.Debug(fmt,##__VA_ARGS__)
#define log_info(fmt,...) log.Info(fmt,##__VA_ARGS__)
#define log_warn(fmt,...) log.Warn(fmt,##__VA_ARGS__)
#define log_error(fmt,...) log.Error(fmt,##__VA_ARGS__)
#else
#define log_debug(fmt,...) 
#define log_info(fmt,...) 
#define log_warn(fmt,...) 
#define log_error(fmt,...) 
#endif
}

#endif  //__RUNTIME_LOGGER_HPP
