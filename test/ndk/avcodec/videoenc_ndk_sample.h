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

#ifndef VIDEOENC_NDK_SAMPLE_H
#define VIDEOENC_NDK_SAMPLE_H

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include "ndk_av_codec.h"
#include "securec.h"
#include "nocopyable.h"
#include "ndk_sample_log.h"
#include "surface.h"
#include "ndk_av_magic.h"

namespace OHOS {
namespace Media {

class VEncSignal {
public:
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<AVMemory *> outBufferQueue_;
    std::queue<uint32_t> indexQueue_;
    std::queue<uint32_t> sizeQueue_;
};

class VEncNdkSample : public NoCopyable {
public:
    VEncNdkSample() = default;
    ~VEncNdkSample();
    void RunVideoEnc();

private:
    struct AVCodec* CreateVideoEncoder(void);
    int32_t Configure(struct AVFormat *format);
    int32_t GetSurface();
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void LoopFunc();
    int32_t SetParameter(AVFormat *format);
    struct AVFormat* CreateFormat(void);
    void GenerateData(uint32_t count, uint32_t fps);
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> readLoop_;
    struct AVCodec* venc_;
    VEncSignal* signal_;
    struct AVCodecOnAsyncCallback cb_;
    int64_t timestampNs_ = 0;
    uint32_t frameCount_ = 0;
    sptr<Surface> surface_ = nullptr;

};
}
}
#endif // VIDEOENC_NDK_SAMPLE_H
