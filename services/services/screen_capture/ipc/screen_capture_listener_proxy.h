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

#ifndef SCREEN_CAPTURE_LISTENER_PROXY_H
#define SCREEN_CAPTURE_LISTENER_PROXY_H

#include "i_standard_screen_capture_listener.h"
#include "media_death_recipient.h"
#include "screen_capture.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class ScreenCaptureListenerCallback : public ScreenCaptureCallBack, public NoCopyable {
public:
    explicit ScreenCaptureListenerCallback(const sptr<IStandardScreenCaptureListener> &listener);
    virtual ~ScreenCaptureListenerCallback();

    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override;
    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override;
    void OnVideoBufferAvailable(bool isReady) override;
    void OnStateChange(AVScreenCaptureStateCode stateCode) override;
    void Stop()
    {
        isStopped_ = true;
    }

private:
    sptr<IStandardScreenCaptureListener> listener_ = nullptr;
    std::atomic<bool> isStopped_ = false;
};

class ScreenCaptureListenerProxy : public IRemoteProxy<IStandardScreenCaptureListener>, public NoCopyable {
public:
    explicit ScreenCaptureListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~ScreenCaptureListenerProxy();

    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override;
    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override;
    void OnVideoBufferAvailable(bool isReady) override;
    void OnStateChange(AVScreenCaptureStateCode stateCode) override;

private:
    static inline BrokerDelegator<ScreenCaptureListenerProxy> delegator_;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_LISTENER_PROXY_H