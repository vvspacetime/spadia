#ifndef TEST_RTCP_PACKET_H
#define TEST_RTCP_PACKET_H

#include <memory>
#include <vector>
#include <byte_io.h>
#include <cstring>
#include <map>
#include <tools.h>

constexpr int RTCPCommonHeaderSize = 4;
constexpr int SenderReportSize = 24;
constexpr int ReceiverReportSize = 24;

class RTCPPacketInterface {
public:
    virtual int Type() = 0;
    virtual ssize_t Serialize(uint8_t* &data) = 0;
};

class RTCPCommonHeader
{
public:
    RTCPCommonHeader() = default;

    uint8_t Parse(const uint8_t * data,const uint32_t size) {
        if (size < RTCPCommonHeaderSize)
            return 0;
        /*
            0                   1                   2                   3
            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |V=2|P|   FMT   |       PT      |          length               |
         */
        version		= data[0] >> 6;
        padding		= (data[0] >> 4 ) & 0x01;
        count		= data[0] & 0x1F;
        packetType	= data[1];
        // data[2] express 32bit's length exclude header
        length		= (webrtc::ByteReader<uint16_t>::ReadBigEndian(data + 2) + 1) << 2;
        return RTCPCommonHeaderSize;
    }
    uint32_t Serialize(uint8_t * data,const uint32_t size) const {
        if (size < RTCPCommonHeaderSize)
            return 0;

        data[0] = (padding ? 0xA0 : 0x80) | (count & 0x1F);
        data[1] = packetType;
        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + 2, (length >> 2) - 1);
        return RTCPCommonHeaderSize;
    }
    void Dump() const {

    }
public:
    uint8_t count	= 0; /* varies by packet type :5*/
    bool padding	= 0; /* padding flag */
    uint8_t version	= 2; /* protocol version :2 */
    uint8_t packetType = 0; /* RTCP packet type */
    uint16_t length	= 0; /* pkt len in words, w/o this word */
};

class RTCPReceiverReportBlock {
public:
    uint32_t GetSSRC()			const { return webrtc::ByteReader<uint32_t>::ReadBigEndian(buffer);  }
    uint8_t GetFactionLost()		const { return webrtc::ByteReader<uint8_t>::ReadBigEndian(buffer+4);  }
    uint32_t GetLostCount()		const { return webrtc::ByteReader<uint32_t>::ReadBigEndian(buffer+5) & 0x007FFFFF;  }
    uint32_t GetLastSeqNum()		const { return webrtc::ByteReader<uint32_t>::ReadBigEndian(buffer+8);  }
    uint32_t GetJitter()		const { return webrtc::ByteReader<uint32_t>::ReadBigEndian(buffer+12); }
    uint32_t GetLastSR()		const { return webrtc::ByteReader<uint32_t>::ReadBigEndian(buffer+16); }
    uint32_t GetDelaySinceLastSR()	const { return webrtc::ByteReader<uint32_t>::ReadBigEndian(buffer+20); }
    uint32_t GetSize()			const { return 24; }

    void SetSSRC(uint32_t ssrc)		{ webrtc::ByteWriter<uint8_t>::WriteBigEndian(buffer, ssrc);		}
    void SetFractionLost(uint8_t fraction)	{ webrtc::ByteWriter<uint8_t>::WriteBigEndian(buffer+4,fraction);	}
    void SetLostCount(uint32_t count)		{ webrtc::ByteWriter<uint32_t>::WriteBigEndian(buffer+5,count & 0x7FFFFF);		}
    void SetLastSeqNum(uint32_t seq)		{ webrtc::ByteWriter<uint32_t>::WriteBigEndian(buffer+8, seq);		}
    void SetLastJitter(uint32_t jitter)	{ webrtc::ByteWriter<uint32_t>::WriteBigEndian(buffer+12, jitter);	}
    void SetLastSR(uint32_t last)		{ webrtc::ByteWriter<uint32_t>::WriteBigEndian(buffer+16, last);		}
    void SetDelaySinceLastSR(uint32_t delay)	{ webrtc::ByteWriter<uint32_t>::WriteBigEndian(buffer+20, delay);	}

    void SetDelaySinceLastSRMS(uint32_t ms)
    {
        //calculate the delay, expressed in units of 1/65536 seconds
        uint32_t dlsr = (ms/1000) << 16 | (uint32_t)((ms%1000)*65.535);
        //Set it
        SetDelaySinceLastSR(dlsr);
    }

    uint32_t GetDelaySinceLastSRMS()
    {
        //Get the delay, expressed in units of 1/65536 seconds
        uint32_t dslr = GetDelaySinceLastSR();
        //Return in milis
        return (dslr>>16)*1000 + ((double)(dslr & 0xFFFF))/65.635;
    }


    uint32_t Serialize(uint8_t *data,uint32_t size)
    {
        //Check size
        if (size<24)
            return 0;
        //Copy
        memcpy(data,buffer,24);
        //Return size
        return 24;
    }

    uint32_t Parse(const uint8_t* data,uint32_t size)
    {
        //Check size
        if (size<24)
            return 0;
        //Copy
        memcpy(buffer,data,24);
        //Return size
        return 24;
    }

    void Dump()
    {
//        Debug("\t\t[Report ssrc=%u\n",		GetSSRC());
//        Debug("\t\t\tfractionLost=%u\n",	GetFactionLost());
//        Debug("\t\t\tlostCount=%u\n",		GetLostCount());
//        Debug("\t\t\tlastSeqNum=%u\n",		GetLastSeqNum());
//        Debug("\t\t\tjitter=%u\n",		GetJitter());
//        Debug("\t\t\tlastSR=%u\n",		GetLastSR());
//        Debug("\t\t\tdelaySinceLastSR=%u\n",	GetDelaySinceLastSR());
//        Debug("\t\t/]\n");
    }
private:
    uint8_t buffer[24] = {0};
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
    static int Parse(uint8_t * data, ssize_t len, std::vector<std::shared_ptr<RTCPPacketInterface>>& output);
};

class SendReportPacket: public RTCPPacketInterface {
public:
    SendReportPacket(uint8_t* data, ssize_t size) {
        Parse(data, size);
    }
    SendReportPacket() {

    }
    ssize_t Serialize(uint8_t* &data) override {
        // todo report

        data = (uint8_t*)malloc(RTCPCommonHeaderSize + SenderReportSize);
        RTCPCommonHeader header;
        header.count	  = reports.size();
        header.packetType = Type();
        header.padding	  = false;
        header.length	  = RTCPCommonHeaderSize + SenderReportSize;
        uint32_t len = header.Serialize(data, RTCPCommonHeaderSize + SenderReportSize);

        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, ssrc);
        len += 4;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, ntpSec);
        len += 4;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, ntpFrac);
        len += 4;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, rtpTimestamp);
        len += 4;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, packetsSent);
        len += 4;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, octectsSent);
        len += 4;
        return len;
    }

    int Type() override {
        return RTCPPacket::SenderReport;
    }

    uint32_t GetNTPTimestamp() const {
        return ((uint64_t)ntpSec)<<32 | ntpFrac;
    }

    void SetTimestamp(uint64_t time) {
        // 1970 -> 1900
        uint32_t sec = time/1E6;
        ntpSec = sec + 2208988800UL;
        // frac = usec/1E6 * 2^32
        ntpFrac = (time - sec*1E6) * 4294.967296;
    }

    uint64_t GetTimestamp() {
        uint64_t ts = ntpSec - 2208988800UL;
        ts *= 1E6;
        ts += ntpFrac / 4294.967296;
        return ts;
    }
private:
    void Parse(uint8_t* data, ssize_t size) {
        RTCPCommonHeader header;
        uint32_t len = header.Parse(data,size);
        if (!len) {
            return;
        }
        uint32_t packetSize = header.length;
        if (size < packetSize) return;

        ssrc = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        ntpSec = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        ntpFrac = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        rtpTimestamp = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        packetsSent	= webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        octectsSent	= webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        // todo report
        return;
    }

public:
    uint32_t ssrc = 0; /* sender generating this report */
    uint32_t ntpSec	= 0; /* NTP timestamp */
    uint32_t ntpFrac		= 0;
    uint32_t rtpTimestamp	= 0; /* RTP timestamp */
    uint32_t packetsSent	= 0; /* packets sent */
    uint32_t octectsSent	= 0; /* octets sent */

    std::vector<RTCPReceiverReportBlock> reports;
};

class ReceiverReportPacket: public RTCPPacketInterface {
public:
    ReceiverReportPacket(uint8_t* data, ssize_t size) {

    }
    ReceiverReportPacket() {

    }
    ssize_t Serialize(uint8_t* &data) override {
        uint32_t packetSize = RTCPCommonHeaderSize + sizeof(ssrc) + ReceiverReportSize;
        data = (uint8_t*)malloc(packetSize);

        //RTCP common header
        RTCPCommonHeader header;
        header.count	  = reports.size();
        header.packetType = Type();
        header.padding	  = 0;
        header.length	  = packetSize;
        uint32_t len = header.Serialize(data, packetSize);

        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, ssrc);
        len += 4;
        for(int i = 0;i < header.count; i++)
            len += reports[i].Serialize(data + len, packetSize - len);
        return len;
    }

    int Type() override {
        return RTCPPacket::ReceiverReport;
    }

private:
    void Parse(uint8_t* data, ssize_t size) {
        //Get header
        RTCPCommonHeader header;
        uint32_t len = header.Parse(data,size);

        if (!len) return;
        uint32_t packetSize = header.length;
        if (size < packetSize) return;

        ssrc = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        for(int i = 0; i < header.count && len + 24 <= size; i ++)
        {
            auto report = RTCPReceiverReportBlock();
            len += report.Parse(data+len,size-len);
            reports.push_back(std::move(report));
        }
    }

public:
    uint32_t ssrc;
    std::vector<RTCPReceiverReportBlock> reports;
};

class RTCPRTPFeedbackPacket: public RTCPPacketInterface {
public:
    enum FeedbackType {
        NACK = 1,
        TransportWideFeedbackMessage = 15,
    };
    class FeedBackField {
    public:
        virtual ssize_t Serialize(uint8_t* data) = 0;
        virtual void Parse(uint8_t* data, ssize_t size) = 0;
        virtual FeedbackType Type() = 0;
//        virtual ssize_t GetSize() const = 0;
    };

    class TransportCCFeedBack : public FeedBackField {
    public:
        enum PacketStatus
        {
            NotReceived = 0,
            SmallDelta = 1,
            LargeOrNegativeDelta = 2,
            Reserved = 3
        };

        virtual void Parse(uint8_t* data, ssize_t size) {
            //Create the  status count
            std::vector<PacketStatus> statuses;

            if (size < 8) return;

            uint32_t baseSeqNumber = webrtc::ByteReader<uint32_t>::ReadBigEndian(data);
            uint16_t packetStatusCount = webrtc::ByteReader<uint16_t>::ReadBigEndian(data + 2);
            referenceTime = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + 4) & 0xFFFFFF00u;
            feedbackPacketCount	= webrtc::ByteReader<uint8_t>::ReadBigEndian(data + 7);
            //Rseserve initial space
            statuses.reserve(packetStatusCount);
            uint32_t len = 8;
            //Get all
            while (statuses.size() < packetStatusCount) {
                if (len + 2 > size)
                    return;

                uint16_t chunk = webrtc::ByteReader<uint16_t>::ReadBigEndian(data + len);
                len += 2;

                //Check packet type
                if (chunk >> 15u)
                {
                    /*
                        0                   1
                        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
                           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                           |T|S|       symbol list         |
                           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                         T = 1
                    */
                    if (chunk >> 14u & 1u) {
                        // two bit status
                        uint16_t flag = 0x3000u;
                        for (uint32_t j = 0; j < 7; ++ j) {
                            auto status = (PacketStatus)((chunk >> (12 - 2 * j)) & 0x03u);
                            statuses.push_back(status);
                        }
                    } else {
                        // one bit status
                        for (uint32_t j = 0; j < 14; ++ j) {
                            auto status = (PacketStatus)((chunk >> (13 - j)) & 0x01u);
                            statuses.push_back(status);
                        }
                    }
                } else {
                    /*
                     * same status
                        0                   1
                        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
                           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                           |T| S |       Run Length        |
                           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        T = 0
                     */
                    auto status = (PacketStatus)(chunk >> 13) ;
                    uint16_t run = chunk & 0x1FFF;
                    for (uint16_t j = 0; j < run; ++ j)
                        statuses.push_back(status);
                }
            }
            uint64_t time = referenceTime * 64000; // us
            for (uint32_t i = 0; i < packetStatusCount; ++ i) {
                switch (statuses[i]) {
                    case PacketStatus::NotReceived: {
                        // don't read any bit
                        packets[baseSeqNumber + i] = 0;
                        break;
                    }
                    case PacketStatus::SmallDelta: {
                        if (len + 1 > size)
                            return;
                        int delta = webrtc::ByteReader<uint8_t>::ReadBigEndian(data + len)* 250 ;
                        time += delta;
                        len += 1;
                        packets[baseSeqNumber+i] = time;
                        break;
                    }
                    case PacketStatus::LargeOrNegativeDelta: {
                        //Check size
                        if (len + 2 > size)
                            return;
                        short aux = webrtc::ByteReader<short>::ReadBigEndian(data + len);
                        int delta = aux * 250;
                        len += 2;
                        time += delta;
                        packets[baseSeqNumber + i] = time;
                        break;
                    }
                    case PacketStatus::Reserved:
                        //Ignore
                        break;
                }
            }
            len = pad32(len);
        }

        ssize_t Serialize(uint8_t* data) override {
            if (packets.empty()) return 0;
            int len = 0;
            uint16_t baseSeqNumber	= packets.begin()->first;
            bool firstReceived	= false;
            uint64_t referenceTime	= 0;
            uint16_t packetStatusCount	= packets.size();

            /*
                0                   1                   2                   3
                0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                   |      base sequence number     |      packet status count      |
                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                   |                 reference time                | fb pkt. count |
                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
             */
            webrtc::ByteWriter<uint16_t>::WriteBigEndian(data, baseSeqNumber);
            webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + 2, packetStatusCount);
            len += 4;
            // set3 referenceTime and feedbackPacketCount when first received
            uint64_t time = 0; // us
            std::vector<int> deltas;
            std::vector<PacketStatus> statuses;

            for (auto it = packets.begin(); it != packets.end(); ++ it) {
                PacketStatus status = PacketStatus::NotReceived;
                if (it->second) {
                    int delta = 0;
                    if (!firstReceived) {
                        firstReceived = true;
                        referenceTime = (it->second/64000) & 0x7FFFFF;
                        time = referenceTime * 64000;
                        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + 4, referenceTime << 8u);
                        webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + 7, feedbackPacketCount);
                        len += 4;
                    }

                    if (it->second > time)
                        delta = (it->second - time) / 250; // todo first delta not zero
                    else
                        delta = -(int)((time - it->second) / 250);
                    if (delta < 0 || delta > 127)
                        status = PacketStatus::LargeOrNegativeDelta;
                    else
                        status = PacketStatus::SmallDelta;
                    deltas.push_back(delta);
                    time = time + delta * 250;
                }

                statuses.push_back(status);
            }

            for (int i = 0; i < statuses.size(); ) {
                /*
                 * 	 0                   1
					 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
                    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                    |T|S|        Symbols            |
                    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
					T = 1
					S = 1
                 */
                uint16_t chunk = 0xC000;
                for (int j = 0; j < 7 && i + j < statuses.size(); j ++) {
                    auto state = statuses[i + j];
                    chunk |= (state << (12 - 2*j));
                }
                webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + len, chunk);
                len += 2;
                i += 7;
            }

            //Write now the deltas
            for (auto it = deltas.begin(); it != deltas.end(); ++ it)
            {
                //Check size
                if (*it < 0 || *it > 127)
                {
                    webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + len, (short)*it);
                    len += 2;
                } else {
                    webrtc::ByteWriter<uint8_t>::WriteBigEndian(data + len, (uint8_t)*it);
                    len ++;
                }
            }

            //Add zero padding
            while (len % 4)
                data[len++] = 0;
            return len;
        }

        FeedbackType Type() override {
            return TransportWideFeedbackMessage;
        }

        void InsertPacket(uint32_t transportNum, uint64_t timeUS) {
            packets[transportNum] = timeUS;
        }

        uint8_t feedbackPacketCount;
        uint32_t referenceTime = 0;
    private:
        std::map<uint32_t, uint64_t> packets;
    };

    class NackFeedBack: public FeedBackField {
    public:
        virtual void Parse(uint8_t* data, ssize_t size) override {
            ssrc = webrtc::ByteReader<uint32_t>::ReadBigEndian(data);
            fsn = webrtc::ByteReader<uint16_t>::ReadBigEndian(data + 4);
            blp = webrtc::ByteReader<uint16_t>::ReadBigEndian(data + 6);
        }

        ssize_t Serialize(uint8_t* data) override {
            int len = 0;
            webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, ssrc);
            len += 4;
            webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + len, fsn);
            len += 2;
            webrtc::ByteWriter<uint16_t>::WriteBigEndian(data + len, blp);
            len += 2;
            return len;
        }

        FeedbackType Type() {
            return NACK;
        }

        uint32_t ssrc;
        uint16_t fsn;
        uint16_t blp;
    };


    RTCPRTPFeedbackPacket() {

    }
    RTCPRTPFeedbackPacket(uint8_t* data, ssize_t size) {
        RTCPCommonHeader header;
        uint32_t len = header.Parse(data,size);
        Parse(data + RTCPCommonHeaderSize, size);
    }

    ssize_t Serialize(uint8_t* &data) override {
        const int enough = 1200;
        data = (uint8_t*)malloc(enough);
        // write header at last
        int len = RTCPCommonHeaderSize;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, 0);
        len += 4;
        webrtc::ByteWriter<uint32_t>::WriteBigEndian(data + len, ssrc_);
        len += 4;

        int fieldSize = field_->Serialize(data + len);
        RTCPCommonHeader header;
        header.packetType = RTCPPacket::RTPFeedback;
        header.length = fieldSize + len;
        header.count = field_->Type();
        header.Serialize(data, enough);
        return header.length;
    }

    int Type() override {
        return RTCPPacket::RTPFeedback;
    }

    void SetField(std::unique_ptr<FeedBackField>&& field) {
        field_ = std::move(field);
    }

    void SetSSRC(uint32_t ssrc) {
        ssrc_ = ssrc;
    }
private:
    void Parse(uint8_t* data, ssize_t size) {
        /*
        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |V=2|P|   FMT   |       PT      |          length               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                  SSRC of packet sender                        |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                  SSRC of media source                         |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       :            Feedback Control Information (FCI)                 :
         */
        int len = 0;
        RTCPCommonHeader header;
        header.Parse(data + len, size);
        len += RTCPCommonHeaderSize;
        len += 4; // skip ssrc of packet sender
        ssrc_ = webrtc::ByteReader<uint32_t>::ReadBigEndian(data + len);
        len += 4;
        if (header.count == NACK) {
            field_ = std::unique_ptr<NackFeedBack>(new NackFeedBack);
        } else if (header.count == TransportWideFeedbackMessage) {
            field_ = std::unique_ptr<TransportCCFeedBack>(new TransportCCFeedBack);
        }
        field_->Parse(data + len, size);
    }
    std::unique_ptr<FeedBackField> field_;
    uint32_t ssrc_;
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
