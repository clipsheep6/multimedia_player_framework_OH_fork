#include "muxer_demo.h"
#include <iostream>
#include <cstdio>
#include <securec.h>

namespace OHOS {
namespace Media {
std::vector<std::string> MDKey = {
    // "track_index",
    // "track_type",
    // "codec_mime",
    // "duration",
    // "bitrate",
    // "max-input-size",
    "width",
    "height",
    // "pixel-format",
    // "frame-rate",
    // "capture-rate",
    // "i_frame_interval",
    // "req_i_frame"
    // "channel-count",
    // "sample-rate",
    // "track-count",
    // "vendor."
};

void MuxerDemo::ReadTrackInfo() {
    uint32_t i;
    fmt_ctx_ = avformat_alloc_context();
    avformat_open_input(&fmt_ctx_, url_, nullptr,nullptr);
    avformat_find_stream_info(fmt_ctx_, NULL);

    for (i = 0; i < fmt_ctx_->nb_streams; i++) {
        if (fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            index_ = i;
            std::cout << "index_ is: " << index_ << std::endl;
            std::cout << "fmt_ctx_->nb_streams is: " << fmt_ctx_->nb_streams << std::endl;
            width_ = fmt_ctx_->streams[i]->codec->width;
            height_ = fmt_ctx_->streams[i]->codec->height;
            break;
        }
    }
}

void MuxerDemo::WriteTrackSample() {
    AVPacket pkt;
    memset_s(&pkt, sizeof(pkt), 0, sizeof(pkt));

    AVCodecContext* pCodecContext = fmt_ctx_->streams[index_]->codec;
    TrackSampleInfo info1;
    info1.timeUs = 0;
    info1.size = pCodecContext->extradata_size;
    info1.offset = 0;
    info1.flags = CODEC_DATA;
    info1.trackIdx = 1;
    std::cout << "pCodecContext->extradata_size: " << pCodecContext->extradata_size << std::endl;
    std::shared_ptr<AVMemory> avMem1 = std::make_shared<AVMemory>(pCodecContext->extradata, info1.size);
	avMem1->SetRange(info1.offset, info1.size);
    std::cout << "avMem1->Capacity() is: " << avMem1->Capacity() << std::endl;
    std::cout << "avMem1->Size() is: " << avMem1->Size() << std::endl;
    muxer_->WriteTrackSample(avMem1, info1);

    while(1) {
        av_init_packet(&pkt);
        if (av_read_frame(fmt_ctx_, &pkt) < 0) {
            av_packet_unref(&pkt);
            break;
        }
        if (pkt.stream_index == index_) {
            TrackSampleInfo info;
            info.timeUs = static_cast<int64_t>(pkt.pts * av_q2d(fmt_ctx_->streams[pkt.stream_index]->time_base) * 1000000);
            info.size = pkt.size;
            info.offset = 0;
            if (pkt.flags == AV_PKT_FLAG_KEY) {
                info.flags = SYNC_FRAME;
            }
            info.trackIdx = 1;
            std::cout << "pkt.pts is: " << pkt.pts << std::endl;
            std::cout << "pkt.pts is: " << pkt.pts * av_q2d(fmt_ctx_->streams[pkt.stream_index]->time_base) << std::endl;
            std::cout << "pkt.size is: " << pkt.size << std::endl;
            std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(pkt.data, pkt.size);
		    avMem->SetRange(info.offset, pkt.size);
            std::cout << "avMem->Capacity() is: " << avMem->Capacity() << std::endl;
            std::cout << "avMem->Size() is: " << avMem->Size() << std::endl;
            muxer_->WriteTrackSample(avMem, info);
        }
    }
    avformat_close_input(&fmt_ctx_);
}

void MuxerDemo::DoNext()
{
    std::string path = "/data/media/output.mp4";
    std::string format = "mp4";
    muxer_->SetOutput(path, format);
    ReadTrackInfo();
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/x-h264");
    trackDesc.PutIntValue(std::string(MD_KEY_WIDTH), width_);
    trackDesc.PutIntValue(std::string(MD_KEY_HEIGHT), height_);
    std::cout << "width is: " << width_ << std::endl;
    std::cout << "height is: " << height_ << std:: endl;
    int32_t trackId;
    muxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;
    muxer_->Start();
    WriteTrackSample();
    muxer_->Stop();
    muxer_->Release();
    // std::string cmd;
    // do {
    //     std::cout << "Enter your step:" << std::endl;
    //     std::cin >> cmd;

    //     if (cmd == "SetOutput") {
    //         std::string path = "/data/media/output.mp4";
    //         std::string format = "mp4";
    //         // std::cout << "Enter path:" << std::endl;
    //         // std::cin >> path;
    //         // std::cout << "Enter format:" << std::endl;
    //         // std::cin >> format;
    //         muxer_->SetOutput(path, format);
    //         continue;
    //     } else if (cmd == "SetLocation") {
    //         float latitude = 1;
    //         float longitude = 1;
    //         // std::cout << "Enter latitude:" << std::endl;
    //         // std::cin >> latitude;
    //         // std::cout << "Enter longitude:" << std::endl;
    //         // std::cin >> longitude;
    //         muxer_->SetLocation(latitude, longitude);
    //         continue;
    //     } else if (cmd == "SetOrientationHint") {                                                    
    //         int degrees = 180;
    //         // std::cout << "Enter degrees:" << std::endl;
    //         // std::cin >> degrees;
    //         muxer_->SetOrientationHint(degrees);
    //         continue;
    //     } else if (cmd == "AddTrack") {
    //         ReadTrackInfo();
    //         MediaDescription trackDesc;
    //         // for (auto s : MDKey) {
    //         //     std:: cout << "Enter " << s << std::endl;
    //         //     if (s == "mime-type") {
    //         //         std::string value;
    //         //         std::cin >> value;
    //         //         trackDesc.PutStringValue(s, value);
    //         //     } else {
    //         //         int32_t value;
    //         //         std::cin >> value;
    //         //         trackDesc.PutIntValue(s, value);
    //         //     }
    //         // }
    //         trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/x-h264");
    //         trackDesc.PutIntValue(std::string(MD_KEY_WIDTH), width_);
    //         trackDesc.PutIntValue(std::string(MD_KEY_HEIGHT), height_);
    //         std::cout << "width is: " << width_ << std::endl;
    //         std::cout << "height is: " << height_ << std:: endl;
    //         int32_t trackId;
    //         muxer_->AddTrack(trackDesc, trackId);
    //         std::cout << "trackId is: " << trackId << std::endl;
    //         continue;
    //     } else if (cmd == "Start") {
    //         muxer_->Start();
    //         continue;
    //     } else if (cmd == "WriteTrackSample") {
    //         WriteTrackSample();
    //         continue;
    //     } else if (cmd == "Stop") {
    //         muxer_->Stop();
    //         continue;
    //     } else if (cmd == "Release") {
    //         muxer_->Release();
    //         break;
    //     } else {
    //         std::cout << "Unknow cmd, try again" << std::endl;
    //         continue;
    //     }
    // } while (1);
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