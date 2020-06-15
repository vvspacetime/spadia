#include "rtcp_packet.h"

std::vector<std::shared_ptr<RTCPPacketInterface>> RTCPPacket::Parse(uint8_t* data, ssize_t len) {
    std::vector<std::shared_ptr<RTCPPacketInterface>> rv;
    while(len >= 4) {
        uint8_t pt = webrtc::ByteReader<uint8_t>::ReadBigEndian((uint8_t*)data + 1);
        uint16_t sliceLen = webrtc::ByteReader<uint16_t>::ReadBigEndian((uint8_t*)data + 2);
        switch (pt) {
            case SenderReport: {
                rv.push_back(std::make_shared<SendReportPacket>((uint8_t*)data, sliceLen + 4));
                break;
            }
            case ReceiverReport: {
                rv.push_back(std::make_shared<ReceiverReportPacket>((uint8_t*)data, sliceLen + 4));
                break;
            }
            case RTPFeedback: {
                break;
            }
            case PayloadFeedback: {
                break;
            }
            case XR: {
                break;
            }
            default: {

            }
        }
        len -= 4;
        data += 4;
    }
}

ssize_t ReceiverReportPacket::Serialize(uint8_t *&data) {
    return 0;
}
