#ifndef TEST_RTP_SESSION_MANAGER_H
#define TEST_RTP_SESSION_MANAGER_H

#include <vector>
#include <map>
#include "rtcp_packet.h"
#include "rtp_packet.h"



class LostPackets {

};

class RTPIncomingSession {

};

class RTPOutgoingSession {

};

class RTPSessionManager {
public:
    enum RTPExtension {
        Unknown = 0,
        TransportCC = 1
    };

    std::vector<std::shared_ptr<RTCPPacketInterface>> GenerateRTCP() {

    }

    /**
     *
     * @param pkt  input
     * @param data output
     * @param size output
     * @return
     */
    bool IncomingRTP(RTPPacket& pkt, uint8_t* &data, int& size) {
        // todo
//        pkt.Serialize()
    }

    bool IncomingRTCP() {

    }

    bool OutgoingRTP() {

    }

    bool OutgoingRTCP() {

    }

    bool SetExtension(uint8_t id, const std::string& extUri) {
        if (extUri == "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01") {
            extMap[id] = TransportCC;
            return true;
        }
        // ignore other ext
        return false;
    }

private:
    std::map<uint32_t, LostPackets> losts; // ssrc->lost
    std::map<uint8_t, RTPExtension> extMap; // id:
//    std::map<uint32_t, uint8_t> payloadMap; // ssrc payload
};


#endif //TEST_RTP_SESSION_MANAGER_H
