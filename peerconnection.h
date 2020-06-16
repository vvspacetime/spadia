#ifndef TEST_PEERCONNECTION_H
#define TEST_PEERCONNECTION_H
extern "C" {
#include <uv.h>
}
#include "pool.h"
#include <string>
#include <functional>
#include "media_stream.h"
#include <cstring>
#include "rtp_session_manager.h"
#include "tools.h"

constexpr uint64_t IceTimeout = 10000; // 10s


class PeerConnection : public MediaStreamSink, public std::enable_shared_from_this<PeerConnection> {
public:
    PeerConnection();

    ~PeerConnection();

    void AddStream(std::shared_ptr<MediaStream>& stream) {
        localStream_ = stream;
        localStream_->AddSink(id_, weak_from_this());
    }

    void AddRemoteStreamListener(std::function<void (std::shared_ptr<MediaStream>)>&& callback) {
        callback_ = std::move(callback);
    }

    std::string CreateOffer();

    std::string CreateAnswer(const std::string& offer);

    // use with create offer
    void SetRemoteSdp(const std::string& sdp);

    /**
     * as MediaStreamSink, recv from mediastream
     */
    virtual void OnMediaData(std::shared_ptr<RTPPacket> pkt) override;

    /**
     * from socket
     */
    void OnSocketData(char* data, ssize_t size, sockaddr_in* rmtSock);

    void Start();
    /**
    *   when receive binding response
    */
    void Active(sockaddr_in* rmtSock);

    const std::string& GetLocalUsername() {
        return localUsername_;
    }

    const std::string& GetRemoteUsername() {
        return remoteUsername_;
    }

    const std::string& GetLocalPassword() {
        return localPassword_;
    }

    const std::string& GetRemotePassword() {
        return remotePassword_;
    }

    void OnIceTimer();

    void Destroy();
private:
    void sendBindRequest() {
        // build binding request
    }
    void sendData(uint8_t* data, ssize_t size);

    void handleIncomingRTCPData(uint8_t* data, ssize_t size);
    void handleIncomingRTPData(uint8_t* data, ssize_t size);
private:
    RTPSessionManager sessionManager;
    std::string id_;
    uv_udp_t socket_;
    uv_timer_t iceTimer;
    uv_loop_t* loop_;
    std::function<void (std::shared_ptr<MediaStream>)> callback_;
    std::shared_ptr<MediaStream> localStream_;
    std::shared_ptr<MediaStream> remoteStream_;
    std::string localUsername_ = randomID();
    std::string localPassword_ = randomID();
    std::string remoteUsername_;
    std::string remotePassword_;
    bool active_;
    uint64_t lastPacketRecvTimeMS_ = 0;
    uint16_t port_;
    sockaddr_in lastRemote_;
};


#endif //TEST_PEERCONNECTION_H
