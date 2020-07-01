#include "rtp_session_manager.h"
#include <assert.h>
#include <math.h>
#include "peerconnection.h"

/**** IncomingSession ****/
RTPIncomingSession::RTPIncomingSession(uv_loop_t *loop, RTPSessionManager* manager): manager_(manager) {
    timer_.data = this;
    uv_timer_init(loop, &timer_);
    lastRRMS_ = get_time_ms();
}


bool RTPIncomingSession::AddPacket(std::shared_ptr<RTPPacket> &pkt) {
    // TODO: lost

    // TODO: rr
//    if (lastRRMS_ - pkt->RecvTimeMS() >= 1000) {
//        RTCPReceiverReportBlock block;
//        block.SetSSRC(uint32_t ssrc);
//        block.SetFractionLost(uint8_t fraction);
//        block.SetLostCount(uint32_t count);
//        block.SetLastSeqNum(uint32_t seq);
//        block.SetLastJitter(uint32_t jitter);
//        block.SetLastSR(uint32_t last);
//        block.SetDelaySinceLastSR(uint32_t delay);
//        ReceiverReportPacket pkt;
//        // todo
//        manager_->OnRTCP(pkt);
//    }
    // transport cc feedback
    std::unique_ptr<RTCPRTPFeedbackPacket::TransportCCFeedBack> field(new RTCPRTPFeedbackPacket::TransportCCFeedBack);
    uint8_t tccId = 0;
    if (manager_->GetExtensionId(RTPSessionManager::TransportCC, tccId)) {
        uint16_t tccNum = 0;
        // TODO get extended tcc number
        if (pkt->GetTransportCCNum(tccId, tccNum)) {
            field->InsertPacket(tccNum, get_time_utc_us());
            RTCPRTPFeedbackPacket packet;
            packet.SetSSRC(pkt->SSRC());
            packet.SetField(std::move(field));
            manager_->OnRTCP(packet);
        }
    }
    return true;
}

/**** RTPSessionManager ****/
void RTPSessionManager::OnRTCP(RTCPPacketInterface &rtcp) {
    uint8_t *data;
    ssize_t size = rtcp.Serialize(data);

    auto pc = pc_.lock();
    if (pc) {
        pc->sendData(data, size);
    }
}

/**** LostSession ***/
constexpr uint64_t nackWaitTime = 20; // 20ms
constexpr uint64_t jitterTime = 5;

void RTPIncomingSession::RTPLostSession::Reset() {
    packets.clear();
    packets.resize(Size);
    totalLost = 0;
    nextSeq = 0;
}

uint16_t RTPIncomingSession::RTPLostSession::AddPacket(std::shared_ptr<RTPPacket> &packet) {
    const uint32_t extSeq = packet->ExtSeqNum();

    if (extSeq > nextSeq && extSeq - nextSeq >= Size - 1) {
        packets.clear();
        packets.resize(Size);
        totalLost = 0;
        nextSeq = 0;
    }

    if (!nextSeq) {
        packets[extSeq % Size] = PacketInfo(extSeq, packet->RecvTimeMS());
        nextSeq = extSeq + 1;
        return totalLost;
    }

    uint16_t pos = extSeq % Size;
    if (nextSeq <= extSeq) { // future packet
        // printf("future\n");
        while(nextSeq < extSeq) {
            uint16_t nx = nextSeq % Size;
            if (isCountInTotalLost(packets[nx])) { // cover an lost packet
                totalLost --;
            }
            packets[nx] = PacketInfo(nextSeq, 0);
            packets[nx].loss_at_time = packet->RecvTimeMS();
            nextSeq ++;
            totalLost ++;
        }
        assert(nextSeq == extSeq);
        if (isCountInTotalLost(packets[pos])) { // cover an lost packet
            totalLost --;
        }
        packets[pos] = PacketInfo(extSeq, packet->RecvTimeMS());
        nextSeq ++;
    } else { // old packet
        if (isCountInTotalLost(packets[pos])) {
            totalLost --;
        }
        packets[pos] = PacketInfo(extSeq, packet->RecvTimeMS());
    }
    // printf("total lost:%lu\n", totalLost_);
    assert(totalLost <= Size);
    return totalLost;
}

bool RTPIncomingSession::RTPLostSession::isLost(const RTPIncomingSession::RTPLostSession::PacketInfo &pkt, uint64_t now,
                                                uint32_t rtt) {
    return (!pkt.created_at_time &&
            now - pkt.loss_at_time > jitterTime &&
            now - pkt.sent_at_time > fmax(nackWaitTime, rtt) &&
            pkt.retry < 20 &&
            pkt.extSeq != -1);
}

bool RTPIncomingSession::RTPLostSession::isCountInTotalLost(const RTPIncomingSession::RTPLostSession::PacketInfo &pkt) {
    return pkt.created_at_time == 0 && pkt.extSeq != -1;
}


