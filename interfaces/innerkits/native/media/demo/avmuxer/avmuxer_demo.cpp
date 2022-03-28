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
#include <fcntl.h>
#include "securec.h"
namespace OHOS {
namespace Media {
static const int32_t H264_FRAME_SIZE[] =
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
static const int32_t AAC_FRAME_SIZE[] = 
    { 361, 368, 22, 20, 20, 20, 20, 20, 18, 198, 513, 499, 534, 522, 541, 608, 596, 613, 631, 543, 
      563, 505, 411, 375, 402, 361, 396, 405, 372, 402, 382, 371, 363, 366, 401, 390, 519, 325, 367, 
      365, 389, 358, 389, 413, 327, 493, 378, 360, 359, 387, 356, 391, 362, 360, 393, 408, 384, 348, 
      361, 437, 494, 381, 397, 329, 365, 376, 354, 376, 347, 541, 366, 377, 370, 365, 368, 366, 358, 
      366, 401, 395, 393, 385, 348, 365, 386, 375, 381, 371, 549, 335, 376, 362, 390, 391, 350, 380, 
      368, 360, 387, 408, 392, 383, 390, 418, 353, 383, 375, 410, 355, 375, 437, 413, 393, 427, 397, 
      397, 363, 418, 393, 412, 373, 433, 323, 381, 396, 391, 372, 400, 397, 294, 280, 391, 405, 378, 
      410, 505, 411, 385, 380, 377, 345, 393, 375, 377, 378, 351, 401, 377, 404, 368, 370, 402, 386, 
      398, 393, 392, 386, 403, 360, 393, 373, 386, 362, 344, 416, 355, 380, 353, 398, 422, 386, 365, 
      335, 340, 383, 371, 386, 375, 399, 398, 352, 387, 380, 390, 391, 390, 385, 404, 384, 407, 365, 
      348, 388, 388, 385, 392, 383, 462, 463, 466, 392, 351, 358, 385, 358, 389, 378, 359, 349, 424, 
      335, 392, 347, 348, 379, 302, 295, 357, 458, 466, 460, 370, 390, 402, 405, 411, 378, 383, 351, 
      413, 420, 362, 343, 369, 367, 378, 355, 385, 410, 420, 375, 398, 394, 384, 376, 400, 395, 389, 
      395, 363, 422, 364, 365, 380, 395, 364, 395, 350, 319, 308, 374, 560, 503, 500, 438, 427, 445, 
      416, 366, 424, 331, 354, 376, 381, 387, 369, 382, 343, 366, 442, 419, 348, 362, 354, 405, 419, 
      332, 376, 388, 405, 365, 428, 379, 384, 387, 403, 385, 344, 366, 381, 366, 371, 286, 328, 470, 
      413, 404, 409, 406, 376, 370, 380, 393, 345, 400, 386, 397, 376, 407, 364, 362, 351, 377, 345, 
      375, 413, 353, 419, 382, 379, 383, 444, 392, 368, 374, 376, 349, 413, 405, 374, 355, 412, 385, 
      356, 277, 361, 461, 398, 431, 381, 405, 389, 374, 392, 377, 407, 377, 389, 406, 391, 378, 420, 
      388, 372, 372, 350, 373, 446, 399, 354, 368, 350, 373, 418, 390, 366, 367, 351, 414, 413, 362, 
      373, 364, 312, 400, 391, 371, 384, 478, 391, 400, 344, 360, 383, 349, 370, 393, 369, 364, 366, 
      401, 377, 360, 392, 398, 388, 358, 374, 386, 395, 374, 419, 376, 393, 376, 348, 416, 381, 363, 
      376, 381, 369, 378, 416, 366, 379, 363, 430, 368, 358, 470, 296, 358};

static const std::string VIDEO_CODEC_MIME = "video/avc";
static const int32_t WIDTH = 480;
static const int32_t HEIGHT = 640;
static const int32_t FRAME_RATE = 30;
static const std::string AUDIO_CODEC_MIME = "audio/mp4a-latm";
static const int32_t BASE_TIME = 1000;
static const int32_t SAMPLE_RATE = 44100;
static const int32_t CHANNEL_COUNT = 2;
static const uint32_t SLEEP_UTIME = 100000;
static const uint32_t TIME_STAMP = 33;

bool AVMuxerDemo::PushBuffer(std::shared_ptr<std::ifstream> File, const int32_t *FrameArray,
    int32_t i, int32_t TrakcId, int64_t stamp)
{
    if (FrameArray == nullptr) {
        std::cout << "Frame array error" << std::endl;
        return false;
    }
    uint8_t *buffer = (uint8_t *)malloc(sizeof(char) * (*FrameArray));
    if (buffer == nullptr) {
        std::cout << "no memory" << std::endl;
        return false;
    }
    File->read((char *)buffer, *FrameArray);
    std::shared_ptr<AVMemory> aVMem = std::make_shared<AVMemory>(buffer, *FrameArray);
    aVMem->SetRange(0, *FrameArray);
    TrackSampleInfo info;
    info.size = *FrameArray;
    info.trackIdx = TrakcId;
    if (i == 0) {
        info.timeMs = 0;
        info.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    } else if ((i == 1 && TrakcId == videoTrakcId_) || TrakcId == audioTrackId_) {
        info.timeMs = stamp;
        info.flags = AVCODEC_BUFFER_FLAG_SYNC_FRAME;
    } else {
        info.timeMs = stamp;
        info.flags = AVCODEC_BUFFER_FLAG_PARTIAL_FRAME;
    }

    avmuxer_->WriteTrackSample(aVMem, info);
    free(buffer);
    usleep(SLEEP_UTIME);

    return true;
}

std::shared_ptr<std::ifstream> openFile(const std::string filePath) {
    auto file = std::make_unique<std::ifstream>();
    file->open(filePath, std::ios::in | std::ios::binary);
    return file;
}

void AVMuxerDemo::WriteTrackSampleByteStream()
{
    const int32_t *videoFrameArray = H264_FRAME_SIZE;
    const int32_t *audioFrameArray = AAC_FRAME_SIZE;
    std::shared_ptr<std::ifstream> videoFile = openFile("/data/media/test.h264");
    std::shared_ptr<std::ifstream> audioFile = openFile("/data/media/test.aac");

    double videoStamp = 0;
    double audioStamp = 0;
    int i = 0;
    int videoLen = sizeof(H264_FRAME_SIZE) / sizeof(int32_t);
    int audioLen = sizeof(AAC_FRAME_SIZE) / sizeof(int32_t);
    while (i < videoLen && i < audioLen) {
        if (!PushBuffer(videoFile, videoFrameArray, i, videoTrakcId_, videoStamp)) {
            break;
        }
        if (!PushBuffer(audioFile, audioFrameArray, i, audioTrackId_, audioStamp)) {
            break;
        }
        i++;
        videoFrameArray++;
        audioFrameArray++;
        videoStamp += TIME_STAMP;
        audioStamp += (*audioFrameArray) * BASE_TIME / SAMPLE_RATE;
        std::cout << videoStamp << std::endl;
        std::cout << audioStamp << std::endl;
    }
}

int32_t AVMuxerDemo::AddTrackVideo()
{
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MediaDescriptionKey::MD_KEY_CODEC_MIME), VIDEO_CODEC_MIME);
    trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_WIDTH), WIDTH);
    trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_HEIGHT), HEIGHT);
    trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_FRAME_RATE), FRAME_RATE);
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;

    return trackId;
}

int32_t AVMuxerDemo::AddTrackAudio()
{
    MediaDescription trackDesc;
    trackDesc.PutStringValue(std::string(MediaDescriptionKey::MD_KEY_CODEC_MIME), AUDIO_CODEC_MIME);
    trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_SAMPLE_RATE), SAMPLE_RATE);
    trackDesc.PutIntValue(std::string(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT), CHANNEL_COUNT);
    int32_t trackId;
    avmuxer_->AddTrack(trackDesc, trackId);
    std::cout << "trackId is: " << trackId << std::endl;

    return trackId;
}

void AVMuxerDemo::DoNext()
{
    std::string path = "/data/media/output.mp4";
    std::string format = "mp4";
    int32_t fd = open(path.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        std::cout << "Open file failed! filePath is: " << path << std::endl;
        return;
    }
    avmuxer_->SetOutput(fd, format);
    avmuxer_->SetLocation(30.1111, 150.22222);
    avmuxer_->SetOrientationHint(90);

    videoTrakcId_ = AddTrackVideo();
    audioTrackId_ = AddTrackAudio();
    avmuxer_->Start();
    WriteTrackSampleByteStream();

    avmuxer_->Stop();
    avmuxer_->Release();

    (void)::close(fd);
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