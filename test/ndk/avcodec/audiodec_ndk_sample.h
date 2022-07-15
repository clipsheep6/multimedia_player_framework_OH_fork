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

#ifndef AUDIODEC_NDK_SAMPLE_H
#define AUDIODEC_NDK_SAMPLE_H

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "securec.h"
#include "ndk_av_codec.h"
#include "nocopyable.h"
#include "ndk_sample_log.h"

namespace OHOS {
namespace Media {

class ADecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<uint32_t>  sizeQueue_;
    std::queue<AVMemory *> inBufferQueue_;
    std::queue<AVMemory *> outBufferQueue_;
};

class ADecNdkSample : public NoCopyable {
public:
    ADecNdkSample() = default;
    ~ADecNdkSample();
    void RunAudioDec();

private:
    struct AVCodec* CreateAudioDecoder(void);
    int32_t Configure(struct AVFormat *format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t SetParameter(AVFormat *format);
    void InputFunc();
    void OutputFunc();
    struct AVFormat* CreateFormat(void);
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct AVCodec* adec_;
    ADecSignal* signal_;
    struct AVCodecOnAsyncCallback cb_;
    int64_t timeStamp_ = 0;
    uint32_t frameCount_ = 0;
};
}
}
#endif // AUDIODEC_NDK_SAMPLE_H
