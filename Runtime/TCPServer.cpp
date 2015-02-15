/*
 * Author      : zzn
 * Created     : 2013-12-19 16:21:05
 * Description : 
 * License: GNU Public License.
*/

#include "TCPServer.hpp"
#include "Logger.hpp"

using namespace Runtime;

void default_msg_handler(TCPConn &conn, ReadBuf &buf){
    log_debug("default msg handler, ignore msg\n");

}

TCPServer::TCPServer(short port, const char *ip) {
    int enable = 1;
    server_sock_.SetSockOpt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    server_sock_.Bind(port,ip);
    server_sock_.Listen();
    
    //监听socket必须设置为non-blocking，否则可能导致accept阻塞。
    server_sock_.SetNonBlock();
    msg_handler = default_msg_handler;
}


void TCPServer::OnError(Poller &) {
    ERROR("server down");
}

TCPServer::~TCPServer() {
    for (auto it = client_link_.begin(); it != client_link_.end(); ++it){
        if (*it!=NULL){
            delete *it;
        }
    }
}

void TCPServer::add_link(TCPConn *cli){
    client_link_.push_back(cli);
}

void TCPServer::rm_link(TCPConn *cli){
    for (auto it = client_link_.begin(); it != client_link_.end(); ++it){
        if (*it==cli){
            client_link_.erase(it);
            return;
        }
    }
}


void TCPServer::OnReadable(Poller &poller){
    Address4 addr;
    int fd=server_sock_.Accept(addr);
    if (fd>0){
        TCPConn *cs = new TCPConn(*this,fd,addr);
        if (cs!=NULL){
            log_debug("add connection :%p \n",cs);
            add_link(cs);
            poller.Add(cs, Event::POLLIN|Event::POLLOUT|Event::POLLET);
        }
    }
}

void TCPServer::OnWriteable(Poller &){
    ERROR("not writeable");
}

void TCPServer::Remove(TCPConn *cli){
    rm_link(cli);
    delete cli;
}

int TCPServer::GetFD()const{
    return server_sock_.GetFD();
}




TCPConn::TCPConn(TCPServer &server, int fd, Address4 &addr)
    :server_(server),client_sock_(fd, addr)
{
    client_sock_.SetNonBlock();
}

void TCPConn::OnError(Poller &poller){
    log_debug("remove it:%p\n",this);
    poller.Del(this);
    server_.Remove(this);
    log_debug("remove it:%p done\n",this);
}

/*
void TCPConn::Read(void *buf, size_t len){
    std::lock_guard<std::mutex> _(read_lock_);
    if (read_buf_.Size() >= len){
        struct iovec v[1];
        v[0].iov_base = buf;
        v[0].iov_len = len;
        read_buf_.Eat(&v[0],sizeof(v)/sizeof(v[0]));
    }else{
        //TODO: read block;
        Yield();
    }
}
*/

/*
 * FIXME: 如果当前connection有许多内容要读，可能会导致别的连接饿死。
 * 所以读一段内容后要让出处理时间以处理别连接。
 */
void TCPConn::OnReadable(Poller &){
    std::lock_guard<std::mutex> _(read_lock_);
    ssize_t len=0;
    while (true){
        if (read_buf_.data.size() - read_buf_.top < 4096){
            read_buf_.data.resize(read_buf_.data.size() + 4096);
        }

        len=client_sock_.Read(&read_buf_.data[read_buf_.top], read_buf_.data.size() - read_buf_.top);
        if (len==-1){
            //log_debug("no more to read\n");
            return;
        }else if(len==0){
            log_debug("connection down:%p\n",this);
            EOFERROR();
            return;
        }else{
            read_buf_.top += len;
            log_debug("%p read len:%ld data, what to do now\n",this, len);
            server_.msg_handler(*this,read_buf_);
        }
    }
}

void TCPConn::Write(void *buf, size_t len){
    if (len > 0){
        int ret = 0;
        std::lock_guard<std::mutex> _(write_lock_);

        if (write_buf_.Empty()){
            ret=client_sock_.Write(buf,len);
            if (ret==-1){
                log_debug("write buffer is full");
                return;
            }
        }
        if (len - ret >0){
            write_buf_.Grow((char *)buf + ret, len - ret);
        }
    }
}

void TCPConn::OnWriteable(Poller &){
    log_debug("Writable function\n");

    struct iovec buf;
    ssize_t len;
    std::lock_guard<std::mutex> _(write_lock_);
    while(!write_buf_.Empty()) {
        write_buf_.Get(&buf);
        len=client_sock_.Write(buf.iov_base,buf.iov_len);
        if (len==-1){
            log_debug("socket write buffer is full");
            return;
        }

        write_buf_.Eat(len);
        log_debug("write %ld data\n",len);
        log_debug("%s\n",(char *)buf.iov_base);

        if (len < (ssize_t)buf.iov_len){
            // socket write buffer is full.
            return;
        }
    }

}

int TCPConn::GetFD()const{
    return client_sock_.GetFD();
}

