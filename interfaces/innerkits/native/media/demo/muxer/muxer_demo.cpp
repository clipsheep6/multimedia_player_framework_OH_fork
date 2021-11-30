#include "muxer_demo.h"
#include <iostream>
#include <cstdio>
#include <securec.h>

namespace OHOS {
namespace Media {
std::vector<std::string> MDKey = {
    "track_index",
    "track_type",
    "codec_mime",
    "duration",
    "bitrate",
    "max-input-size",
    "width",
    "height",
    "pixel-format",
    "frame-rate",
    "capture-rate",
    "i_frame_interval",
    "req_i_frame"
    "channel-count",
    "sample-rate",
    "track-count",
    "vendor."
};

void MuxerDemo::WriteTrackSample() {
    AVFormatContext *fmt_ctx_;
    AVInputFormat *pstAVFmt_ = NULL;
    AVDictionary *pstOpts_ = NULL;
    fmt_ctx_ = avformat_alloc_context();
    const char *url = "/data/media/test.mp4";
    avformat_open_input(&fmt_ctx_, url, pstAVFmt_, &pstOpts_);
    AVPacket pkt;
    memset_s(&pkt, sizeof(pkt), 0, sizeof(pkt));

    while(1) {
        av_init_packet(&pkt);
        if (av_read_frame(fmt_ctx_, &pkt) < 0) {
            av_packet_unref(&pkt);
            break;
        }
        // double pts_time = pkt.pts * av_q2d(fmt_ctx_->streams[pkt.stream_index]->time_base);
        TrackSampleInfo info;
        info.timeUs = 0;
        info.size = 0;
        info.offset = 0;
        info.flags = PARTIAL_FRAME;
        info.trackIdx = 0;
        std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(pkt.data, pkt.size);
		// avMem->SetRange(info.offset, pkt.size);
        muxer_->WriteTrackSample(avMem, info);
    }
}

void MuxerDemo::DoNext()
{
    std::string cmd;
    do {
        std::cout << "Enter your step:" << std::endl;
        std::cin >> cmd;

        if (cmd == "SetOutput") {
            std::string path;
            std::string format;
            std::cout << "Enter path:" << std::endl;
            std::cin >> path;
            std::cout << "Enter format:" << std::endl;
            std::cin >> format;
            muxer_->SetOutput(path, format);
            continue;
        } else if (cmd == "SetLocation") {
            float latitude;
            float longitude;
            std::cout << "Enter latitude:" << std::endl;
            std::cin >> latitude;
            std::cout << "Enter longitude:" << std::endl;
            std::cin >> longitude;
            muxer_->SetLocation(latitude, longitude);
            continue;
        } else if (cmd == "SetOrientationHint") {                                                    
            int degrees;
            std::cout << "Enter degrees:" << std::endl;
            std::cin >> degrees;
            muxer_->SetOrientationHint(degrees);
            continue;
        } else if (cmd == "AddTrack") {
            MediaDescription trackDesc;
            for (auto s : MDKey) {
                std:: cout << "Enter " << s << std::endl;
                if (s == "mime-type") {
                    std::string value;
                    std::cin >> value;
                    trackDesc.PutStringValue(s, value);
                } else {
                    int32_t value;
                    std::cin >> value;
                    trackDesc.PutIntValue(s, value);
                }
            }
            int32_t trackId;
            muxer_->AddTrack(trackDesc, trackId);
            std::cout << "trackId is: " << trackId << std::endl;
            continue;
        } else if (cmd == "Start") {
            muxer_->Start();
            continue;
        } else if (cmd == "WriteTrackSample") {
            WriteTrackSample();
            continue;
        } else if (cmd == "Stop") {
            muxer_->Stop();
            continue;
        } else if (cmd == "Release") {
            muxer_->Release();
            break;
        } else {
            std::cout << "Unknow cmd, try again" << std::endl;
            continue;
        }
    } while (1);
}

void MuxerDemo::RunCase()
{
    muxer_ = OHOS::Media::MuxerFactory::CreateMuxer();
    if (muxer_ == nullptr) {
        std::cout << "muxer_ is null" << std::endl;
        return;
    }
    DoNext();
}
}  // namespace Media
}  // namespace OHOS