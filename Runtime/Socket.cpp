/*
 * * Author: zzn
 * * Date : Fri Dec 13 12:07:13 CST 2013
 *
 * */

#include "Socket.hpp"
#include "Exception.hpp"
#include <stdio.h>

using namespace Runtime;

FlagGuard::FlagGuard(int fd,int _flag,int type)
    :m_fd(fd)
{
    m_flag=fcntl(m_fd,F_GETFL);
    if (m_flag==-1)
    {
        SYSERROR();
    }

    int f=m_flag;
    switch(type)
    {
    case FLAG_CLR:
        f=m_flag|_flag;
        break;
    case FLAG_SET:
        f=m_flag&(~_flag);
        break;
    default:
        //can not be here!
        ERROR("invalid action");
        break;
    }

    if (fcntl(m_fd,F_SETLK,f)==-1){
        SYSERROR();
    }
}

FlagGuard::~FlagGuard() {
    fcntl(m_fd,F_SETLK,m_flag);
}


Socket::Socket(int sd,const Address4 &addr)
    :sock_(sd)
{
    if (sock_<0){
        ERROR("invalid socket fd");
    }
    set_addr(addr);
    SetLinger();
}

Socket::Socket(int domain,int type,int protocol){
    sock_ = socket(domain,type,protocol);
    if (sock_==-1){
        SYSERROR();
    }

    m_address.reset(new Address4);

    SetLinger();
}

void Socket::Shutdown(int how) {
     if (sock_!=-1){
        shutdown(sock_, how);
    }
}
void Socket::Close() {
    if (sock_!=-1){
        close(sock_);
    }
    sock_ = -1;
}

Socket::~Socket() {
    Close();
}

int Socket::GetFD()const{
    return sock_;
}

/*
 *  EAGAIN时返回-1，连接断开时返回0
 * XXX: 还是改为EAGAIN返回0，连接断开时抛出异常？
 */
ssize_t Socket::Write(const void *pmsg, size_t len){
    const char *msg=(const char *)pmsg;
    if (len>0){
        ssize_t ret=write(sock_,msg,len);
        if (ret<0){
            int err=errno;
            if (err==EAGAIN || err== EWOULDBLOCK){
                // XXX: epoll said it's writable, but we get EAGAIN.
                return -1;
            }

            SYSERROR();
        }
        return ret;
    }
    return 0;
}

ssize_t Socket::Writev(const struct iovec *iov, int iovcnt){
    ssize_t ret=writev(sock_,iov,iovcnt);
    if (ret<0){
        int err=errno;
        if (err==EAGAIN || err== EWOULDBLOCK){
            // XXX: epoll said it's writable, but we get EAGAIN.
            return -1;
        }

        SYSERROR();
    }
    return ret;
}


ssize_t Socket::Readv(const struct iovec *iov, int iovcnt){
    ssize_t ret=readv(sock_,iov,iovcnt);
    if (ret<0){
        int err=errno;
        if (err==EAGAIN || err== EWOULDBLOCK){
            // XXX: epoll said it's writable, but we get EAGAIN.
            return -1;
        }

        SYSERROR();
    }
    return ret;
}

ssize_t Socket::Read(void *pbuf,size_t len){
    assert(len<SSIZE_MAX);
    char *buf=(char *)pbuf;
restart:
    ssize_t ret=read(sock_,buf,len);
    if (ret<0){
        int err=errno;
        if(err==EAGAIN || err== EWOULDBLOCK){
            // XXX: epoll said it's readable, but we get EAGAIN.
            return -1;
        }else if (err==EINTR){
            goto restart;
        }


        SYSERROR();

    }
    return ret;
}

void Socket::SetBlock(){
    int flag=fcntl(sock_,F_GETFL);
    if (flag==-1){
        SYSERROR();
    }

    if ((flag & O_NONBLOCK) == O_NONBLOCK){
        flag &= ~O_NONBLOCK;
        if (fcntl(sock_,F_SETFL, flag)<0){
            SYSERROR();
        }
    }
}
void Socket::SetNonBlock(){
    int flag=fcntl(sock_,F_GETFL);
    if (flag==-1){
        SYSERROR();
    }

    if ((flag & O_NONBLOCK) == O_NONBLOCK){
        return;
    }

    if (fcntl(sock_,F_SETFL,flag | O_NONBLOCK)<0) {
        SYSERROR();
    }
}

void Socket::SetLinger(int time) {
    struct linger l={.l_onoff=1,.l_linger=time};

    //    struct linger {
    //        int l_onoff;    /* linger active */
    //        int l_linger;   /* how many seconds to linger for */
    //    };

    SetSockOpt(SOL_SOCKET,SO_LINGER,&l,sizeof(l));
}

void Socket::SetSockOpt(int level, int optname, const void *optval, socklen_t optlen){
    if (setsockopt(sock_,level,optname,optval,optlen)!=0){
        SYSERROR();
    }
}

void Socket::Connect(const Address &addr) {
    int ret=connect(sock_,addr.GetSockAddr(),addr.GetSockLen());
    if ((ret < 0) && (errno != EINPROGRESS)){
        SYSERROR();
    }
}

void Socket::Connect(const char *ip, short port) {
    Address4 addr(port,ip);
    Connect(addr);
}

void Socket::Listen(int qlen) {
    if (listen(sock_,qlen)<0) {
        SYSERROR();
    }
}

int Socket::Accept(Address &addr) {
    struct sockaddr_in caddr;
    socklen_t len=sizeof(caddr);

    int cd=accept(sock_,(struct sockaddr *)&caddr,&len);
    if (cd==-1){
        //non-blocking socket可能会出现以下errno，这是正常现象。
        int err=errno;
        if (err==EWOULDBLOCK ||            // EAGAIN和EWOULDBLOCK 值一样。
            err==ECONNABORTED ||
            err==EINTR ||
            err==EPROTO)
        {

            return -1;
        }

        SYSERROR();
    }

    addr.SetSockaddr((struct sockaddr *)&caddr,len);
    return cd;
}

void Socket::Bind(short port , const char *ip ){
    Address4 addr(port,ip);
    Bind(addr);
}

void Socket::Bind(const Address4 &addr){
    set_addr(addr);
    struct sockaddr *self_addr=m_address->GetSockAddr();
    socklen_t len=m_address->GetSockLen();

    if (bind(sock_,self_addr,len)<0){
        SYSERROR();
    }
}

void Socket::set_addr(const Address4 &addr) {
    m_address.reset(new Address4(addr));
}

