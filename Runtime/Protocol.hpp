#ifndef __PROTOCOL_HPP
#define __PROTOCOL_HPP

#include <sys/types.h>
#include <stdint.h>

#pragma pack(1)

struct Protocol{
    uint32_t len;
    uint32_t channel;
    char data[0];
};

#pragma pack()

#endif// __PROTOCOL_HPP

