#include "rtp_packet.h"
#include <cstring>
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|X|  CC   |M|     PT      |       sequence number         |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                           timestamp                           |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           synchronization source (SSRC) identifier            |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |            Contributing source (CSRC) identifiers             |
// |                             ....                              |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |  header eXtension profile id  |       length in 32bits        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                          Extensions                           |
// |                             ....                              |
// +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
// |                           Payload                             |
// |             ....              :  padding...                   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |               padding         | Padding size  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
constexpr size_t kFixedHeaderSize = 12;
constexpr uint8_t kRtpVersion = 2;
constexpr uint16_t kOneByteExtensionProfileId = 0xBEDE;
constexpr uint16_t kTwoByteExtensionProfileId = 0x1000;
constexpr size_t kOneByteExtensionHeaderLength = 1;
constexpr size_t kTwoByteExtensionHeaderLength = 2;
constexpr size_t kDefaultPacketSize = 1500;

bool RTPPacket::Parse(uint8_t *data, int size) {
    this->data_ = data;
    this->size_ = size;

    if (size < kFixedHeaderSize) {
        return -1;
    }
    const uint8_t version = data[0] >> 6;
    if (version != kRtpVersion) {
        return -1;
    }
    const bool hasPadding = (data[0] & 0x20) != 0;
    const bool hasExt = (data[0] & 0x10) != 0;
    const uint8_t numberCC = data[0] & 0x0f;
    maker_ = (data[1] & 0x80) != 0;
    payloadType_ = data[1] & 0x7f;
    seqNum_ = webrtc::ByteReader<uint16_t>::ReadBigEndian(&data[2]);
    timestamp_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&data[4]);
    ssrc_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(&data[8]);
    if (size < kFixedHeaderSize + numberCC * 4) {
        return -1;
    }
    payloadOffset_ = kFixedHeaderSize + numberCC * 4;
    if (hasPadding) {
        paddingSize_ = data[size - 1];
        if (paddingSize_ == 0) {
            std::cout << "warning: padding size is zero" << std::endl;
            return false;
        }
    } else {
        paddingSize_ = 0;
    }

    int extensions_size_ = 0;
    if (hasExt) {
        /* RTP header extension, RFC 3550.
         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |      defined by profile       |           length              |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                        header extension                       |
        |                             ....                              |
        */
        size_t extension_offset = payloadOffset_ + 4;
        if (extension_offset > size) {
            return false;
        }
        uint16_t profile =
                webrtc::ByteReader<uint16_t>::ReadBigEndian(&data[payloadOffset_]);
        size_t extensions_capacity =
                webrtc::ByteReader<uint16_t>::ReadBigEndian(&data[payloadOffset_ + 2]);
        extensions_capacity *= 4;
        if (extension_offset + extensions_capacity > size) {
            return false;
        }
        /*
         * one byte, len=bytes-1
           0                   1                   2                   3
           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |       0xBE    |    0xDE       |           length=3            |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |  ID   | L=0   |     data      |  ID   |  L=1  |   data...
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                ...data   |    0 (pad)    |    0 (pad)    |  ID   | L=3   |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                          data                                 |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          two byte
           0                   1                   2                   3
           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |       0x10    |    0x00       |           length=3            |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |      ID       |     L=0       |     ID        |     L=1       |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |       data    |    0 (pad)    |       ID      |      L=4      |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
          |                          data                                 |
          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                 */
        if (profile != kOneByteExtensionProfileId &&
            profile != kTwoByteExtensionProfileId) {
            std::cout << "Unsupported rtp extension " << profile << std::endl;
        } else {
            size_t extension_header_length = profile == kOneByteExtensionProfileId
                                             ? kOneByteExtensionHeaderLength
                                             : kTwoByteExtensionHeaderLength;
            constexpr uint8_t kPaddingByte = 0;
            constexpr uint8_t kPaddingId = 0;
            constexpr uint8_t kOneByteHeaderExtensionReservedId = 15;
            while (extensions_size_ + extension_header_length < extensions_capacity) {
                if (data[extension_offset + extensions_size_] == kPaddingByte) {
                    extensions_size_++;
                    continue;
                }
                int id;
                uint8_t length;
                if (profile == kOneByteExtensionProfileId) {
                    id = data[extension_offset + extensions_size_] >> 4;
                    length = 1 + (data[extension_offset + extensions_size_] & 0xf);
                    if (id == kOneByteHeaderExtensionReservedId ||
                        (id == kPaddingId && length != 1)) {
                        break;
                    }
                } else {
                    id = data[extension_offset + extensions_size_];
                    length = data[extension_offset + extensions_size_ + 1];
                }

                if (extensions_size_ + extension_header_length + length >
                    extensions_capacity) {
                    std::cout << "Oversized rtp header extension." << std::endl;
                    break;
                }

//                size_t offset =
                        extension_offset + extensions_size_ + extension_header_length;
//                std::cout << "extension id:" << id << std::endl;
//                std::cout << "extension offset:" << offset << std::endl;
//                std::cout << "extension length:" << length << std::endl;
                extensions_size_ += extension_header_length + length;
            }
        }
        payloadOffset_ = extension_offset + extensions_capacity;
    }

    if (payloadOffset_ + paddingSize_ > size) {
        return false;
    }
    payloadSize_ = size - payloadOffset_ - paddingSize_;
    // payload = [payloadOffset, payloadOffset+payloadSize)
    return true;
}

void RTPPacket::Dump() {
    printf("RTPPacket:[ssrc:%u,seq:%u,payload:%u,timestamp:%u,marker:%u]\n", ssrc_, seqNum_, payloadType_, timestamp_, maker_);
}

int RTPPacket::Serialize(uint8_t *&data, const std::vector<RTPExtension *> &exts, const RTPSession &session) {
    // todo exts
    auto len = 3 + payloadSize_; // header + payload, ignore extensions
    data = (uint8_t*)malloc(len);
    webrtc::ByteWriter<uint8_t>::WriteBigEndian(data, 0x10);
    data += 1;
    webrtc::ByteWriter<uint8_t>::WriteBigEndian(data, maker_ << 7 | session.pt);
    data += 1;
    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data, session.seq);
    data += 2;
    webrtc::ByteWriter<uint32_t>::WriteBigEndian(data, timestamp_);
    data += 4;
    memcpy(data, Payload(), PayloadSize());
    return 0;
}

