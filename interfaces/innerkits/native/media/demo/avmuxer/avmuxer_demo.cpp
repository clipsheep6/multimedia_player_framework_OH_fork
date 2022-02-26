/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avmuxer_demo.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <unistd.h>
#include "securec.h"
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

static const int32_t ES[501] =
    { 646, 5855, 3185, 3797, 3055, 5204, 2620, 6262, 2272, 3702, 4108, 4356, 4975, 4590, 3083, 1930, 1801, 1945,
      3475, 4028, 1415, 1930, 2802, 2176, 1727, 2287, 2274, 2033, 2432, 4447, 4130, 5229, 5792, 4217, 5804,
      6586, 7506, 5128, 5549, 6685, 5248, 4819, 5385, 4818, 5239, 4148, 6980, 5124, 4255, 5666, 4756, 4975,
      3840, 4913, 3649, 4002, 4926, 4284, 5329, 4305, 3750, 4770, 4090, 4767, 3995, 5039, 3820, 4566, 5556,
      4029, 3755, 5059, 3888, 3572, 4680, 4662, 4259, 3869, 4306, 3519, 3160, 4400, 4426, 4370, 3489, 4907,
      4102, 3723, 4420, 4347, 4117, 4578, 4470, 4579, 4128, 4157, 4226, 4742, 3616, 4476, 4084, 4623, 3736,
      4207, 3644, 4349, 4948, 4009, 3583, 4658, 3974, 5441, 4049, 3786, 4093, 3375, 4207, 3787, 4365, 2905,
      4371, 4132, 3633, 3652, 2977, 4387, 3368, 3887, 3464, 4198, 4690, 4467, 2931, 3573, 4652, 3901, 4403,
      3120, 3494, 4666, 3898, 3607, 3272, 4070, 3151, 3237, 3936, 3962, 3637, 3716, 3735, 4371, 3141, 3322,
      4401, 3579, 4006, 2720, 3526, 4796, 3737, 3824, 3257, 4310, 2992, 3537, 3209, 3453, 3819, 3212, 4384,
      3571, 3682, 3344, 3017, 3960, 2737, 1970, 2433, 1442, 1560, 4710, 1070, 877, 833, 838, 776, 735, 1184,
      1172, 699, 723, 2828, 4257, 4329, 3567, 5365, 4213, 3612, 4833, 3388, 3553, 3535, 4937, 4057, 3990,
      5047, 4197, 4656, 3219, 3661, 3666, 3908, 4385, 4350, 3636, 4038, 5213, 3677, 3789, 4221, 4137, 4440,
      3447, 3836, 3912, 4806, 3100, 2963, 5204, 2394, 2391, 1772, 1586, 1598, 2558, 2663, 4537, 3530, 4045,
      4641, 5723, 3688, 4231, 3420, 3462, 3828, 4764, 3944, 4499, 4375, 4597, 4305, 3872, 3969, 2805, 4398,
      3480, 4105, 3890, 3761, 3652, 4356, 2771, 3972, 2930, 3456, 3236, 4648, 3627, 2689, 3827, 2254, 3492,
      2988, 4408, 3007, 4611, 3018, 4783, 2556, 3263, 4536, 4159, 3818, 5093, 3539, 4336, 3400, 3871, 4019,
      4619, 5520, 3781, 4026, 4864, 3340, 4153, 4641, 4292, 4071, 4144, 5109, 3695, 4512, 3882, 3943, 4152,
      4133, 3862, 4717, 3431, 4984, 4164, 4359, 3401, 3727, 4256, 3563, 4694, 3225, 3984, 2432, 3790, 2827,
      3595, 4124, 3854, 2890, 3477, 3989, 3251, 3714, 3345, 4742, 1967, 3931, 1985, 1737, 1854, 2192, 2370,
      2083, 3265, 3312, 3071, 4255, 3994, 4563, 4650, 4885, 3868, 4698, 3103, 3682, 4197, 5532, 3963, 4756,
      4067, 3917, 3667, 3812, 4793, 3260, 3763, 4670, 3184, 2930, 3558, 3245, 4120, 4700, 3671, 4442, 3406,
      4862, 4331, 5064, 4058, 4075, 3160, 3930, 5187, 3816, 3795, 3085, 3564, 3856, 3948, 4474, 3511, 4108,
      4789, 2944, 3323, 2162, 2657, 2219, 1653, 2824, 2716, 3523, 2760, 3328, 3042, 3828, 3759, 3950, 3830,
      3336, 4457, 3193, 3706, 4314, 3937, 3422, 4067, 5328, 3693, 4567, 3444, 4317, 4929, 3838, 4129, 2975,
      4227, 4639, 4348, 2935, 3999, 4745, 3919, 3694, 2602, 4538, 4637, 4250, 3716, 3513, 3856, 4916, 4460,
      4263, 4153, 4299, 3577, 5527, 2486, 3332, 4133, 4145, 3369, 3576, 3940, 4304, 3179, 5266, 3536, 3622,
      2684, 3449, 3621, 4363, 4216, 4913, 5026, 3336, 3057, 2782, 3716, 3036, 4438, 3904, 4823, 1761, 2045,
      1446, 3210, 1625, 2400, 3489, 4719, 3954, 3756, 4940, 2371, 4516, 3739, 3572, 2644, 3837, 4915, 2251,
      4248, 4019, 4407, 4217, 2913, 5106 };

static const uint32_t SLEEP_UTIME = 100000;
static const uint32_t TIME_STAMP = 33333;

void AVMuxerDemo::ReadTrackInfoAVC() {
    uint32_t i;
    fmt_ctx_ = avformat_alloc_context();
    avformat_open_input(&fmt_ctx_, url_.c_str(), nullptr,nullptr);
    avformat_find_stream_info(fmt_ctx_, NULL);

    for (i = 0; i < fmt_ctx_->nb_streams; i++) {
        if(fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex_ = i;
            std::cout << "videoIndex_ is: " << videoIndex_ << std::endl;
            std::cout << "fmt_ctx_->nb_streams is: " << fmt_ctx_->nb_streams << std::endl;
            width_ = fmt_ctx_->streams[i]->codec->width;
            height_ = fmt_ctx_->streams[i]->codec->height;
            frameRate_ = fmt_ctx_->streams[i]->avg_frame_rate.num/fmt_ctx_->streams[i]->avg_frame_rate.den;
        } else if (fmt_ctx_->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::cout << "audioIndex_ is: " << i << std::endl;
            sampleRate_ = fmt_ctx_->streams[i]->codec->sample_rate;
            channels_ = fmt_ctx_->streams[i]->codec->channels;
            audioIndex_ = i;
        }
    }
}

void AVMuxerDemo::WriteTrackSampleAVC() {
    AVPacket pkt;
    memset_s(&pkt, sizeof(pkt), 0, sizeof(pkt));

    for (int i = 0; i <= 1; i++) {
        AVCodecContext *pCodecContext = fmt_ctx_->streams[i]->codec;
        TrackSampleInfo infoCodec;
        infoCodec.timeUs = 0;
        infoCodec.size = pCodecContext->extradata_size;
        infoCodec.offset = 0;
        infoCodec.flags = AVCODEC_BUFFER_FLAG_CODEDC_DATA;
        infoCodec.trackIdx = i + 1;
        std::cout << "pCodecContext->extradata_size: " << pCodecContext->extradata_size << std::endl;
        std::shared_ptr<AVMemory> avMem1 = std::make_shared<AVMemory>(pCodecContext->extradata, infoCodec.size);
	    avMem1->SetRange(infoCodec.offset, infoCodec.size);
        std::cout << "avMem1->Capacity() is: " << avMem1->Capacity() << std::endl;
        std::cout << "avMem1->Size() is: " << avMem1->Size() << std::endl;
        avmuxer_->WriteTrackSample(avMem1, infoCodec);
    }

    while (1) {
        av_init_packet(&pkt);
        if (av_read_frame(fmt_ctx_, &pkt) < 0) {
            av_packet_unref(&pkt);
            break;
        }
        TrackSampleInfo info;
        info.timeUs = static_cast<int64_t>(av_rescale_q(pkt.dts, fmt_ctx_->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q));
        if (info.timeUs < 0) {
            info.timeUs = 0;
        }
        info.size = pkt.size;
        info.offset = 0;
        if (pkt.flags == AV_PKT_FLAG_KEY) {
            info.flags = AVCODEC_BUFFER_FLAG_SYNC_FRAME;
        }
        std::cout << "pkt.dts is: " << info.timeUs << ", pkt.size is: " << pkt.size << std::endl;
        std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(pkt.data, pkt.size);
		avMem->SetRange(info.offset, pkt.size);
        std::cout << "avMem->Capacity() is: " << avMem->Capacity() << std::endl;
        std::cout << "avMem->Size() is: " << avMem->Size() << std::endl;
        std::cout << "avMem->Data() is: " << (int *)(avMem->Data()) << std::endl;
        std::cout << "avMem->Base() is: " << (int *)(avMem->Base()) << std::endl;
        if (pkt.stream_index == videoIndex_) {  
            info.trackIdx = 1;
            std::cout << "info.trackIdx is: " << info.trackIdx << std::endl;
        } else if(pkt.stream_index == audioIndex_) {
            info.trackIdx = 2;
            std::cout << "info.trackIdx is: " << info.trackIdx << std::endl;
        }
        avmuxer_->WriteTrackSample(avMem, info);
        usleep(SLEEP_UTIME);
    }
    avformat_close_input(&fmt_ctx_);
}

void AVMuxerDemo::ReadTrackInfoByteStream() {
    width_ = 480;
    height_ = 640;
    frameRate_ = 30;
}

void AVMuxerDemo::WriteTrackSampleByteStream()
{
    const int32_t *frameLenArray_ = ES;
    const std::string filePath = "/data/media/video.es";
    auto testFile = std::make_unique<std::ifstream>();
    testFile->open(filePath, std::ios::in | std::ios::binary);
    bool isFirstStream = true;
    bool isSecondStream = true;
    int64_t timeStamp = 0;
    int i = 0;
    int len = sizeof(ES) / sizeof(int32_t);
    while (i < len) {
        uint8_t *tempBuffer = (uint8_t *)malloc(sizeof(char) * (*frameLenArray_));
        if (tempBuffer == nullptr) {
            std::cout << "no memory" << std::endl;
            break;
        }
        size_t num = testFile->read((char *)tempBuffer, *frameLenArray_).gcount();
        std::cout << "num is: " << num << std::endl;
        std::cout << "tempBuffer is: " << tempBuffer << std::endl;
        std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(tempBuffer, *frameLenArray_);
        // memcpy_s(avMem->Base(), *frameLenArray_, tempBuffer, *frameLenArray_);
        avMem->SetRange(0, *frameLenArray_);
        if (avMem->Data() == nullptr || avMem->Base() == nullptr) {
            std::cout << "no memory" << std::endl;
            break;
        } 
        std::cout << "avMem->Capacity() is: " << avMem->Capacity() << std::endl;
        std::cout << "avMem->Size() is: " << avMem->Size() << std::endl;
        std::cout << "avMem->Data() is: " << (int *)(avMem->Data()) << std::endl;
        std::cout << "avMem->Base() is: " << (int *)(avMem->Base()) << std::endl;
        TrackSampleInfo info;
        info.size = *frameLenArray_;
        info.offset = 0;
        info.trackIdx = 1;
        if (isFirstStream) {
            info.timeUs = 0;
            info.flags = AVCODEC_BUFFER_FLAG_CODEDC_DATA;
            isFirstStream = false;
            avmuxer_->WriteTrackSample(avMem, info);
        } else {
            info.timeUs = timeStamp;
            if (isSecondStream) {
                info.flags = AVCODEC_BUFFER_FLAG_SYNC_FRAME;
                isSecondStream = false;
            } else {
                info.flags = AVCODEC_BUFFER_FLAG_PARTIAL_FRAME;
            }
            timeStamp += TIME_STAMP;
            avmuxer_->WriteTrackSample(avMem, info);
        }
        frameLenArray_++;
        free(tempBuffer);
        i++;
        usleep(SLEEP_UTIME);
    }
}

void AVMuxerDemo::AddTrackVideo()
{
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/avc");
    trackDesc.PutIntValue(std::string(MD_KEY_WIDTH), width_);
    trackDesc.PutIntValue(std::string(MD_KEY_HEIGHT), height_);
    trackDesc.PutIntValue(std::string(MD_KEY_FRAME_RATE), frameRate_);
    std::cout << "width is: " << width_ << std::endl;
    std::cout << "height is: " << height_ << std:: endl;
    std::cout << "frameRate is: " << frameRate_ << std::endl;
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;
}

void AVMuxerDemo::AddTrackAudio()
{
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MD_KEY_CODEC_MIME), "audio/mpeg");
    trackDesc.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), channels_);
    trackDesc.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), sampleRate_);
    std::cout << "channels_ is: " << channels_ << std::endl;
    std::cout << "sampleRate_ is: " << sampleRate_ << std:: endl;
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;
}

void AVMuxerDemo::DoNext()
{
    std::string cmd;
    std::string path = "file:///data/media/output.mp4";
    std::string format = "mp4";
    avmuxer_->SetOutput(path, format);
    avmuxer_->SetLocation(0, 0);
    avmuxer_->SetOrientationHint(0);
    std::cout << "Enter AVC or byte-stream:" << std::endl;
    std::cin >> cmd;

    if (cmd == "AVC") {
        ReadTrackInfoAVC();
        AddTrackVideo();
        AddTrackAudio();
        avmuxer_->Start();
        WriteTrackSampleAVC();
    } else if (cmd == "byte-stream") {
        ReadTrackInfoByteStream();
        AddTrackVideo();
        avmuxer_->Start();
        WriteTrackSampleByteStream();
    }

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