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
    cout << "Error received, errorType: " << errorType << " errorCode: " << errorCode << endl;
}

void TestScreenCaptureCallbackTest::OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type)
{
    cout << "OnAudioBufferAvailable received: " << isReady << ", AudioCaptureSourceType: " << type << endl;
}

void TestScreenCaptureCallbackTest::OnVideoBufferAvailable(bool isReady)
{
    cout << "OnVideoBufferAvailable received: " << isReady << endl;
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
        return false;
    }
    return true;
}

int32_t TestScreenCapture::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack>& callback)
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->SetScreenCaptureCallback(callback);
}

int32_t TestScreenCapture::Init(AVScreenCaptureConfig config)
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->Init(config);
}

int32_t TestScreenCapture::StartScreenCapture()
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->StartScreenCapture();
}

int32_t TestScreenCapture::StopScreenCapture()
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->StopScreenCapture();
}

int32_t TestScreenCapture::Release()
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    screenCapture->Release();
    screenCapture = nullptr;
    return MSERR_OK;
}

int32_t TestScreenCapture::SetMicrophoneEnabled(bool isMicrophone)
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->SetMicrophoneEnabled(isMicrophone);
}

int32_t TestScreenCapture::SetCanvasRotation(bool canvasRotation)
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->SetCanvasRotation(canvasRotation);
}

int32_t TestScreenCapture::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer, AudioCaptureSourceType type)
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->AcquireAudioBuffer(audioBuffer, type);
}

sptr<OHOS::SurfaceBuffer> TestScreenCapture::AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, Rect &damage)
{
    if (screenCapture == nullptr) {
        cout << "error! screenCapture == nullptr!" << endl;
        return nullptr;
    }
    return screenCapture->AcquireVideoBuffer(fence, timestamp, damage);
}

int32_t TestScreenCapture::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->ReleaseAudioBuffer(type);
}

int32_t TestScreenCapture::ReleaseVideoBuffer()
{
    if (screenCapture == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    return screenCapture->ReleaseVideoBuffer();
}
