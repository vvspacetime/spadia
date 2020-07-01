#ifndef TEST_RTP_SESSION_MANAGER_H
#define TEST_RTP_SESSION_MANAGER_H

#include <utility>
#include <vector>
#include <map>
#include "rtcp_packet.h"
#include "rtp_packet.h"

class PeerConnection;
class RTPSessionManager;

class RTPIncomingSession {
    class RTPLostSession {
    public:
        static constexpr int Size = 256;
        RTPLostSession(): packets(Size) {

        }
        void Reset();
        uint16_t AddPacket(std::shared_ptr<RTPPacket>& pkt);
        std::vector<RTCPRTPFeedbackPacket::NackFeedBack> GetNacks();
        uint16_t GetTotal();

    public:
        struct PacketInfo {
            PacketInfo() {}
            PacketInfo(uint32_t extSeq, uint64_t create_time): extSeq(extSeq), created_at_time(create_time) {

            }
            uint64_t loss_at_time = 0;
            uint64_t created_at_time = 0; // flag exist
            uint64_t sent_at_time = 0;
            uint8_t retry = 0;
            int64_t extSeq = -1;
        };
    private:
        static bool isLost(const PacketInfo& pkt, uint64_t now, uint32_t rtt);
        static bool isCountInTotalLost(const PacketInfo& pkt);
        std::vector<PacketInfo> packets;
        uint16_t totalLost = 0;
        uint32_t nextSeq = 0;

    };
public:
    uint64_t lastRRMS_;
    uv_timer_t timer_;
    RTPSessionManager* manager_;
    RTPIncomingSession(uv_loop_t* loop, RTPSessionManager* manager);

    void Stop() {
        uv_timer_stop(&timer_);
        timer_.data = nullptr;
    }

    void OnTimer();

    bool AddPacket(std::shared_ptr<RTPPacket>& pkt);
};

class RTPOutgoingSession {
public:
    RTPOutgoingSession(uv_loop_t* loop, RTPSessionManager* manager) {

    }
};

class RTPSessionManager {
public:
    RTPSessionManager(uv_loop_t* loop, std::weak_ptr<PeerConnection> pc):
        incoming_(loop, this),
        outgoing_(loop, this),
        pc_(std::move(pc)) {
    }

    ~RTPSessionManager() {
        incoming_.Stop();
    }
    enum RTPExtensionType {
        Unknown = 0,
        TransportCC = 1
    };

    bool OnRecvRTP(std::shared_ptr<RTPPacket>& pkt) {
        //
        incoming_.AddPacket(pkt);
    }

    bool OnRecvRTCP(std::shared_ptr<RTCPPacketInterface> &rtcp) {
        if (rtcp->Type() == RTCPPacket::SenderReport) {
//            incoming_.
        }
        // expand transport cc num
        // expand seq num
    }

    bool OnSendRTP() {

    }

    bool OnSendRTCP() {
        // only statistic
    }

    bool SetExtension(uint8_t id, const std::string& extUri) {
        if (extUri == "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01") {
            extMap_[id] = TransportCC;
            return true;
        }
        // ignore other ext
        return false;
    }

    bool GetExtensionId(RTPExtensionType type, uint8_t& id) {
        for (auto& it: extMap_) {
            if (it.second == type) {
                id = it.first;
                return true;
            }
        }
        return false;
    }
    void OnRTCP(RTCPPacketInterface &rtcp);

private:
    RTPIncomingSession incoming_;
    RTPOutgoingSession outgoing_;
    std::weak_ptr<PeerConnection> pc_;
    std::map<uint8_t, RTPExtensionType> extMap_; // id to extension
};


#endif //TEST_RTP_SESSION_MANAGER_H
