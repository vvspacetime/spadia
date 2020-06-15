#ifndef TEST_RTCP_PACKET_H
#define TEST_RTCP_PACKET_H

#include <memory>
#include <vector>
#include <byte_io.h>

class RTCPPacketInterface {
    virtual int Type() = 0;
    virtual ssize_t Serialize(uint8_t* &data) = 0;
};

class RTCPPacket {
public:
    enum Type {
        ExtendedJitterReport = 195,
        SenderReport = 200,
        ReceiverReport = 201,
        SDES = 202,
        Bye = 203,
        App = 204,
        RTPFeedback = 205,
        PayloadFeedback = 206,
        XR = 207
    };
    static bool IsRTCP(const uint8_t* data, ssize_t size) {
        return size >= 4 && (data[1]&0x7f) >= 64 && (data[1]&0x7f) < 96;
    }
    static std::vector<std::shared_ptr<RTCPPacketInterface>> Parse(uint8_t * data, ssize_t len);
};

class SendReportPacket: public RTCPPacketInterface {
public:
    SendReportPacket(uint8_t* data, ssize_t size) {
        Parse(data, size);
    }
    SendReportPacket() {

    }
    ssize_t Serialize(uint8_t* &data) override {

    }
    int Type() override {
        return RTCPPacket::SenderReport;
    }
private:
    void Parse(uint8_t* data, ssize_t size) {

    }
};

class ReceiverReportPacket: public RTCPPacketInterface {
public:
    ReceiverReportPacket(uint8_t* data, ssize_t size) {

    }
    ssize_t Serialize(uint8_t* &data) override;
    int Type() override {
        return RTCPPacket::ReceiverReport;
    }
};

class RTCPRTPFeedbackPacket: public RTCPPacketInterface {
public:
    enum FeedbackType {
        NACK = 1,
        TransportWideFeedbackMessage = 15,
    };
    ssize_t Serialize(uint8_t* &data) override;
    int Type() override {
        // todo
    }
};

class PayloadFeedbackPacket: public RTCPPacketInterface {
public:
    enum FeedbackType {
        PictureLossIndication = 1,
        FullIntraRequest = 4,
    };
    ssize_t Serialize(uint8_t* &data) override;
    int Type() override {
        // todo
    }
};

class XRPacket: public RTCPPacketInterface {
    ssize_t Serialize(uint8_t* &data) override;
    int Type() override {
        // todo
    }
};





#endif //TEST_RTCP_PACKET_H
