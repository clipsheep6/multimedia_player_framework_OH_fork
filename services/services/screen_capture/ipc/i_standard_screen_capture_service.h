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

#ifndef I_STANDARD_SCREEN_CAPTURE_SERVICE_H
#define I_STANDARD_SCREEN_CAPTURE_SERVICE_H

#include <memory>
#include <atomic>
#include "securec.h"
#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "i_screen_capture_service.h"

namespace OHOS {
namespace Media {
class IStandardScreenCaptureService : public IRemoteBroker {
public:
    virtual ~IStandardScreenCaptureService() = default;
    virtual void Release() = 0;
    virtual int32_t DestroyStub() = 0;
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
    virtual int32_t SetMicrophoneEnabled(bool isMicrophone) = 0;
    virtual int32_t SetCanvasRotation(bool canvasRotation) = 0;
    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type) = 0;
    virtual int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfaceBuffer, int32_t &fence,
                                       int64_t &timestamp, OHOS::Rect &damage) = 0;
    virtual int32_t ReleaseAudioBuffer(AudioCaptureSourceType type) = 0;
    virtual int32_t ReleaseVideoBuffer() = 0;
    virtual int32_t ExcludeContent(ScreenCaptureContentFilter &contentFilter) = 0;

    /**
     * IPC code ID
     */
    enum ScreenCaptureServiceMsg {
        SET_LISTENER_OBJ = 0,
        RELEASE = 1,
        DESTROY = 2,
        SET_CAPTURE_MODE = 3,
        SET_DATA_TYPE = 4,
        SET_RECORDER_INFO = 5,
        SET_OUTPUT_FILE = 6,
        INIT_AUDIO_ENC_INFO = 7,
        INIT_AUDIO_CAP = 8,
        INIT_VIDEO_ENC_INFO = 9,
        INIT_VIDEO_CAP = 10,
        ACQUIRE_AUDIO_BUF = 11,
        ACQUIRE_VIDEO_BUF = 12,
        RELEASE_AUDIO_BUF = 13,
        RELEASE_VIDEO_BUF = 14,
        SET_MIC_ENABLE = 15,
        START_SCREEN_CAPTURE = 16,
        START_SCREEN_CAPTURE_WITH_SURFACE = 17,
        STOP_SCREEN_CAPTURE = 18,
        SET_SCREEN_ROTATION = 19,
        EXCLUDE_CONTENT = 20,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardScreenCaptureService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_SCREEN_CAPTURE_SERVICE_H