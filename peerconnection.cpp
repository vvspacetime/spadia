#include "peerconnection.h"
#include "server.h"
#include "rtcp_packet.h"
#include <glog/logging.h>
#include <sdptransform/sdptransform.hpp>
#include "tools.h"
#include "stun_packet.h"

void buf_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->len = suggested_size * 32; // for recvmmsg
    buf->base = (char*)malloc(buf->len);
}

void read_u(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    printf("read: %lu\n", nread);
    if (nread > 0 && handle->data) {
        auto pc = (PeerConnection *) handle->data;
        pc->OnSocketData(buf->base, nread, (sockaddr_in*)addr);
    }
    if (nread == 0 && addr == nullptr) {
        free(buf->base);
    }
}

void send_u(uv_udp_send_t* req, int status) {

}

void ontimer(uv_timer_t* handle) {
    auto pc = (PeerConnection*)handle->data;
    pc->OnIceTimer();
}

PeerConnection::PeerConnection() {
    id_ = uuid();
    VLOG(2) << "PeerConnection() " << id_;
    loop_ = Pool::GetInstance().GetLoop();
    uv_timer_init(loop_, &iceTimer);
    uv_udp_init(loop_, &socket_);

    socket_.data = this;
    iceTimer.data = this;
}



PeerConnection::~PeerConnection() {
    VLOG(2) << "~PeerConnection() " << id_;
    if (localStream_) {
        localStream_->RemoveSink(id_);
    }
    uv_udp_recv_stop(&socket_);
    socket_.data = nullptr;
    iceTimer.data = nullptr;
}


void PeerConnection::Active(sockaddr_in *rmtSock) {
    if (!active_) {
        LOG(INFO) << "PeerConnection::Active | " << id_;
        // todo connect new sock addr
        active_ = true;
        Pool::Async(loop_, [pc=shared_from_this(), this, remote=&rmtSock]() {
            sockaddr_in send_addr;
            uv_ip4_addr("0.0.0.0", Server::GetInstance().GetPort(), &send_addr);
            uv_udp_bind(&socket_, (const sockaddr*)&send_addr, UV_UDP_REUSEADDR);
            uv_udp_connect(&socket_, (const sockaddr*)&remote);
            uv_udp_recv_start(&socket_, buf_alloc, read_u);
            uv_timer_start(&iceTimer, ontimer, 0, 2500);
        });
    }
}

void PeerConnection::OnIceTimer() {
    sendBindRequest();
    auto now = get_time_ms();
    if (lastPacketRecvTimeMS_ == 0) {
        lastPacketRecvTimeMS_ = now;
        return;
    }

    // timeout judge
    if (now - lastPacketRecvTimeMS_ > IceTimeout) {
        LOG(INFO) << "PeerConnection::OnIceTimer | timeout " << id_;
//        Destroy();
    }
}

void PeerConnection::Destroy() {
    LOG(INFO) << "PeerConnection::Destroy | id_";
    Server::GetInstance().RemovePC(localUsername_, remoteUsername_);
}

void PeerConnection::handleIncomingRTCPData(uint8_t *data, ssize_t size) {
    VLOG(9) << "PeerConnection::handleIncomingRTCPData | size:" << size;
    auto packets = RTCPPacket::Parse(data, size);
    for (auto& pkt : packets) {
        // todo response xr
        // todo transmit fir to sender
    }
}

void PeerConnection::handleIncomingRTPData(uint8_t *data, ssize_t size) {
    VLOG(9) << "PeerConnection::handleIncomingRTPData | size:" << size;
    auto packet = std::make_shared<RTPPacket>();
    packet->Parse(data, size);

    remoteStream_->OnRTP(packet);
    // todo send rr, rr = remoteStream.GetRR()
    // todo send nack = remoteStream.GetNack()
    // todo send transport feedback
}


void PeerConnection::OnSocketData(char *data, ssize_t size, sockaddr_in* rmtSock) {
    VLOG(9) << "PeerConnection::OnSocketData | size:" << size;
    if (StunPacket::IsStun((uint8_t*)data, size)) {
        auto p = StunPacket::Parse((uint8_t*)data, size);
        auto q = p->MakeResponse();
        uint8_t* data;
        int size = q->Serialize(data, localPassword_);
        uv_buf_t buf[1];
        buf[0].len = size;
        buf[0].base = (char*)data;
        uv_udp_send(&sendT_, &socket_, buf, 1, (const sockaddr *) rmtSock, send_u);
        return;
    }

    if (RTCPPacket::IsRTCP((const uint8_t*)data, size)) {
        handleIncomingRTCPData((uint8_t*)data, size);
    } else {
        handleIncomingRTPData((uint8_t*)data, size);
    }
}

void PeerConnection::OnMediaData(std::shared_ptr<RTPPacket> pkt) {
    uint8_t* data;
//    int size = pkt->Serialize(data, );
//    sendData(data, size);
}

void PeerConnection::sendData(uint8_t *data, ssize_t size) {

}

json createBaseSdp() {
    json j;
    j["groups"] = {{"mids", "0 1"}, {"type", "BUNDLE"}};
    j["name"] = "-";
    j["origin"] = {
            {"address", "127.0.0.1"},
            {"ipVer", 4},
            {"netType", "IN"},
            {"sessionId", 0},
            {"sessionVersion", 2},
            {"username", "-"}
    };
    j["timing"] = {
            {"start", 0},
            {"stop", 0}
    };
    j["version"] = 0;
    j["connection"] = {
            {"ip", "0.0.0.0"},
            {"version", 4},
    };
    return std::move(j);
}

json getSSRCs(uint32_t ssrc, const std::string& msid, const std::string& cname, const std::string& label) {
    return json::array({
                               {{"attribute", "cname"},
                                {"id", ssrc},
                                {"value", cname}},
                               {{"attribute", "msid"},
                                {"id", ssrc},
                                {"value", msid + " " + label}},
                               {{"attribute", "mslabel"},
                                {"id", ssrc},
                                {"value", label}},
                       });
}

std::string PeerConnection::CreateAnswer(const std::string &offer) {
    json offerJ = sdptransform::parse(offer);
    json answerJ = createBaseSdp();
    std::string msid = "*";
    if (localStream_) {
        msid = uuid();
    }
    std::string cname = uuid();
    answerJ["msidSemantic"] = {
            {"semantic", "WMS"},
            {"token", msid},
    };
    auto offerMediaJ = offerJ["media"];
    json answerMediaJ = json::array();
    for (auto& media : offerMediaJ) {
        char* direction;
        if (media["direction"] == "sendonly" || media["direction"] == "sendrecv") {
            direction = "recvonly";
        } else  {
            direction = "sendonly";
        }
        json aExt;
        for (auto& e: media["ext"]) {
            // if set success
            if (sessionManager.SetExtension(e["value"], e["uri"])) {
                aExt.push_back({
                    {"uri", e["uri"]},
                    {"value", e["value"]}
                });
            }
        }
        json aRtcpFb;
        for (auto& rtcp: media["rtcpFb"]) {
            if (rtcp["type"] == "transport-cc" || rtcp["type"] == "nack") {
                aRtcpFb.push_back(rtcp);
            }
        }
        json answerMedia = {
                {"connection", {
                    {"ip", "0.0.0.0"},
                    {"version", 4},}
                },
                {"direction", direction},
                {"ext", aExt},
                {"rtcpMux", "rtcp-mux"},
                {"rtcpRsize", "rtcp-rsize"},
                {"type", media["type"]},
                {"rtp", media["rtp"]},
                {"rtcpFb", aRtcpFb},
                {"fmtp", media["fmtp"]},
                {"payloads", media["payloads"]},
                {"port", 9},
                {"mid", media["mid"]},
//                {"msid", msid},
                {"icePwd", localPassword_},
                {"iceUfrag", localUsername_},
                {"protocol", media["protocol"]},
                {"candidates", json::array({{
                    {"component", 1},
                    {"foundation", "1"},
                    {"ip", Server::GetInstance().GetCandidateIp()},
                    {"port", Server::GetInstance().GetPort()},
                    {"priority", 33554431},
                    {"transport", "UDP"},
                    {"type", "host"}
                    }})
                }
        };

        if (media["direction"] == "recvonly" || media["direction"] == "sendrecv") {
            if (localStream_ && localStream_->AudioSSRC() && media["type"] == "audio") {
                auto ssrcs = getSSRCs(localStream_->AudioSSRC(), msid, cname, "audio0");
                answerMedia["ssrcs"] = ssrcs;
            } else if (localStream_ && localStream_->VideoSSRC() && media["type"] == "video") {
                auto ssrcs = getSSRCs(localStream_->VideoSSRC(), msid, cname, "video0");
                answerMedia["ssrcs"] = ssrcs;
            }
        }
        if (media["direction"] == "sendonly" || media["direction"] == "sendrecv") {
            // this peer only recv
            auto ssrcs = media["ssrcs"];
            uint32_t ssrc = ssrcs[0]["id"];
            if (!remoteStream_) {
                remoteStream_ = std::make_shared<MediaStream>();
            }
            if (media["type"] == "audio") {
                remoteStream_->AudioSSRC() = ssrc;
            }
        }
        remoteUsername_ = media["iceUfrag"];
        remotePassword_ = media["icePwd"];
        answerMediaJ.push_back(answerMedia);
    }
    answerJ["media"] = answerMediaJ;
    auto answer = sdptransform::write(answerJ);
    auto s = shared_from_this();
    Server::GetInstance().AddPC(shared_from_this());
    return answer;
}

std::string PeerConnection::CreateOffer() {
    json j;
    sdptransform::write(j);
    return "";
}

void PeerConnection::SetRemoteSdp(const std::string &sdp) {
    Server::GetInstance().AddPC(shared_from_this());
    // read candidate from sdp
    Server::GetInstance().SendBindRequestForStart();
}