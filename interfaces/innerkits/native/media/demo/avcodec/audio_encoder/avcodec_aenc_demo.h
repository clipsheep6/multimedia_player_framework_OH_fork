/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_AENC_DEMO_H
#define AVCODEC_AENC_DEMO_H

#include <atomic>
#include <thread>
#include <queue>
#include <string>
#include "avcodec_audio_encoder.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AEncSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
};

class AEncDemoCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit AEncDemoCallback(std::shared_ptr<AEncSignal> signal);
    virtual ~AEncDemoCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;

private:
    std::shared_ptr<AEncSignal> signal_;
};

class AEncDemo : public NoCopyable {
public:
    AEncDemo() = default;
    virtual ~AEncDemo() = default;
    void RunCase();

private:
    int32_t CreateAenc();
    int32_t Configure(const Format &format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void InputFunc();
    void OutputFunc();

    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::shared_ptr<AudioEncoder> aenc_;
    std::shared_ptr<AEncSignal> signal_;
    std::shared_ptr<AEncDemoCallback> cb_;
    int64_t timeStamp_ = 0;
    uint32_t sampleCount_ = 0;
};
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_AENC_DEMO_H
