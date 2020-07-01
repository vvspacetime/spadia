#include "rtcp_packet.h"

int RTCPPacket::Parse(uint8_t* data, ssize_t len, std::vector<std::shared_ptr<RTCPPacketInterface>>& output) {
    int cnt = 0;
    while(len >= 4) {
        uint8_t pt = webrtc::ByteReader<uint8_t>::ReadBigEndian((uint8_t*)data + 1);
        uint16_t sliceLen = webrtc::ByteReader<uint16_t>::ReadBigEndian((uint8_t*)data + 2);
        uint32_t packetLen =  RTCPCommonHeaderSize + sliceLen * 4;
        switch (pt) {
            case SenderReport: {
                output.push_back(std::make_shared<SendReportPacket>((uint8_t*)data, packetLen));
                break;
            }
            case ReceiverReport: {
                output.push_back(std::make_shared<ReceiverReportPacket>((uint8_t*)data, packetLen));
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
        len -= packetLen;
        data += packetLen;
        cnt ++;
    }
    return cnt;
}
