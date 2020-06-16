#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H
#include <cstdint>
#include <uv.h>
#include <uuid/uuid.h>
#include <string>

uint64_t get_time_ms();

std::string uuid();

// only number or a--z or A--Z
std::string randomID();

inline uint32_t pad32(uint32_t size)
{
    //Alling to 32 bits
    if (size & 0x03)
        //Add padding
        return  (size & 0xFFFFFFFC) + 0x04;
    else
        return size;
}

inline uint16_t get_port(uint16_t sin_port) {
    return ((sin_port & 0x00FF) << 8) | ((sin_port & 0xFF00) >> 8);
}

#endif //TEST_TOOLS_H
