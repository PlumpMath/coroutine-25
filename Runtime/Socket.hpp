#ifndef __SOCKET_HPP
#define __SOCKET_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <limits.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Address.hpp"
#include <sys/uio.h>


namespace Runtime
{
    enum FLAG
    {
        FLAG_CLR,
        FLAG_SET,
    };

    class FlagGuard {
    public:
        FlagGuard(int fd,int flag,int type=FLAG_SET);
        ~FlagGuard();
    private:
        int m_flag;
        int m_fd;
    };

    class Socket 
    {
    public:
        Socket(int domain=AF_INET,int type=SOCK_STREAM,int protocol=0);
        Socket(int fd,const Address4 &addr);
        ~Socket();
        int GetFD()const;
        ssize_t Write(const void *msg,size_t len);
        ssize_t Writev(const struct iovec *iov, int iovcnt);

        ssize_t Read(void *buf,size_t len);
        ssize_t Readv(const struct iovec *iov, int iovcnt);

        void Bind(const Address4 &addr);
        void Bind(short port, const char *ip=NULL);
        void Listen(int qlen=25);
        int Accept(Address &addr);

        void SetNonBlock();
        void SetBlock();

        void SetLinger(int time=10);
        void SetSockOpt(int level, int optname, const void *optval, socklen_t optlen);

        void Connect(const Address &addr);
        void Connect(const char *ip,short port);
        void Close();
        void Shutdown(int);
    private:
        void set_addr(const Address4 &addr);
    private:
        int sock_;
        std::auto_ptr<Address> m_address;
    };
    
    //client
    class CSocket:public Socket
    {
    public:
    };
    
    //Server
    class SSocket:public Socket
    {
    public:
    private:

    };

/*
    class INSocket:Socket
    {
    };

    class UNSocket:Socket
    {
    }
*/
}

#endif //__SOCKET_HPP
