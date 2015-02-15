#include "Address.hpp"

using namespace Runtime;

Address4::Address4(struct sockaddr_in *addr)
    :m_addr(new struct sockaddr_in)
{
    Init();
    if (addr!=NULL) {
        SetSockaddr(addr);
    }
}
Address4::Address4(short port,const char *ip )
    :m_addr(new struct sockaddr_in)
{
    Init();
    m_addr->sin_port=htons(port);
    if ((ip==NULL) || (strcmp(ip,"0.0.0.0")==0))
    {
        m_addr->sin_addr.s_addr=INADDR_ANY;
    }
    else if(inet_aton(ip,&(m_addr->sin_addr))==0)
    {
        ERROR("invalid ip");
    }
}

Address4::Address4(const Address4 &addr )
{
    m_addr.reset(new struct sockaddr_in);
    Init();
    SetSockaddr(addr.GetSockAddr(),m_len);
}

Address4 &Address4::operator=(const Address4 &addr )
{
    Init();
    SetSockaddr(addr.GetSockAddr(),m_len);

    return *this;
}

void Address4::Init()
{
    m_addr->sin_family=AF_INET;
}


void Address4::SetSockaddr(const struct sockaddr_in *addr)
{
    SetSockaddr((const struct sockaddr *)addr,m_len);
}

void Address4::SetSockaddr(const struct sockaddr *addr,socklen_t len)
{
    if (m_len!=len) {
        ERROR("invalid address");
    }
    memcpy(m_addr.get(),addr,m_len);
}

const char *Address4::GetIP(char *ip, socklen_t len) const
{
    return inet_ntop(m_addr->sin_family,&(m_addr->sin_addr),ip,len);
}

socklen_t Address4::GetSockLen() const
{
    return m_len;
}

struct sockaddr *Address4::GetSockAddr()
{
    return (struct sockaddr *)(m_addr.get());
}
const struct sockaddr *Address4::GetSockAddr() const
{
    return (struct sockaddr *)(m_addr.get());
}

int Address4::GetDomain() const
{
    return m_domain;
}

short Address4::GetPort() const
{
    return m_addr->sin_port;
}


Address6::Address6(struct sockaddr_in6 *addr)
    :m_addr(new struct sockaddr_in6)
{
    Init();
    if (addr!=NULL)
    {
        SetSockaddr(addr);
    }
}
Address6::Address6(const Address6 &addr )
{
    Init();
    SetSockaddr(addr.GetSockAddr(),m_len);
}

Address6 &Address6::operator=(const Address6 &addr )
{
    Init();
    SetSockaddr(addr.GetSockAddr(),m_len);

    return *this;
}
void Address6::Init()
{
    m_addr->sin6_family=AF_INET6;
}

void Address6::SetSockaddr(const struct sockaddr_in6 *addr)
{
    SetSockaddr((const struct sockaddr *)addr,m_len);
}

void Address6::SetSockaddr(const struct sockaddr *addr,socklen_t len)
{
    if (m_len!=len)
    {
        ERROR("invalid address");
    }
    memcpy(m_addr.get(),addr,m_len);
}

const char *Address6::GetIP(char *ip, socklen_t len) const
{
    return inet_ntop(m_addr->sin6_family,&(m_addr->sin6_addr),ip,len);
}

socklen_t Address6::GetSockLen() const
{
    return m_len;
}

struct sockaddr *Address6::GetSockAddr()
{
    return (struct sockaddr *)(m_addr.get());
}
const struct sockaddr *Address6::GetSockAddr() const
{
    return (struct sockaddr *)(m_addr.get());
}

int Address6::GetDomain() const
{
    return m_domain;
}

short Address6::GetPort() const
{
    return m_addr->sin6_port;
}


