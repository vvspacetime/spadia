#ifndef TEST_STUN_PACKET_H
#define TEST_STUN_PACKET_H

#include <string>
#include <atomic>
#include <memory>

class StunPacket {
public:
    enum Type  {Request=0,Indication=1,Response=2,Error=3};
    enum Method {Binding=1};
    enum AttributeType {
        Username = 0x0006,
        MessageIntegrity = 0x0008,
    };
    static std::unique_ptr<StunPacket> CreateRequest(std::string& localUsername, std::string& remoteUsername, std::string& remotePwd);
    static std::unique_ptr<StunPacket> Parse(uint8_t* data, int size);
    static bool IsStun(const uint8_t* data, int size);
    StunPacket();
    int Serialize(uint8_t *&data, const std::string &pwd);
    std::unique_ptr<StunPacket> MakeResponse();
    const std::string& GetUsername() {
        return username_;
    }
private:
    std::string username_;
    Method method_;
    Type type_;
    uint8_t transId_[12];
};

#endif //TEST_STUN_PACKET_H
