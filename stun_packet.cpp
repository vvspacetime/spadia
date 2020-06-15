#include "stun_packet.h"
#include <openssl/hmac.h>
#include "byte_io.h"
#include <cstring>
#include "tools.h"
#include "crc32.h"
#include <assert.h>

static std::atomic_uint32_t g_transId; // todo each pc has an auto-increase id
static const uint8_t MagicCookie[4] = {0x21,0x12,0xA4,0x42};
constexpr int MaxStunSize = 640;
constexpr int StunHeaderSize = 20;
constexpr int AttrHeaderSize = 4;
constexpr int HMACSize = 20;
constexpr int FingerprintSize = 4;
StunPacket::StunPacket() {

}


std::unique_ptr<StunPacket> StunPacket::Parse(uint8_t *data, int size) {
    std::unique_ptr<StunPacket> rv(new StunPacket);
    /**
     *
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |0 0|     STUN Message Type     |         Message Length        |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                         Magic Cookie                          |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      |                     Transaction ID (96 bits)                  |
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

     STUN Message Type:
       0                 1
       2  3  4 5 6 7 8 9 0 1 2 3 4 5
      +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
      |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
      |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
      +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    uint16_t wd = webrtc::ByteReader<uint16_t>::ReadBigEndian(data);
    rv->method_ = static_cast<Method>(wd & 0xFEEF);
    rv->type_ = static_cast<Type>(wd & 0x0110);
    memcpy(rv->transId_, data + 8, 12); /// 96bit = 12byte

    /**
       STUN Attributes

       After the STUN header are zero or more attributes.  Each attribute
       MUST be TLV encoded, with a 16-bit type, 16-bit length, and value.
       Each STUN attribute MUST end on a 32-bit boundary.  As mentioned
       above, all fields in an attribute are transmitted most significant
       bit first.
           0                   1                   2                   3
           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |         Type                  |            Length             |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                         Value (variable)                ....
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
    int i = StunHeaderSize;
    while(i + 4 < size) {
        uint16_t attrType = webrtc::ByteReader<uint16_t>::ReadBigEndian(data + i);
        uint16_t attrLen = webrtc::ByteReader<uint16_t>::ReadBigEndian(data + i + 2);
        i += AttrHeaderSize;
        if (i + attrLen > size)  {
            return nullptr;
        }
        switch (attrType) {
            case Username: {
                rv->username_ = std::string((char*)data + i, attrLen);
                break;
            }
        }
        i += pad32(attrLen);
    }

    return std::move(rv);
}

std::unique_ptr<StunPacket> StunPacket::CreateRequest(std::string &localUsername, std::string &remoteUsername, std::string &remotePwd) {
    return std::unique_ptr<StunPacket>();
}

bool StunPacket::IsStun(const uint8_t *data, int size) {
    return ((data[0] == 0x00 && data[1] == 0x01)
            || (data[0] == 0x01 && data[1] == 0x01));
}

int StunPacket::Serialize(uint8_t *&data, const std::string &pwd) {
    data = (uint8_t*)malloc(MaxStunSize);
    uint16_t wd = (uint16_t)method_ | ((uint16_t)type_ & 2) << 7 | ((uint16_t)type_ & 1) << 4;
    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data, wd);
    memcpy(data+4, MagicCookie, 4);
    memcpy(data+8, transId_, 12);

    int attrSize = 0;
    // write attribute
    // username
//    if (username_.size()) {
//        int attrLen = pad32(username_.size());
//        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+StunHeaderSize+attrSize, Username);
//        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+StunHeaderSize+attrSize+2, attrLen);
//        memcpy(data+StunHeaderSize+AttrHeaderSize, username_.c_str(), username_.size());
//        attrSize += AttrHeaderSize + attrLen;
//    }

    // write xor ip
    {
        int attrLen = 8;
        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+StunHeaderSize+attrSize, 0x20);
        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+StunHeaderSize+attrSize+2, 8);
        webrtc::ByteWriter<uint64_t>::WriteBigEndian(data+StunHeaderSize+attrSize+AttrHeaderSize, 0x0001cd86e1ba8b43);
        attrSize += AttrHeaderSize + attrLen;
    }

    // write length, 4 attribute header and 20 byte HMAC-SHA1
    attrSize += AttrHeaderSize + HMACSize + AttrHeaderSize + FingerprintSize; // add message integrity
    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+2, attrSize - AttrHeaderSize - FingerprintSize);

    // calculate
    attrSize -= AttrHeaderSize + HMACSize + AttrHeaderSize + FingerprintSize; // remove message integrity
    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+attrSize+StunHeaderSize, AttributeType::MessageIntegrity);
    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+attrSize+StunHeaderSize+2, (uint16_t)HMACSize);
    uint len;
    HMAC(EVP_sha1(), pwd.c_str(), pwd.size(), data, attrSize+StunHeaderSize, data+StunHeaderSize+attrSize+AttrHeaderSize, &len);
//    printf("auth:\n");
//    for (int i = 0; i < attrSize + StunHeaderSize; i ++) {
//        printf("%u ", data[i]);
//    }
//    printf("auth end\n");
//    printf("pass: %s\n", pwd.c_str());
    assert(len == HMACSize);
    attrSize += AttrHeaderSize + HMACSize;

    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+2, attrSize + AttrHeaderSize + 4);
    // write fingerprint
    {
        uint32_t crc32 = CRC32::GetInstance().CalculateXORCRC(data, attrSize + StunHeaderSize);
        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+StunHeaderSize+attrSize, 0x8028);
        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data+StunHeaderSize+attrSize+2, 4);
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data+StunHeaderSize+attrSize+AttrHeaderSize, crc32);
        attrSize += AttrHeaderSize + 4;
    }


    return attrSize + StunHeaderSize;
}

std::unique_ptr<StunPacket> StunPacket::MakeResponse() {
    std::unique_ptr<StunPacket> rv(new StunPacket);
    rv->type_ = Response;
    rv->method_ = this->method_;
    rv->username_ = this->username_;
    memcpy(rv->transId_, this->transId_, 12);
    return rv;
}


