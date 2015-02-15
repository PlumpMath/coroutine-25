/*
 * Author      : zzn
 * Created     : 2013-12-19 16:21:05
 * Description : 
 * License: GNU Public License.
*/
#ifndef __TCPSERVER_HPP
#define __TCPSERVER_HPP

#include "Address.hpp"
#include "Socket.hpp"
#include "Epoll.hpp"
#include "Buffer.hpp"
#include <mutex>
#include <functional>


namespace Runtime
{

    class TCPConn;

    typedef std::function<void (TCPConn &,ReadBuf &)> MessageHandler;


    typedef std::vector<TCPConn *> TCPLink;
    class TCPServer:public Pollable
    {
    public:
        TCPServer(short port, const char *ip=NULL);
        virtual ~TCPServer();
        virtual void Remove(TCPConn *cli);

        void OnReadable(Poller &) override;
        void OnWriteable(Poller &)override;
        void OnError(Poller &)override;
        int GetFD()const;
    public:
        MessageHandler msg_handler;
    private:
        void add_link(TCPConn *cli);
        void rm_link(TCPConn *cli);
    protected:
        Socket server_sock_; 
        TCPLink  client_link_;
    };

    class TCPConn:public Pollable
    {
    public:
        TCPConn(TCPServer &server, int fd, Address4 &addr);

        void OnReadable(Poller &);
        void OnWriteable(Poller &);
        void OnError(Poller &);
        int GetFD()const;
        void Write(void *buf, size_t len);

    private:
        TCPServer &server_; 
        Socket client_sock_; 
    public:
        WriteBuf write_buf_;
        std::mutex write_lock_;
        ReadBuf read_buf_;
        std::mutex read_lock_;
    };
};
#endif  //__TCPSERVER_HPP
