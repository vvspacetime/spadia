#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include "pool.h"
#include <cstring>
#include <memory>
#include "peerconnection.h"
#include <map>
#include <glog/logging.h>

class Server {
public:
    Server(uint32_t port);
    ~Server() {

    }
    static Server& GetInstance(uint32_t port = 0) {
        static Server s(port);
        return s;
    }

    /**
     * start thread loop
     */
    void Start();

    virtual void OnData(char* base, size_t len, sockaddr_in* remote);

    void SendBindRequestForStart() {
        // send packet to sock
    }

    void AddPC(std::shared_ptr<PeerConnection> pc) {
        std::string name = pc->GetLocalUsername() + ":" + pc->GetRemoteUsername();
        LOG(INFO) << "AddPC:" << name;
        pcTable[name] = pc;
    }

    void RemovePC(const std::string& localUsername, const std::string& remoteUsername) {
        std::string name = localUsername + ":" + remoteUsername;
        LOG(INFO) << "RemovePC:" << name;
        pcTable.erase(name);
    }

    const std::string& GetCandidateIp() {
        return candidateIp_;
    }

    const uint32_t GetPort() {
        return port_;
    }
private:
    uv_loop_t* loop_;
    std::map<std::string, std::shared_ptr<PeerConnection>> pcTable;
    sockaddr_in sockAddr_; // don't connect, only for recv stun
    uv_udp_t socket_;
    std::string candidateIp_ = "127.0.0.1";
    uint32_t port_ = 8080;
    uv_udp_send_t sendT_;
};

#endif //TEST_SERVER_H
