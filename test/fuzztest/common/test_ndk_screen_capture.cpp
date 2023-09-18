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

#include "test_ndk_screen_capture.h"
#include <sync_fence.h>
#include "securec.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

void TestScreenCaptureNdkCallback::OnError(OH_AVScreenCapture *screenCapture, int32_t errorCode)
{
    cout << "Error received, errorCode: " << errorCode << endl;
}

void TestScreenCaptureNdkCallback::OnAudioBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady,
    OH_AudioCaptureSourceType type)
{
    cout << "OnAudioBufferAvailable received: " << isReady << ", AudioCaptureSourceType: " << type << endl;
}

void TestScreenCaptureNdkCallback::OnVideoBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady)
{
    cout << "OnVideoBufferAvailable received: " << isReady << endl;
}

TestNdkScreenCapture::TestNdkScreenCapture()
{
}

TestNdkScreenCapture::~TestNdkScreenCapture()
{
}