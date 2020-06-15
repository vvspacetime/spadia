#include "server.h"
#include "stun_packet.h"

static void buf_alloc_s(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->len = suggested_size * 32; // for recvmmsg
    buf->base = (char*)malloc(buf->len);
}

static void read_s(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    if (addr != nullptr) {
        sockaddr_in* addr_in = (sockaddr_in*)addr;
        if (nread > 0 && handle->data) {
            auto s = (Server*)handle->data;
            s->OnData(buf->base, nread, addr_in); // no copy need
        }
    }

    if (nread == 0 && addr == nullptr) {
        free(buf->base);
    }
}

static void send_u(uv_udp_send_t* req, int status) {

}


Server::Server(uint32_t port) : port_(port) {
    LOG(INFO) << "Server(" << port << ")";
    loop_ = Pool::GetInstance().GetLoop();
    socket_.data = this;
    // todo run in loop
    sleep(2);
    uv_udp_init(loop_, &socket_);
    uv_ip4_addr("0.0.0.0", port, &sockAddr_);
    uv_udp_bind(&socket_, (const struct sockaddr *)&sockAddr_, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&socket_, buf_alloc_s, read_s);
}

void Server::Start() {
    LOG(INFO) << "Server Start!";
    uv_run(loop_, uv_run_mode::UV_RUN_DEFAULT);
}

void Server::OnData(char *base, size_t len, sockaddr_in *remote) {
    if (!StunPacket::IsStun((uint8_t*)base, len)) return;
    auto p = StunPacket::Parse((uint8_t*)base, len);
    char ipName[40];
    uv_ip4_name(remote, ipName, 40);
    VLOG(9) << "read_s from:" << ipName << " port:" << remote->sin_port << " len:" << len << " username:" << p->GetUsername();
    auto q = p->MakeResponse();

    auto it = pcTable.find(p->GetUsername());
    if (it != pcTable.end()) {
        uint8_t* data;
        int size = q->Serialize(data, it->second->GetLocalPassword());
        uv_buf_t buf[1];
        buf[0].len = size;
        buf[0].base = (char*)data;
        uv_udp_send(&sendT_, &socket_, buf, 1, (const sockaddr *) remote, send_u);
        // libuv will copy it
        free(data);
//        it->second->Active(remote);
    } else {
        VLOG(0) << "can't find pc with stun username:" << p->GetUsername();
    }
}