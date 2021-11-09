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

#ifndef AVCODEC_INFO_H
#define AVCODEC_INFO_H

#include <cstdint>
#include <string>
#include <vector>
#include "avcodec_common.h"
#include "format.h"

namespace OHOS {
namespace Media {

class AudioCapability {
public:
    virtual ~AudioCapability() = default;
    virtual std::vector<int32_t> GetSupportedBitrate() = 0;
    virtual std::vector<int32_t> GetSupportedChannel() = 0;
    virtual std::vector<int32_t> GetSupportedSampleRate() = 0;
};

class EncoderCapability {
public:
    virtual ~EncoderCapability() = default;
    virtual std::vector<int32_t> GetSupportedComplexity() = 0;
    virtual std::vector<int32_t> GetSupportedQuality() = 0;
    virtual std::vector<int32_t> GetSupportedBitrateMode() = 0;
};

class VideoCapability {
public:
    virtual ~VideoCapability() = default;
    virtual std::vector<int32_t> GetSupportedBitrate() = 0;
    virtual std::vector<int32_t> GetSupportedFrameRate() = 0;
    virtual std::vector<int32_t> GetSupportedHeight() = 0;
    virtual std::vector<int32_t> GetSupportedWidth() = 0;
    virtual int32_t GetHeightAlignment() = 0;
    virtual int32_t GetWidthAlignment() = 0;
    virtual bool IsSizeSupported(int32_t width, int32_t height) = 0;
};

class AVCodecCapability {
public:
    virtual ~AVCodecCapability() = default;
    virtual AudioCapability GetAudioCapability() = 0;
    virtual VideoCapability GetVideoCapability() = 0;
    virtual EncoderCapability GetEncoderCapability() = 0;
    virtual std::string GetMimeType() = 0;
    virtual Format GetDefaultFormat() = 0;
    virtual int32_t GetMaxSupportedInstances() = 0;
    virtual bool IsFormatSupported(const Format &format) = 0;
};

class AVCodecInfo {
public:
    virtual ~AVCodecInfo() = default;
    virtual std::shared_ptr<AVCodecCapability> GetCapability() = 0;
    virtual std::string GetName() = 0;
    virtual AVCodecType GetType() = 0;
    virtual bool isHardwareAccelerated() = 0;
    virtual bool isSoftwareOnly() = 0;
    virtual bool isVendor() = 0;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_INFO_H