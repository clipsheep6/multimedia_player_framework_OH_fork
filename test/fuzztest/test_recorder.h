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
#define FUZZTEST_CHECK(cond, fmt, ret, operation1, operation2)            \
if (cond) {                                                               \
    (void)printf("%s\n", fmt);                                            \
    operation1;                                                           \
    operation2;                                                           \
    return ret;                                                           \
}
#define FUZZTEST_CHECK_DO(cond, fmt, operation)                           \
if (cond) {                                                               \
    (void)printf("%s\n", fmt);                                            \
    operation;                                                            \
}
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

    int32_t CameraServicesForAudio(RecorderTestParam::VideoRecorderConfig &recorderConfig) const;
    int32_t CameraServicesForVideo(RecorderTestParam::VideoRecorderConfig &recorderConfig) const;
    int32_t RequesetBuffer(const std::string &recorderType, RecorderTestParam::VideoRecorderConfig &recorderConfig);
    void StopBuffer(const std::string &recorderType);
    int32_t SetConfig(const std::string &recorderType, RecorderTestParam::VideoRecorderConfig &recorderConfig) const;
    int32_t GetStubFile();
    void HDICreateESBuffer();
    void HDICreateYUVBuffer();
    uint64_t GetPts();
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