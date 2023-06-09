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

#ifndef SCREEN_CAPTURE_SERVICE_PROXY_H
#define SCREEN_CAPTURE_SERVICE_PROXY_H

#include "i_standard_screen_capture_service.h"

namespace OHOS {
namespace Media {
class ScreenCaptureServiceProxy : public IRemoteProxy<IStandardScreenCaptureService>, public NoCopyable {
public:
    explicit ScreenCaptureServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~ScreenCaptureServiceProxy();

    void Release() override;
    int32_t DestroyStub() override;
    int32_t InitAudioCap(AudioCapInfo audioInfo) override;
    int32_t InitVideoCap(VideoCapInfo videoInfo) override;
    int32_t StartScreenCapture() override;
    int32_t StopScreenCapture() override;
    int32_t AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCapSourceType type) override;
    int32_t AcquireVideoBuffer(sptr<OHOS::SurfaceBuffer> &surfacebuffer, int32_t &fence,
                               int64_t &timestamp, OHOS::Rect &damage) override;
    int32_t ReleaseAudioBuffer(AudioCapSourceType type) override;
    int32_t ReleaseVideoBuffer() override;
    int32_t SetMicrophoneEnable(bool isMicrophone) override;
    int32_t SetListenerObject(const sptr<IRemoteObject> &object) override;

private:
    static inline BrokerDelegator<ScreenCaptureServiceProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_SERVICE_PROXY_H