#ifndef __ADDRESS_HPP
#define  __ADDRESS_HPP

#include <memory>
#include <stdlib.h>
#include "Exception.hpp"
#include <stdio.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */



namespace Runtime
{
    class Address
    {
    public:
        const char *GetIP(char *ip, socklen_t len) const ;
        virtual short GetPort() const =0;
        virtual int GetDomain() const =0;
        virtual const struct sockaddr *GetSockAddr() const =0;
        virtual struct sockaddr *GetSockAddr() =0;
        virtual socklen_t GetSockLen() const=0;
        virtual void SetSockaddr(const struct sockaddr *addr,socklen_t len)=0;
    };

    class Address4:public Address
    {
    public:
        Address4(struct sockaddr_in *addr=NULL);
        Address4(short port,const char *ip=NULL);

        Address4(const Address4 &addr);
        Address4 &operator=(const Address4 &addr);

        const char *GetIP(char *ip, socklen_t len) const ;
        short GetPort() const ;
        int GetDomain() const ;
        const struct sockaddr *GetSockAddr() const;
        struct sockaddr *GetSockAddr();
        socklen_t GetSockLen() const;
        void SetSockaddr(const struct sockaddr *addr,socklen_t len);
    private:
        void SetSockaddr(const struct sockaddr_in *addr);
        void Init();
    private:
        std::auto_ptr<struct sockaddr_in> m_addr;
    private:
        static const socklen_t m_len=sizeof(struct sockaddr_in);
        static const int m_domain=AF_INET;
    };

    class Address6:public Address
    {
        Address6(struct sockaddr_in6 *addr=NULL);
        
        Address6(const Address6 &addr);
        Address6 &operator=(const Address6 &addr);

        const char *GetIP(char *ip, socklen_t len) const ;
        short GetPort() const ;
        int GetDomain() const ;
        struct sockaddr *GetSockAddr();
        const struct sockaddr *GetSockAddr() const;
        socklen_t GetSockLen() const;
        void SetSockaddr( const struct sockaddr *addr,socklen_t len);
    private:
        void SetSockaddr(const struct sockaddr_in6 *addr);
        void Init();
    private:
        std::auto_ptr<struct sockaddr_in6> m_addr;

        static const int m_domain=AF_INET6;
        static const socklen_t m_len=sizeof(struct sockaddr_in6);
    };

}
#endif //__ADDRESS_HPP
