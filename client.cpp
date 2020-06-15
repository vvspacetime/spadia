//#include <iostream>
//#include <memory>
//#include <cstring>
//#include <thread>
//
//extern "C" {
//    #include <uv.h>
//}
//
//uv_loop_t *loop;
//uv_udp_t send_socket;
//uv_udp_t recv_socket;
//
//
//void buf_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
//    buf->len = suggested_size;
//    buf->base = (char*)malloc(suggested_size);
//}
//
//void read(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
//    if (nread > 0) {
//        char *data = (char*)malloc(nread + 1);
//        memcpy(data, buf->base, nread);
//        data[nread] = '\0';
//
//        printf("recv:%s,size:%d\n", data, nread);
//    }
//}
//
//void send(uv_udp_send_t* req, int status) {
//    printf("send:%d\n", status);
//}
//
//int main() {
//    loop = uv_default_loop();
//
//    uv_udp_init(loop, &recv_socket);
//    struct sockaddr_in recv_addr;
//    uv_ip4_addr("0.0.0.0", 8080, &recv_addr);
//    uv_udp_bind(&recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
//    uv_udp_recv_start(&recv_socket, buf_alloc, read);
//
//    uv_udp_init(loop, &send_socket);
//
//    uv_udp_send_t send_req;
//
//    uv_buf_t send_msg[] = { {"123", 3}, };
//    struct sockaddr_in send_addr;
//    uv_ip4_addr("127.0.0.1", 8080, &send_addr);
//    struct sockaddr_in send_bind_addr;
//    uv_ip4_addr("127.0.0.1", 1000, &send_bind_addr);
//    uv_udp_bind(&send_socket, (const struct sockaddr *)&send_bind_addr, UV_UDP_MMSG_CHUNK);
//    int rv = uv_udp_connect(&send_socket, (const struct sockaddr *)&send_addr);
//    printf("rv: %d\n", rv);
//    uv_udp_send(&send_req, &send_socket, send_msg, 1, nullptr, send); // for connected, addr is nullptr
//
//    return uv_run(loop, UV_RUN_DEFAULT);
//}