/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef I_SCREEN_CAPTURE_SERVICE_H
#define I_SCREEN_CAPTURE_SERVICE_H

#include <string>
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "buffer/avsharedmemory.h"
#include "refbase.h"
#include "surface.h"
#include "screen_capture.h"

namespace OHOS {
namespace Media {
class IScreenCaptureService {
public:
    virtual ~IScreenCaptureService() = default;
    virtual int32_t SetCaptureMode(CaptureMode captureMode) = 0;
    virtual int32_t SetDataType(DataType dataType) = 0;
    virtual int32_t SetRecorderInfo(RecorderInfo recorderInfo) = 0;
    virtual int32_t SetOutputFile(int32_t fd) = 0;
    virtual int32_t InitAudioEncInfo(AudioEncInfo audioEncInfo) = 0;
    virtual int32_t InitAudioCap(AudioCaptureInfo audioInfo) = 0;
    virtual int32_t InitVideoEncInfo(VideoEncInfo videoEncInfo) = 0;
    virtual int32_t InitVideoCap(VideoCaptureInfo videoInfo) = 0;
    virtual int32_t StartScreenCapture(bool isPrivacyAuthorityEnabled = false) = 0;
    virtual int32_t StartScreenCaptureWithSurface(sptr<Surface> surface, bool isPrivacyAuthorityEnabled = false) = 0;
    virtual int32_t StopScreenCapture() = 0;
    virtual int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type) = 0;
    virtual int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfacebuffer, int32_t &fence,
                                       int64_t &timestamp, OHOS::Rect &damage) = 0;
    virtual int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) = 0;
    virtual int32_t ReleaseVideoBuffer() = 0;
    virtual int32_t SetMicrophoneEnabled(bool isMicrophone) = 0;
    virtual int32_t SetCanvasRotation(bool canvasRotation) = 0;
    virtual int32_t SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback) = 0;
    virtual void Release() = 0;
    virtual int32_t ExcludeContent(ScreenCaptureContentFilter &contentFilter) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_SCREEN_CAPTURE_SERVICE_H