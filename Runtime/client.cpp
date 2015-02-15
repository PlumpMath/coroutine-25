#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "Logger.hpp"

#include "Address.hpp"
#include "Socket.hpp"
#include "Epoll.hpp"
#include <string.h>
#include "Protocol.hpp"

namespace Runtime
{
    Logger log;

    class TCPClient:public Pollable
    {
    public:
        TCPClient(const char *ip, short port, Epoll &poller)
            :poller_(poller)
        {
            // connect成功之前无事可干
            client_sock_.Connect(ip, port);

            //    client_sock_.SetNonBlock();
        }

        ~TCPClient() noexcept{

        }

        void OnReadable(Poller &)override{

        }
        void OnWriteable(Poller &)override{

        }
        void OnError(Poller &)override{

        }

        void Write(char *msg, size_t len){
            Protocol p;
            p.len = htonl(len);
            int c = 1;
            p.channel = htonl(c);

            struct iovec v[2];
            v[0].iov_base = &p;
            v[0].iov_len = sizeof(p);
            v[1].iov_base = msg;
            v[1].iov_len = len;

            client_sock_.Writev(&v[0],2);
        }

        int GetFD()const override{
            return client_sock_.GetFD();
        }
    private:
        Epoll  &poller_;
        Socket client_sock_; 
    };

    class Stdin:public Pollable{
    public:
        Stdin(Epoll &poller, TCPClient &cli)
            :poller_(poller),cli_(cli)
        {
            poller_.Add(this, Event::POLLIN);
            eof_ = false;
        }

        void OnReadable(Poller &){
            int len = read(fd,&line_buf_[0],sizeof(line_buf_));
            if (len==0){
                poller_.Del(this);
                eof_ = true;
            }
            cli_.Write(line_buf_, len);
        }

        void OnWriteable(Poller &){

        }

        void OnError(Poller &){

        }

        int GetFD()const override{
            return fd;
        }

        bool End(){
            return eof_;
        }
    private:
        Epoll &poller_;
        bool eof_;
        char line_buf_[1024];
        TCPClient &cli_;
    private:
        static const int fd = 0;
    };

};

using namespace Runtime;


int main(int argc, char **argv){

    assert(argc > 1);

    short port = atoi(argv[1]);
    const char *ip="127.0.0.1";

    if (argc > 2){
        ip = argv[2];
    }


    try{
        Epoll poller;

        TCPClient c(ip, port, poller);
        Stdin s(poller, c);

        poller.Loop();
    }catch (std::exception &e){
        printf("%s",e.what());
    }catch(...){
        printf("unknow exception\n");
    }

}
