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
    virtual int32_t InitAudioCap(AudioCapInfo audioInfo) = 0;
    virtual int32_t InitVideoCap(VideoCapInfo videoInfo) = 0;
    virtual int32_t StartScreenCapture() = 0;
    virtual int32_t StopScreenCapture() = 0;
    virtual int32_t SetMicrophoneEnable(bool isMicrophone) = 0;
    virtual int32_t SetListenerObject(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCapSourceType type) = 0;
    virtual int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfacebuffer, int32_t &fence,
                                       int64_t &timestamp, OHOS::Rect &damage) = 0;
    virtual int32_t ReleaseAudioBuffer(AudioCapSourceType type) = 0;
    virtual int32_t ReleaseVideoBuffer() = 0;

    /**
     * IPC code ID
     */
    enum ScreenCaptureServiceMsg {
        SET_LISTENER_OBJ,
        RELEASE,
        DESTROY,
        INITAUDIO_CAP,
        INITVIDEO_CAP,
        ACQUIRE_AUDIO_BUF,
        ACQUIRE_VIDEO_BUF,
        RELEASE_AUDIO_BUF,
        RELEASE_VIDEO_BUF,
        SETMICENABLE,
        STARTSCREEN_CAPTURE,
        STOPSCREEN_CAPTURE,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardScreenCaptureService");
};
} // namespace Media
} // namespace OHOS
#endif // I_STANDARD_SCREEN_CAPTURE_SERVICE_H