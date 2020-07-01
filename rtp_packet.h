#ifndef RTP_PLAYER_RTP_PACKET_H
#define RTP_PLAYER_RTP_PACKET_H
#include <cstdint>
#include <cstdio>
#include <byte_io.h>
#include <iostream>
#include <vector>
#include "tools.h"
#include <map>
struct RTPExtension {
    uint8_t id;
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
    RTPPacket(): recvTimeMS_(get_time_ms()) {

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
    uint32_t SSRC() { return ssrc_; }
    uint16_t SeqNum() { return seqNum_; }

    uint8_t *Data() { return data_;}
    int Size() { return size_; }

public:
    uint32_t ExtSeqNum() { return uint32_t (seqCycle_ << 16u) | seqNum_; }
    uint16_t SetSeqCycle(uint16_t cycle) { seqCycle_ = cycle; };
    uint64_t RecvTimeMS() { return recvTimeMS_; }

    bool GetTransportCCNum(uint8_t id, uint16_t& num) {
        auto it = exts_.find(id);
        if (it != exts_.end()) {
            num = webrtc::ByteReader<uint16_t>::ReadBigEndian(it->second.data);
            return true;
        }
        return false;
    };
    // extensions

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
    uint16_t seqCycle_ = 0;

    uint64_t recvTimeMS_;
    std::map<uint16_t, RTPExtension> exts_;
};


#endif //RTP_PLAYER_RTP_PACKET_H
