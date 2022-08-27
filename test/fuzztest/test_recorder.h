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
do {                                \
    if (!(cond)) {                  \
        return ret;                 \
    }                               \
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
    uint32_t counts = 0;

    bool CameraServicesForAudio(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool CameraServicesForVideo(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool RequesetBuffer(const std::string &recorderType, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    void StopBuffer(const std::string &recorderType);
    bool SetConfig(const std::string &recorderType, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool GetStubFile();
    void HDICreateESBuffer();
    void HDICreateYUVBuffer();
    uint64_t GetPts();
    bool SetVideoSource(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetAudioSource(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetOutputFormat(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetAudioEncoder(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetAudioSampleRate(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetAudioChannels(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetAudioEncodingBitRate(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetMaxDuration(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetOutputFile(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetRecorderCallback(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool Prepare(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool Start(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool Stop(bool block, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool Reset(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool Release(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool CreateRecorder();
    bool SetVideoEncoder(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetVideoSize(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetVideoFrameRate(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetVideoEncodingBitRate(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetCaptureRate(RecorderTestParam::VideoRecorderConfig &recorderConfig, double fps);
    bool SetNextOutputFile(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool GetSurface(RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetMaxFileSize(int64_t size, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    bool SetParameter(int32_t sourceId, const Format &format, RecorderTestParam::VideoRecorderConfig &recorderConfig);

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