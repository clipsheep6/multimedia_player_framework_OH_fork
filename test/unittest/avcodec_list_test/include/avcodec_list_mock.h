/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AVCODEC_LIST_MOCK_H
#define AVCODEC_LIST_MOCK_H

#include "gtest/gtest.h"
#include "media_errors.h"
#include "securec.h"
#include "avcodec_list.h"
#include "unittest_log.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr uint32_t MIN_WIDTH = 2;
constexpr uint32_t MIN_HEIGHT = 2;
constexpr uint32_t MAX_WIDTH = 3840;
constexpr uint32_t MAX_HEIGHT = 2160;
constexpr uint32_t MAX_FRAME_RATE = 30;
constexpr uint32_t MAX_VIDEO_BITRATE = 3000000;
constexpr uint32_t MAX_AUDIO_BITRATE = 384000;
constexpr uint32_t DEFAULT_SAMPLERATE = 8000;
constexpr uint32_t MAX_CHANNEL_COUNT = 2;
constexpr uint32_t MAX_CHANNEL_COUNT_VORBIS = 7;
class AVCodecListMock : public NoCopyable {
public:
    AVCodecListMock();
    ~AVCodecListMock();
    bool CreateAVCodecList();
    std::string FindVideoDecoder(const Format &format);
    std::string FindVideoEncoder(const Format &format);
    std::string FindAudioDecoder(const Format &format);
    std::string FindAudioEncoder(const Format &format);
    std::vector<std::shared_ptr<VideoCaps>> GetVideoDecoderCaps();
    std::vector<std::shared_ptr<VideoCaps>> GetVideoEncoderCaps();
    std::vector<std::shared_ptr<AudioCaps>> GetAudioDecoderCaps();
    std::vector<std::shared_ptr<AudioCaps>> GetAudioEncoderCaps();

private:
    std::shared_ptr<AVCodecList> avCodecList_ = nullptr;
};
}
}
#endif
