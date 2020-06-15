#ifndef TEST_MEDIA_STREAM_H
#define TEST_MEDIA_STREAM_H
#include <memory>
#include <map>
#include <unordered_map>
#include "rtp_packet.h"

class MediaStreamSink {
public:
    virtual void OnMediaData(std::shared_ptr<RTPPacket> pkt) = 0;
};

class MediaStream {
public:
    enum MediaType {
        AUDIO = 0,
        VIDEO = 1
    };
    MediaStream();

    void SetupWithFile(const std::string& filename) {
        // loop read file and
    }

    virtual void AddSink(const std::string& name, std::weak_ptr<MediaStreamSink> sink) {
        sinks_[name] = sink;
    }

    virtual void RemoveSink(const std::string& name) {
        sinks_.erase(name);
    }

    /**
     * pkt from pc
     */
    virtual void OnRTP(std::shared_ptr<RTPPacket> pkt) {
        for (auto it : sinks_) {
            auto s = it.second.lock();
            if (s) {
                s->OnMediaData(pkt);
            }
        }
    }

    uint32_t& AudioSSRC() {
        return audioSSRC_;
    }

    uint32_t& VideoSSRC() {
        return videoSSRC_;
    }

private:
    std::unordered_map<std::string, std::weak_ptr<MediaStreamSink>> sinks_;
    uint32_t audioSSRC_ = 0;
    uint32_t videoSSRC_ = 0;
};


#endif //TEST_MEDIA_STREAM_H
