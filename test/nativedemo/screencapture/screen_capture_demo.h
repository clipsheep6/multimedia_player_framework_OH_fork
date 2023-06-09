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

#ifndef SCREEN_CAPTURE_DEMO_H
#define SCREEN_CAPTURE_DEMO_H

#include <stdio.h>
#include <fcntl.h>
#include <mutex>
#include <cstdlib>
#include <thread>
#include <string>
#include <memory>
#include <atomic>
#include <iostream>
#include "screen_capture.h"
#include "nocopyable.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "nativetoken.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
namespace Media {
using namespace std;
class ScreenCaptureCallbackTest : public ScreenCaptureCallBack {
public:
    ScreenCaptureCallbackTest(std::shared_ptr<ScreenCapture> screencapture, FILE *pFile, FILE *pFile1) {
        screencapture_ = screencapture;
        pFile_ = pFile;
        videopFile_ = pFile1;
    }
    ~ScreenCaptureCallbackTest() {
        screencapture_  = nullptr;
    }
    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override;

    void OnAudioBufferAvailable(bool isReady, AudioCapSourceType type) override;

    void OnVideoBufferAvailable(bool isReady) override;

    std::shared_ptr<ScreenCapture> screencapture_;

    FILE *pFile_ = nullptr;
    FILE *videopFile_ = nullptr;
    pthread_mutex_t pthread_mutex;
};

static AudioCapInfo miccapinfo = {
    .audioSampleRate = 16000,
    .audioChannels = 2,
    .audioSource = SOURCE_DEFAULT
};

static AudioEncInfo audioencInfo = {
    .audioBitrate = 0,
    .audioCodecformat = AAC_LC
};

static VideoCapInfo videocapinfo = {
    .videoFrameWidth = 720,
    .videoFrameHeight = 1280,
    .videoSource = VIDEO_SOURCE_SURFACE_RGBA
};

static AudioInfo audioinfo = {
    .micCapInfo = miccapinfo,
    .audioEncInfo = audioencInfo
};

static VideoInfo videoinfo = {
    .videoCapInfo = videocapinfo
};

static AVScreenCaptureConfig config = {
    .captureMode = 0,
    .dataType = ORIGINAL_STREAM,
    .audioInfo = audioinfo,
    .videoInfo = videoinfo
};

class ScreenCaptureDemo : public NoCopyable{
public:
    ScreenCaptureDemo() = default;
    virtual ~ScreenCaptureDemo() = default;
    int RunScreenRecord();

    FILE *pFile = nullptr;
    FILE *videopFile_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // SCREEN_CAPTURE_DEMO_H