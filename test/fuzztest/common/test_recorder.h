/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef TEST_RECORDER_H
#define TEST_RECORDER_H

#include <thread>
#include "recorder.h"
#include "aw_common.h"

namespace OHOS {
namespace Media {
#define RETURN_IF(cond, ret, ...)        \
do {                                     \
    if (!(cond)) {                       \
        return ret;                      \
    }                                    \
} while (0)

class TestRecorder : public NoCopyable {
public:
    TestRecorder();
    ~TestRecorder();
    std::shared_ptr<Recorder> recorder = nullptr;
    std::shared_ptr<std::ifstream> file = nullptr;
    std::unique_ptr<std::thread> camereHDIThread;
    OHOS::sptr<OHOS::Surface> producerSurface = nullptr;

    const std::string PURE_VIDEO = "video";
    const std::string PURE_AUDIO = "audio";
    const std::string AUDIO_VIDEO = "av";
    uint32_t nowFrame = 0;

    void CameraServicesForAudio(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void CameraServicesForVideo(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    bool RequesetBuffer(const std::string &recorderType, RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void StopBuffer(const std::string &recorderType);
    bool GetStubFile();
    void HDICreateESBuffer();
    void HDICreateYUVBuffer();
    uint64_t GetPts();
    void SetVideoSource(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetAudioSource(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetOutputFormat(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetAudioEncoder(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetAudioSampleRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetAudioChannels(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetAudioEncodingBitRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetMaxDuration(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetOutputFile(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetRecorderCallback(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void Prepare(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void Start(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void Stop(bool block, RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void Reset(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void Release(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    bool CreateRecorder();
    void SetVideoEncoder(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetVideoSize(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetVideoFrameRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetVideoEncodingBitRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetCaptureRate(RecorderTestParam::VideoRecorderConfig_ &recorderConfig, double fps);
    void SetNextOutputFile(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    bool GetSurface(RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetMaxFileSize(int64_t size, RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration,
        RecorderTestParam::VideoRecorderConfig_ &recorderConfig);
    void SetParameter(int32_t sourceId, const Format &format, RecorderTestParam::VideoRecorderConfig_ &recorderConfig);

private:
    std::atomic<bool> isExit_ { false };
    std::atomic<bool> isStart_ { true };
    int64_t pts = 0;
    int32_t isKeyFrame = 1;
    unsigned char color = 0xFF;
};

class TestRecorderCallbackTest : public RecorderCallback, public NoCopyable {
public:
    TestRecorderCallbackTest() = default;
    virtual ~TestRecorderCallbackTest() = default;

    void OnError(RecorderErrorType errorType, int32_t errorCode) override;
    void OnInfo(int32_t type, int32_t extra) override;
    int32_t GetErrorCode();
private:
    int32_t errorCode_ = 0;
};
}
}
#endif