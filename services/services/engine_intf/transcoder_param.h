/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef TRANSCODER_PARAM_H
#define TRANSCODER_PARAM_H

#include <cstdint>
#include <string>
#include "transcoder.h"

namespace OHOS {
namespace Media {

enum TransCoderPublicParamType : uint32_t {
    // video begin
    VIDEO_PUBLIC_PARAM_BEGIN,
    VIDEO_ENC_FMT,
    VIDEO_RECTANGLE,
    VIDEO_BITRATE,
    VIDEO_PUBLIC_PARAM_END,
    // audio begin
    AUDIO_PUBLIC_PARAM_BEGIN,
    AUDIO_ENC_FMT,
    AUDIO_BITRATE,
    AUDIO_PUBIC_PARAM_END,
    // input begin,
    INPUT_PATH,
    INPUT_URL,
    INPUT_FD,
    // output begin,
    OUTPUT_PATH,
    OUTPUT_FD,
};

/*
 * TransCoder parameter base structure, inherite to it to extend the new parameter.
 */
struct TransCoderParam {
    explicit TransCoderParam(uint32_t t) : type(t) {}
    TransCoderParam() = delete;
    virtual ~TransCoderParam() = default;
    uint32_t type;
};

struct VideoEnc : public TransCoderParam {
    explicit VideoEnc(VideoCodecFormat fmt) : TransCoderParam(TransCoderPublicParamType::VIDEO_ENC_FMT), encFmt(fmt) {}
    VideoCodecFormat encFmt;
};

struct VideoRectangle : public TransCoderParam {
    VideoRectangle(int32_t w, int32_t h) : TransCoderParam(TransCoderPublicParamType::VIDEO_RECTANGLE),
        width(w), height(h) {}
    int32_t width;
    int32_t height;
};

struct VideoBitRate : public TransCoderParam {
    explicit VideoBitRate(int32_t br) : TransCoderParam(TransCoderPublicParamType::VIDEO_BITRATE), bitRate(br) {}
    int32_t bitRate;
};

struct AudioEnc : public TransCoderParam {
    explicit AudioEnc(AudioCodecFormat fmt) : TransCoderParam(TransCoderPublicParamType::AUDIO_ENC_FMT), encFmt(fmt) {}
    AudioCodecFormat encFmt;
};

struct AudioBitRate : public TransCoderParam {
    explicit AudioBitRate(int32_t br) : TransCoderParam(TransCoderPublicParamType::AUDIO_BITRATE), bitRate(br) {}
    int32_t bitRate;
};

struct InputFilePath : public TransCoderParam {
    explicit InputFilePath(const std::string &filePath) : TransCoderParam(TransCoderPublicParamType::INPUT_PATH),
        path(filePath) {}
    std::string path;
};

struct InputUrl : public TransCoderParam {
    explicit InputUrl(std::string inputUrl) : TransCoderParam(TransCoderPublicParamType::INPUT_URL), url(inputUrl) {}
    std::string url;
};

struct InputFd : public TransCoderParam {
    explicit InputFd(int32_t inputFd, int64_t inputOffset, int64_t inputSize) :
        TransCoderParam(TransCoderPublicParamType::INPUT_FD), fd(inputFd), offset(inputOffset), size(inputSize) {}
    int32_t fd;
    int64_t offset;
    int64_t size;
};

struct OutputFilePath : public TransCoderParam {
    explicit OutputFilePath(const std::string &filePath)
        : TransCoderParam(TransCoderPublicParamType::OUTPUT_PATH), path(filePath) {}
    std::string path;
};

struct OutputFd : public TransCoderParam {
    explicit OutputFd(int32_t outFd) : TransCoderParam(TransCoderPublicParamType::OUTPUT_FD), fd(outFd) {}
    int32_t fd;
};
} // namespace Media
} // namespace OHOS
#endif
