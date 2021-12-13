#include "avmuxer_demo.h"
#include <iostream>
#include <cstdio>
#include <securec.h>
#include <unistd.h>

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
    "frame-rate",
    // "capture-rate",
    // "i_frame_interval",
    // "req_i_frame"
    // "channel-count",
    // "sample-rate",
    // "track-count",
    // "vendor."
};

void AVMuxerDemo::ReadTrackInfo() {
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
            frameRate_ = fmt_ctx_->streams[i]->avg_frame_rate.num/fmt_ctx_->streams[i]->avg_frame_rate.den;
            break;
        }
    }
}

void AVMuxerDemo::WriteTrackSample() {
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
    avmuxer_->WriteTrackSample(avMem1, info1);

    // while(1) {
    //     av_init_packet(&pkt);
    //     if (av_read_frame(fmt_ctx_, &pkt) < 0) {
    //         av_packet_unref(&pkt);
    //         break;
    //     }
    //     if (pkt.stream_index == index_) {
    //         std::cout << pkt.flags << std::endl;
    //         int64_t pts = static_cast<int64_t>(av_rescale_q(pkt.pts, fmt_ctx_->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q));
    //         frames_.emplace(std::make_pair(pts, std::make_tuple(pkt.data, pkt.size, pkt.flags)));
    //     }
    // }

    // for (auto iter = frames_.begin(); iter != frames_.end(); iter++) {
    //     TrackSampleInfo info;
    //     info.timeUs = iter->first;
    //     info.size = std::get<1>(iter->second);
    //     info.offset = 0;
    //     if (std::get<2>(iter->second) == 1) {
    //         std::cout << "std::get<2>(iter->second) is: " << std::get<2>(iter->second) << std::endl;
    //         info.flags = SYNC_FRAME;
    //     } else {
    //         info.flags = PARTIAL_FRAME;
    //     }
    //     info.trackIdx = 1;
    //     std::cout << "pkt.pts is: " << info.timeUs << " pkt.size is: " << info.size << " pkt.flag is: " << info.flags << std::endl;
    //     std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(std::get<0>(iter->second), info.size);
	// 	avMem->SetRange(info.offset, info.size);
    //     avmuxer_->WriteTrackSample(avMem, info);
    //     usleep(50000);
    // }

    while(1) {
        av_init_packet(&pkt);
        if (av_read_frame(fmt_ctx_, &pkt) < 0) {
            av_packet_unref(&pkt);
            break;
        }
        if (pkt.stream_index == index_) {
            TrackSampleInfo info;
            info.timeUs = static_cast<int64_t>(av_rescale_q(pkt.dts, fmt_ctx_->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q));
            if (info.timeUs < 0) {
                info.timeUs = 0;
            }
            info.size = pkt.size;
            info.offset = 0;
            if (pkt.flags == AV_PKT_FLAG_KEY) {
                info.flags = SYNC_FRAME;
            }
            info.trackIdx = 1;
            std::cout << "pkt.dts is: " << info.timeUs << ", pkt.size is: " << pkt.size << std::endl;
            std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(pkt.data, pkt.size);
		    avMem->SetRange(info.offset, pkt.size);
            avmuxer_->WriteTrackSample(avMem, info);
        }
        usleep(50000);
    }
    avformat_close_input(&fmt_ctx_);
}

void AVMuxerDemo::DoNext()
{
    std::string path = "/data/media/output.mp4";
    std::string format = "mp4";
    avmuxer_->SetOutput(path, format);
    ReadTrackInfo();
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/x-h264");
    trackDesc.PutIntValue(std::string(MD_KEY_WIDTH), width_);
    trackDesc.PutIntValue(std::string(MD_KEY_HEIGHT), height_);
    trackDesc.PutIntValue(std::string(MD_KEY_FRAME_RATE), frameRate_);
    std::cout << "width is: " << width_ << std::endl;
    std::cout << "height is: " << height_ << std:: endl;
    std::cout << "frameRate is: " << frameRate_ << std::endl;
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;
    avmuxer_->Start();
    WriteTrackSample();
    avmuxer_->Stop();
    avmuxer_->Release();
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
    //         avmuxer_->SetOutput(path, format);
    //         continue;
    //     } else if (cmd == "SetLocation") {
    //         float latitude = 1;
    //         float longitude = 1;
    //         // std::cout << "Enter latitude:" << std::endl;
    //         // std::cin >> latitude;
    //         // std::cout << "Enter longitude:" << std::endl;
    //         // std::cin >> longitude;
    //         avmuxer_->SetLocation(latitude, longitude);
    //         continue;
    //     } else if (cmd == "SetOrientationHint") {                                                    
    //         int degrees = 180;
    //         // std::cout << "Enter degrees:" << std::endl;
    //         // std::cin >> degrees;
    //         avmuxer_->SetOrientationHint(degrees);
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
    //         avmuxer_->AddTrack(trackDesc, trackId);
    //         std::cout << "trackId is: " << trackId << std::endl;
    //         continue;
    //     } else if (cmd == "Start") {
    //         avmuxer_->Start();
    //         continue;
    //     } else if (cmd == "WriteTrackSample") {
    //         WriteTrackSample();
    //         continue;
    //     } else if (cmd == "Stop") {
    //         avmuxer_->Stop();
    //         continue;
    //     } else if (cmd == "Release") {
    //         avmuxer_->Release();
    //         break;
    //     } else {
    //         std::cout << "Unknow cmd, try again" << std::endl;
    //         continue;
    //     }
    // } while (1);
}

void AVMuxerDemo::RunCase()
{
    avmuxer_ = OHOS::Media::AVMuxerFactory::CreateAVMuxer();
    if (avmuxer_ == nullptr) {
        std::cout << "avmuxer_ is null" << std::endl;
        return;
    }
    DoNext();
}
}  // namespace Media
}  // namespace OHOS