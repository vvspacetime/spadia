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
    if (nread > 0 && handle->data) {
        char ipName[40];
        uv_ip4_name((const sockaddr_in *)(addr), ipName, 40);
        VLOG(9) << "read_u from:" << ipName << " port:" << get_port(((const sockaddr_in *)addr)->sin_port) << " len:" << nread;
        auto pc = (PeerConnection *) handle->data;
        pc->OnSocketData(buf->base, nread, (sockaddr_in*)addr);
    }
    if (nread == 0 && addr == nullptr) {
        free(buf->base);
    }
}

void send_u(uv_udp_send_t* req, int status) {
    if (req->data) {
        free(req->data);
        req->data = nullptr;
    }

    delete req;
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
    port_ = Server::GetInstance().GetPort();
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

void PeerConnection::Start() {
    Pool::Async(loop_, [pc=shared_from_this(), this]() {
        sockaddr_in send_addr;
        uv_ip4_addr("0.0.0.0", port_, &send_addr);
        uv_udp_bind(&socket_, (const sockaddr*)&send_addr, UV_UDP_REUSEADDR);
        uv_udp_recv_start(&socket_, buf_alloc, read_u);
        uv_timer_start(&iceTimer, ontimer, 0, 2500);
    });
}

void PeerConnection::Active(sockaddr_in *rmtSock) {
    // todo udp connect
    return;
    if (!active_) {
        LOG(INFO) << "PeerConnection::Active | " << id_;
        // todo connect new sock addr
        active_ = true;
        Pool::Async(loop_, [pc=shared_from_this(), this, remote=*rmtSock]() {
            sockaddr_in send_addr;
            uv_ip4_addr("0.0.0.0", port_, &send_addr);
            uv_udp_bind(&socket_, (const sockaddr*)&send_addr, UV_UDP_REUSEADDR);
            char ipName[40];
            uv_ip4_name((const sockaddr_in *)(&remote), ipName, 40);
//            VLOG(9) << "PeerConnection::Active | " << "connect to:" << ipName << " port:" << get_port(remote.sin_port) << " id:" << id_;
            uv_udp_connect(&socket_, (const sockaddr*)&remote);
            uv_udp_recv_start(&socket_, buf_alloc, read_u);
            uv_timer_start(&iceTimer, ontimer, 0, 2500);
        });
    } else {
//        if (get_time_ms() - lastPacketRecvTimeMS_ <= 2000) { // if > 2s no data, switch connection
//            return;
//        }
//        Pool::Async(loop_, [pc=shared_from_this(), this, remote=*rmtSock]() {
//            char ipName[40];
//            uv_ip4_name((const sockaddr_in *)(&remote), ipName, 40);
//            VLOG(9) << "PeerConnection::Active | " << "switch connect to:" << ipName << " port:" << remote.sin_port << " id:" << id_;
//            uv_udp_connect(&socket_, nullptr); //disconnect
//            uv_udp_connect(&socket_, (const sockaddr*)&remote);
//        });
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
    LOG(INFO) << "PeerConnection::Destroy | id_:" << id_;
    Server::GetInstance().RemovePC(localUsername_, remoteUsername_);
}

void PeerConnection::handleIncomingRTCPData(uint8_t *data, ssize_t size) {
    VLOG(9) << "PeerConnection::handleIncomingRTCPData | size:" << size << " id:" << id_;
    std::vector<std::shared_ptr<RTCPPacketInterface>> packets;
    int cnt = RTCPPacket::Parse(data, size, packets);
    for (auto& p : packets) {
        sessionManager_->OnRecvRTCP(p);
    }
}

void PeerConnection::handleIncomingRTPData(uint8_t *data, ssize_t size) {
    VLOG(9) << "PeerConnection::handleIncomingRTPData | size:" << size << " id:" << id_;
    auto packet = std::make_shared<RTPPacket>();
    packet->Parse(data, size);
    sessionManager_->OnRecvRTP(packet);
    remoteStream_->OnRTP(packet);
}


void PeerConnection::OnSocketData(char *data, ssize_t size, sockaddr_in* rmtSock) {
    VLOG(9) << "PeerConnection::OnSocketData | size:" << size << " id:" << id_;
    lastRemote_ = *rmtSock;
    lastPacketRecvTimeMS_ = get_time_ms();
    if (StunPacket::IsStun((uint8_t*)data, size)) {
        auto p = StunPacket::Parse((uint8_t*)data, size);
        auto q = p->MakeResponse();
        uint8_t* sendData;
        auto sendSize = q->Serialize(sendData, localPassword_);
        uv_buf_t buf[1];
        buf[0].len = sendSize;
        buf[0].base = (char*)sendData;
        auto st = new uv_udp_send_t;
        st->data = sendData;
        uv_udp_send(st, &socket_, buf, 1, (const sockaddr*)&lastRemote_, send_u);
        return;
    }

    if (RTCPPacket::IsRTCP((const uint8_t*)data, size)) {
        handleIncomingRTCPData((uint8_t*)data, size);
    } else {
        handleIncomingRTPData((uint8_t*)data, size);
    }
}

void PeerConnection::OnMediaData(std::shared_ptr<RTPPacket> pkt) {
    VLOG(9) << "PeerConnection::OnMediaData | id:" << id_;
//    uint8_t* data;
//    std::vector<RTPExtension*> exts;
//    RTPSession session;
//    session.ssrc = pkt->SSRC();
//    session.pt = pkt->PayloadType();
//    session.seq = pkt->SeqNum();
//    int size = pkt->Serialize(data, exts, session);
    Pool::Async(loop_, [pc=shared_from_this(), this, pkt](){
        sendData(pkt->Data(), pkt->Size());
    });
}

void PeerConnection::sendData(uint8_t *data, ssize_t size) {
    VLOG(9) << "PeerConnection::sendData | id:" << id_ << " size:" << size;
    uv_buf_t buf[1];
    buf[0].len = size;
    buf[0].base = (char*)data;
    auto st = new uv_udp_send_t;
    st->data = data;
    uv_udp_send(st, &socket_, buf, 1, (const sockaddr*)&lastRemote_, send_u);
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
    sessionManager_ = std::make_unique<RTPSessionManager>(loop_, weak_from_this());

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
            if (sessionManager_->SetExtension(e["value"], e["uri"])) {
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
                    {"port", port_},
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
            } else if (media["type"] == "video") {
                remoteStream_->VideoSSRC() = ssrc;
            }
        }
        remoteUsername_ = media["iceUfrag"];
        remotePassword_ = media["icePwd"];
        answerMediaJ.push_back(answerMedia);
    }

    if (remoteStream_) {
        callback_(remoteStream_);
    }

    answerJ["media"] = answerMediaJ;
    auto answer = sdptransform::write(answerJ);
    auto s = shared_from_this();
    Server::GetInstance().AddPC(shared_from_this());
    return answer;
}

std::string PeerConnection::CreateOffer() {
    sessionManager_ = std::make_unique<RTPSessionManager>(loop_, weak_from_this());
    json j;
    sdptransform::write(j);
    return "";
}

void PeerConnection::SetRemoteSdp(const std::string &sdp) {
    Server::GetInstance().AddPC(shared_from_this());
    // read candidate from sdp
    Server::GetInstance().SendBindRequestForStart();
}