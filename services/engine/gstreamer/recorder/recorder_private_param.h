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

#ifndef RECORDER_PRIVATE_PARAM_H
#define RECORDER_PRIVATE_PARAM_H

#include <memory>
#include <refbase.h>
#include "recorder_param.h"
#include "surface.h"

namespace OHOS {
namespace Media {
enum RecorderPrivateParamType : uint32_t {
    PRIVATE_PARAM_TYPE_BEGIN = PRIVATE_PARAM_SECTION_START,
    SURFACE,
    OUTPUT_FORMAT,
    APP_INFO,
    AUDIO_CAPTURE_CAHNGE_INFO,
    MAX_AMPLITUDE,
    MICRO_PHONE_DESCRIPTOR,
    MICRO_PHONE_DESCRIPTOR_LENGTH,
};

struct SurfaceParam : public RecorderParam {
    SurfaceParam() : RecorderParam(RecorderPrivateParamType::SURFACE), surface_(nullptr) {}
    ~SurfaceParam() = default;
    sptr<Surface> surface_;
};

struct OutputFormat : public RecorderParam {
    explicit OutputFormat(int32_t format) : RecorderParam(RecorderPrivateParamType::OUTPUT_FORMAT), format_(format) {}
    ~OutputFormat() = default;
    int32_t format_;
};

struct AppInfo : public RecorderParam {
    AppInfo(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
        : RecorderParam(RecorderPrivateParamType::APP_INFO),
          appUid_(appUid),
          appPid_(appPid),
          appTokenId_(appTokenId),
          appFullTokenId_(appFullTokenId)
    {}
    ~AppInfo() = default;
    int32_t appUid_;
    int32_t appPid_;
    uint32_t appTokenId_;
    uint64_t appFullTokenId_;
};

struct AudioRecordChangeInfoParam : public RecorderParam {
    AudioRecordChangeInfoParam()
        : RecorderParam(RecorderPrivateParamType::AUDIO_CAPTURE_CAHNGE_INFO)
    {}
    ~AudioRecordChangeInfoParam() = default;
    int32_t createrUID_ = 0;
    int32_t clientUID_ = 0;
    int32_t sessionId_ = 0;
    int32_t inputSource_ = 0;
    int32_t capturerFlags_ = 0;
    int32_t aRecorderState_ = 0;
    int32_t deviceType_ = 0;
    int32_t deviceRole_ = 0;
    int32_t deviceId_ = 0;
    int32_t channelMasks_ = 0;
    int32_t channelIndexMasks_ = 0;
    std::string deviceName_ = "";
    std::string macAddress_ = "";
    int32_t samplingRate_ = 0;
    int32_t encoding_ = 0;
    int32_t format_ = 0;
    int32_t channels_ = 0;
    std::string networkId_ = "";
    std::string displayName_ = "";
    int32_t interruptGroupId_ = 0;
    int32_t volumeGroupId_ = 0;
    bool isLowLatencyDevice_ = false;
    bool muted_ = false;
};

struct AudioMaxAmplitudeParam : public RecorderParam {
    AudioMaxAmplitudeParam() : RecorderParam(RecorderPrivateParamType::MAX_AMPLITUDE), maxAmplitude_(0) {}
    ~AudioMaxAmplitudeParam() = default;
    int32_t maxAmplitude_;
};

struct MicrophoneDescriptorParam : public RecorderParam {
    MicrophoneDescriptorParam() : RecorderParam(RecorderPrivateParamType::MICRO_PHONE_DESCRIPTOR) {}
    ~MicrophoneDescriptorParam() = default;
    int32_t micId_ = 0;
    int32_t deviceType_ = 0;
    int32_t sensitivity_ = 0;
    float position_x_ = 0.0f;
    float position_y_ = 0.0f;
    float position_z_ = 0.0f;
    float orientation_x_ = 0.0f;
    float orientation_y_ = 0.0f;
    float orientation_z_ = 0.0f;
};

struct MicrophoneDescriptorSizeParam : public RecorderParam {
    MicrophoneDescriptorSizeParam() : RecorderParam(RecorderPrivateParamType::MICRO_PHONE_DESCRIPTOR_LENGTH) {}
    ~MicrophoneDescriptorSizeParam() = default;
    int32_t length_;
};
} // namespace Media
} // namespace OHOS
#endif
