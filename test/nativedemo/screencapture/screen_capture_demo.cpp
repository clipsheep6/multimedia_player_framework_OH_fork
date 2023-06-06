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

#include "screen_capture_demo.h"
#include "display_manager.h"

const char* FILE_NAME_AUDIO = "/data/data/audio.pcm";
const char* FILE_NAME_VIDEO = "/data/data/video.yuv";
const int waitTime = 5;

namespace OHOS {
namespace Media {
void ScreenCaptureCallbackTest::OnError(ScreenCaptureErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void ScreenCaptureCallbackTest::OnAudioBufferAvailable(bool isReady, AudioCapSourceType type)
{
    if (isReady) {
        std::shared_ptr<AudioBuffer> audiobuffer = nullptr;
        if (screencapture_->AcquireAudioBuffer(audiobuffer, type) == ERR_OK) {
            cout << "AcquireAudioBuffer,audioBufferLen:" << audiobuffer->length <<
                ",timestampe:" << audiobuffer->timestamp << ",audioSourceType:" << audiobuffer->sourcetype << endl;
            // dump to file
            if ((pFile_ != nullptr) && (audiobuffer->buffer != nullptr)) {
                if (fwrite(audiobuffer->buffer, 1, audiobuffer->length, pFile_) != audiobuffer->length) {
                    cout << "error occurred in audio fwrite:" << strerror(errno) << endl;
                }
                free(audiobuffer->buffer);
                audiobuffer->buffer = nullptr;
                audiobuffer = nullptr;
            }
            screencapture_->ReleaseAudioBuffer(type);
        } else {
            cout << "AcquireAudioBuffer failed" << endl;
        }
    }
}

void ScreenCaptureCallbackTest::OnVideoBufferAvailable(bool isReady)
{
    if (isReady) {
        int32_t fence = 0;
        int64_t timestamp = 0;
        OHOS::Rect damage;
        sptr<OHOS::SurfaceBuffer> surfacebuffer = screencapture_->AcquireVideoBuffer(fence, timestamp, damage);
        if (surfacebuffer != nullptr) {
            int32_t length = surfacebuffer->GetSize();
            cout << "AcquireVideoBuffer,videoBufferLen:"
                << surfacebuffer->GetSize() << ",timestamp:" << timestamp << endl;
            if (videopFile_ != nullptr) {
                if (fwrite(surfacebuffer->GetVirAddr(), 1, length, videopFile_) != length) {
                    cout << "error occurred in video fwrite:" << strerror(errno) <<endl;
                }
            }
            screencapture_->ReleaseVideoBuffer();
        } else {
            cout << "AcquireVideoBuffer failed" << endl;
        }
    }
}

int32_t ScreenCaptureDemo::setConfig(AVScreenCaptureConfig &config)
{
    AudioCapInfo miccapinfo = {
        .audioSampleRate = 16000,
        .audioChannels = 2,
        .audioSource = SOURCE_DEFAULT
    };

    AudioEncInfo audioencInfo = {
        .audioBitrate = 0,
        .audioCodecformat = AAC_LC
    };

    VideoCapInfo videocapinfo = {
        .videoFrameWidth = 720,
        .videoFrameHeight = 1280,
        .videoSource = VIDEO_SOURCE_SURFACE_RGBA
    };

    AudioInfo audioinfo = {
        .micCapInfo = miccapinfo,
        .audioEncInfo = audioencInfo
    };

    VideoInfo videoinfo = {
        .videoCapInfo = videocapinfo
    };

    config = {
        .captureMode = 0,
        .dataType = ORIGINAL_STREAM,
        .audioInfo = audioinfo,
        .videoInfo = videoinfo
    };
    return ERR_OK;
}

int ScreenCaptureDemo::RunScreenRecord()
{
    pFile = fopen(FILE_NAME_AUDIO, "w+");
    if (pFile == nullptr) {
        cout << "pFile video open failed,%{public}s " << strerror(errno) << endl;
    }
    videopFile_ = fopen(FILE_NAME_VIDEO, "w+");
    if (videopFile_ == nullptr) {
        cout << "pFile video open failed,%{public}s " << strerror(errno) << endl;
    }
    std::shared_ptr<ScreenCapture> screencapture_ = ScreenCaptureFactory::CreateScreenCapture();
    if (screencapture_ == nullptr) {
        cout << "CreateScreenCapture failed" << endl;
        return -1;
    }
    AVScreenCaptureConfig config;
    setConfig(config);

    sptr<Rosen::Display> display = Rosen::DisplayManager::GetInstance().GetDefaultDisplaySync();
    if (display != nullptr) {
        cout << "get displayinfo width:" << display->GetWidth() << ",height:"
            << display->GetHeight() << ",density:" << display->GetDpi() << endl;
        config.videoInfo.videoCapInfo.videoFrameWidth = display->GetWidth();
        config.videoInfo.videoCapInfo.videoFrameHeight = display->GetHeight();
    }
    std::shared_ptr<ScreenCaptureCallbackTest> callbackobj =
            std::make_shared<ScreenCaptureCallbackTest>(screencapture_, pFile, videopFile_);
    screencapture_->SetScreenCaptureCallback(callbackobj);
    screencapture_->Init(config);
    screencapture_->StartScreenCapture(ORIGINAL_STREAM);
    sleep(waitTime);
    screencapture_->SetMicrophoneEnable(false);
    sleep(waitTime);
    screencapture_->SetMicrophoneEnable(true);
    sleep(waitTime);
    screencapture_->StopScreenCapture();
    screencapture_->Release();

    if (pFile != nullptr) {
        fclose(pFile);
        pFile = nullptr;
    }

    if (videopFile_ != nullptr) {
        fclose(videopFile_);
        videopFile_ = nullptr;
    }
    return 0;
}
} // namespace Media
} // namespace OHOS