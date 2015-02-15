#include "Exception.hpp"
#include <stdio.h>


Exception::Exception(const char *_func,int _line,const char *msg)
    :funcname(_func),line(_line),errmsg(msg)
{

}

Exception::~Exception() noexcept
{
}

const std::string Exception::Backtrace() const noexcept{

    void *buffer[stack_size];
    std::string s;

    int n= backtrace(buffer, stack_size);
    char **symbols = backtrace_symbols(buffer, n);
    if (symbols == NULL) {
        return std::string(strerror(errno));
    }

    for (int j=0;j<n;j++)
    {
        s+=symbols[j];
        s+="\n";
    }

    free(symbols);
    return s;
}

const std::string Exception::ErrMsg() const noexcept {
    return errmsg;
}

const char *Exception::what() const noexcept{
    std::stringstream e;
    e<<"POS = "<<funcname<<":"<<line<<'\n';
    e<<"DESC= "<<ErrMsg()<<'\n';
    e<<"BackTrace=\n"<<Backtrace();
    return e.str().c_str();
}

SysException::SysException(const char *func,int line,int err) throw()
    :Exception(func,line,strerror((err==0)?errno:err))
{
    
}
#ifdef EXCEPTION_DEBUG
int main()
{
    try
    {
//        ERROR("ERROR");
        SYSERROR();
    }catch(Exception &e){
        printf("%s ",e.what());
    }
    return 0;
}

#endif //EXCEPTION_DEBUG
