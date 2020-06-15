#ifndef TEST_CRC32_H
#define TEST_CRC32_H
#include <cstdint>

class CRC32 {
public:
    CRC32() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (uint32_t j = 0; j < 8; ++j) {
                if (c & 1) {
                    c = 0xEDB88320 ^ (c >> 1);
                } else {
                    c >>= 1;
                }
            }
            table[i] = c;
        }
    }

    static CRC32& GetInstance() {
        static CRC32 crc32;
        return crc32;
    }

    uint32_t CalculateXORCRC(const uint8_t* data, uint32_t size) {
        uint32_t crc = 0;
        uint32_t c = crc ^ 0xFFFFFFFF;
        for (uint32_t i = 0; i < size; ++i)
            c = table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
        crc =  c ^ 0xFFFFFFFF;
        return crc ^ 0x5354554e;
    }

private:
    uint32_t table[256];
};




#endif //TEST_CRC32_H
