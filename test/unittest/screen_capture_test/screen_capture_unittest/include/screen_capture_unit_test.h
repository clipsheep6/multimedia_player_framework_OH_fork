/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SCREEN_CAPTURE_UNIT_TEST_H
#define SCREEN_CAPTURE_UNIT_TEST_H

#include "gtest/gtest.h"
#include "screen_capture_mock.h"

namespace OHOS {
namespace Media {
class ScreenCaptureUnitTestCallback : public ScreenCaptureCallBackMock {
public:
    explicit ScreenCaptureUnitTestCallback(std::shared_ptr<ScreenCaptureMock> ScreenCapture, FILE *aFile, FILE *vFile,
        int32_t aFlag, int32_t vFlag)
        : screenCapture_(ScreenCapture), aFile_(aFile), vFile_(vFile), aFlag_(aFlag), vFlag_(vFlag) {}
    explicit ScreenCaptureUnitTestCallback(std::shared_ptr<ScreenCaptureMock> ScreenCapture)
        : screenCapture_(ScreenCapture) {}
    ~ScreenCaptureUnitTestCallback() = default;
    void OnError(int32_t errorCode) override;
    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override;
    void OnVideoBufferAvailable(bool isReady) override;
    void OnStateChange(AVScreenCaptureStateCode stateCode) override;
    void OnError(int32_t errorCode, void *userData) override;
    void OnBufferAvailable(std::shared_ptr<AVBuffer> buffer, AVScreenCaptureBufferType bufferType,
        int64_t timestamp) override;
    void ProcessAudioBuffer(uint8_t *buffer, int32_t size, int64_t timestamp, AVScreenCaptureBufferType bufferType);
    void ProcessVideoBuffer(sptr<OHOS::SurfaceBuffer> surfacebuffer, int64_t timestamp);
    void CheckDataCallbackAudio(AudioCaptureSourceType type, int32_t flag);
    void CheckDataCallbackVideo(int32_t flag);
    void DumpBuffer(FILE *file, uint8_t *buffer, int32_t size, int64_t timestamp,
        AVScreenCaptureBufferType bufferType = AVScreenCaptureBufferType::SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
    void InitCaptureTrackInfo(FILE *file, int32_t flag, AVScreenCaptureBufferType bufferType);
    AVScreenCaptureStateCode GetScreenCaptureState()
    {
        return screenCaptureState_.load();
    }
    void DumpAudioBuffer(std::shared_ptr<AudioBuffer> audioBuffer);
    void DumpVideoBuffer(sptr<OHOS::SurfaceBuffer> surfacebuffer, int64_t timestamp);

private:
    std::shared_ptr<ScreenCaptureMock> screenCapture_;
    FILE *innerAudioFile_ = nullptr;
    FILE *micAudioFile_ = nullptr;
    FILE *videoFile_ = nullptr;
    int32_t innerAudioFlag_ = 0;
    int32_t micAudioFlag_ = 0;
    int32_t videoFlag_ = 0;
    std::atomic<AVScreenCaptureStateCode> screenCaptureState_ = AVScreenCaptureStateCode::SCREEN_CAPTURE_STATE_INVLID;

    FILE *aFile_ = nullptr;
    FILE *vFile_ = nullptr;
    int32_t aFlag_ = 0;
    int32_t vFlag_ = 0;
};

class ScreenCaptureUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    void AudioLoop(void);
    void AudioLoopWithoutRelease(void);
    int32_t SetConfig(AVScreenCaptureConfig &config);
    int32_t SetConfigFile(AVScreenCaptureConfig &config, RecorderInfo &recorderInfo);
    int32_t SetRecorderInfo(std::string name, RecorderInfo &recorderInfo);
    void OpenFile(std::string name);
    void CloseFile(void);
    void OpenFile(std::string name, bool isInnerAudioEnabled, bool isMicAudioEnabled, bool isVideoEnabled);
    void OpenFileFd(std::string name);

protected:
    static const std::string SCREEN_CAPTURE_ROOT_DIR;

    std::shared_ptr<ScreenCaptureMock> screenCapture_ = nullptr;
    std::shared_ptr<ScreenCaptureUnitTestCallback> screenCaptureCb_ = nullptr;
    std::unique_ptr<std::thread> audioLoop_ = nullptr;

    AVScreenCaptureConfig config_;
    FILE *aFile = nullptr;
    FILE *vFile = nullptr;
    int32_t aFlag = 0;
    int32_t vFlag = 0;
    char fileName[100] = {0};
    FILE *innerAudioFile_ = nullptr;
    FILE *micAudioFile_ = nullptr;
    FILE *videoFile_ = nullptr;
    int32_t outputFd_ = -1;
};
} // namespace Media
} // namespace OHOS
#endif