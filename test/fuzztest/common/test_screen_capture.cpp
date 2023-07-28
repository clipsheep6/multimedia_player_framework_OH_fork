/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "test_screen_capture.h"
#include <sync_fence.h>
#include "securec.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

void TestScreenCaptureCallbackTest::OnError(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void TestScreenCaptureCallbackTest::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    if (isReady) {
        std::shared_ptr<AudioBuffer> audioBuffer;
        if (screenCapture->AcquireAudioBuffer(audioBuffer, type) == MSERR_OK) {
            cout << "AcquireAudioBuffer, audioBufferLen:" << audioBuffer->length << ", timestampe:"
                << audioBuffer->timestamp << ", audioSourceType:" << audioBuffer->sourcetype << endl;
        }
        screenCapture->ReleaseAudioBuffer(type);
    } else {
        cout << "AcquireAudioBuffer failed" << endl;
    }
}

void TestScreenCaptureCallbackTest::OnVideoBufferAvailable(bool isReady)
{
    if (isReady) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screenCapture->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "AcquireVideoBuffer, videoBufferLen:" << surfacebuffer->GetSize() << "timestamp:"
                << timestamp << "size:"<< length << endl;
            screenCapture->ReleaseVideoBuffer();
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
    }
}

TestScreenCapture::TestScreenCapture()
{
}

TestScreenCapture::~TestScreenCapture()
{
}

bool TestScreenCapture::CreateScreenCapture()
{
    screenCapture = ScreenCaptureFactory::CreateScreenCapture();
    if (screenCapture == nullptr) {
        screenCapture->Release();
        return false;
    }
    return true;
}

int32_t TestScreenCapture::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack>& callback)
{
    return screenCapture->SetScreenCaptureCallback(callback);
}

int32_t TestScreenCapture::Init(AVScreenCaptureConfig config)
{
    return screenCapture->Init(config);
}

int32_t TestScreenCapture::StartScreenCapture()
{
    return screenCapture->StartScreenCapture();
}

int32_t TestScreenCapture::StopScreenCapture()
{
    return screenCapture->StopScreenCapture();
}

int32_t TestScreenCapture::Release()
{
    return screenCapture->Release();
}

int32_t TestScreenCapture::SetMicrophoneEnabled(bool isMicrophone)
{
    return screenCapture->SetMicrophoneEnabled(isMicrophone);
}

int32_t TestScreenCapture::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type)
{
    return screenCapture->AcquireAudioBuffer(audioBuffer, type);
}

sptr<OHOS::SurfaceBuffer> TestScreenCapture::AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, Rect &damage)
{
    return screenCapture->AcquireVideoBuffer(fence, timestamp, damage);
}

int32_t TestScreenCapture::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    return screenCapture->ReleaseAudioBuffer(type);
}

int32_t TestScreenCapture::ReleaseVideoBuffer()
{
    return screenCapture->ReleaseVideoBuffer();
}
