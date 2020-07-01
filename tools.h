#ifndef TEST_TOOLS_H
#define TEST_TOOLS_H
#include <cstdint>
#include <uv.h>
#include <uuid/uuid.h>
#include <string>



// since system start time
inline uint64_t get_time_ms() {
    return uv_hrtime()/1e+6;
}

// since UTC1970-1-1 0:0:0
inline uint64_t get_time_utc_us() {
    struct timeval now;
    gettimeofday(&now,0);
    return (((uint64_t)now.tv_sec)*1E6 + now.tv_usec);
}

inline uint64_t get_time_utc_ms() {
    return get_time_utc_us()/1e+3;
}

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
