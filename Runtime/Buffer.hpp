#ifndef __RUNTIME_BUFFER_HPP
#define __RUNTIME_BUFFER_HPP

#include <sys/uio.h>
#include <string>
#include <vector>
#include "Exception.hpp"

namespace Runtime
{

    enum class BufState{
        HEADER_WAITING=0,
        DATA_WAITING,
        DONE,
    };
    struct ReadBuf{

        ReadBuf(){
            state = BufState::HEADER_WAITING;
            data.resize(4094);
            top = 0;
        }

        std::string data;
        BufState state;
        size_t top;
    };

    class WriteBuf{
    public:
        WriteBuf(){
            size_ = 0;
            cursor_ = 0;
        }

        void Grow(void *buf,size_t len){
            size_t cap = size_ + len;
            data.resize(cap);
            memcpy(&data[size_],buf,len);
            size_ = cap;
        }

        void Eat(size_t len){
            size_t size = cursor_ + len;
            if (size < size_){
                cursor_ = size;
            }else if(size == size_){
                cursor_ = size_ = 0;
            }else{
                ERROR("size must not larger than size_");
            }
        }

        bool Empty(){
            return (cursor_ == size_);
        }

        struct iovec *Get(struct iovec *v){
            v->iov_base = &data[cursor_]; 
            v->iov_len = size_ - cursor_;
            return v;
        }

        ~WriteBuf(){
        }

        size_t size_;
        size_t cursor_;
        std::string data;
    };

}
#endif //__RUNTIME_BUFFER_HPP
