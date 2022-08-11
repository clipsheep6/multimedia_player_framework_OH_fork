/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef AUDIOENC_NATIVE_SAMPLE_H
#define AUDIOENC_NATIVE_SAMPLE_H

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "native_avcodec_base.h"
#include "native_avcodec_audioencoder.h"
#include "securec.h"
#include "nocopyable.h"
#include "native_sample_log.h"

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
    std::queue<uint32_t>  outSizeQueue_;
    std::queue<AVMemory *> inBufferQueue_;
    std::queue<AVMemory *> outBufferQueue_;
    std::atomic<bool> isFlushing_ = false;
    std::atomic<bool> isStop_ = false;
    std::atomic<bool> isEOS_ = false;
};

class AEncNdkSample : public NoCopyable {
public:
    AEncNdkSample() = default;
    ~AEncNdkSample();
    void RunAudioEnc();

private:
    struct AVCodec* CreateAudioEncoder(void);
    int32_t Configure(struct AVFormat *format);
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void InputFunc();
    void OutputFunc();
    int32_t SetParameter(AVFormat *format);
    struct AVFormat *CreateFormat(void);
    void ClearIntQueue(std::queue<uint32_t> &q);
    void ClearBufferQueue(std::queue<AVMemory *> &q);

    std::atomic<bool> isRunning_ = false;

    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct AVCodec* aenc_;
    AEncSignal* signal_;
    struct AVCodecAsyncCallback cb_;
    int64_t timeStamp_ = 0;
    uint32_t frameCount_ = 0;
};
}
}
#endif // AUDIOENC_NATIVE_SAMPLE_H
