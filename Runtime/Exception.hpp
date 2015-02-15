/*
 * Author: zzn
 * Date : Fri Dec 13 10:53:27 CST 2013
 *
 */

#ifndef __RUNTIME_EXCEPTION_HPP
#define  __RUNTIME_EXCEPTION_HPP

#include <exception>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <execinfo.h>
#include <stdlib.h>

#define SYSERROR() { throw SysException(__func__,__LINE__);}
#define SYSERROR1(eno) { throw SysException(__func__,__LINE__,eno);}
#define ERROR(msg) { throw Exception(__func__,__LINE__,msg);}
#define EOFERROR() { throw EOFException(__func__,__LINE__);}

#define THROW(E)  throw E(__func__,__LINE__)

class Exception:public std::exception
{
public:
    Exception(const char *func,int line,const char *msg);
    virtual ~Exception() noexcept;
    virtual const std::string ErrMsg() const noexcept;
    virtual const char *what() const noexcept;
private:
    const std::string Backtrace()const noexcept;

private:
    std::string funcname;
    int line;
    std::string errmsg;
    static const unsigned int stack_size=100;
};

class EOFException: public Exception
{
public:
    EOFException(const char *func,int line)
        :Exception(func,line,"EOF")
    {
    
    }   

};

class SysException: public Exception
{
public:
    SysException(const char *func,int line,int err=0) throw();
//    const char *ErrMsg();
private:
    int err;
};
#endif //__RUNTIME_EXCEPTION_HPP
