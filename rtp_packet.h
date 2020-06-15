#ifndef RTP_PLAYER_RTP_PACKET_H
#define RTP_PLAYER_RTP_PACKET_H
#include <cstdint>
#include <cstdio>
#include <byte_io.h>
#include <iostream>
#include <vector>

struct RTPExtension {
    bool oneByte;
    uint8_t header;
    uint8_t length;
    uint8_t* data;
};

// for ssrc and seq count
struct RTPSession {
    uint32_t ssrc;
    uint16_t seq;
    uint8_t pt;
};


class RTPPacket {
public:
    RTPPacket() {

    }

public:
    bool Parse(uint8_t* data, int size);
    /**
     * different session serialize to different ssrc
     */
    int Serialize(uint8_t* &data, const std::vector<RTPExtension*>& exts, const RTPSession& session);
    void Dump();
    uint8_t* Payload() { return data_ + payloadOffset_; }
    int PayloadSize() { return payloadSize_; }
    uint8_t PayloadType() { return payloadType_; }
private:
    int payloadOffset_;
    int payloadSize_;
    uint32_t ssrc_;
    uint32_t timestamp_;
    uint16_t seqNum_;
    uint8_t payloadType_;
    bool maker_;
    uint8_t *data_;
    int size_;
    int paddingSize_;
};


#endif //RTP_PLAYER_RTP_PACKET_H
