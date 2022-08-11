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

#ifndef VIDEODEC_NATIVE_SAMPLE_H
#define VIDEODEC_NATIVE_SAMPLE_H

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
#include "native_avcodec_videodecoder.h"
#include "securec.h"
#include "nocopyable.h"
#include "native_sample_log.h"
#include "surface.h"
#include "native_avmagic.h"

namespace OHOS {
namespace Media {

class VIDCodecSignal {
public:
    std::mutex vdecInpMutex_;
    std::mutex vdecOutMutex_;

    std::mutex vencOutMutex_;
    std::mutex mutex_;

    std::condition_variable vdecInCond_;
    std::condition_variable vdecOutCond_;
    std::condition_variable vencOutCond_;

    std::queue<AVMemory *> vdecInpBufferQueue_;
    std::queue<AVMemory *> vencOutBufferQueue_;

    std::queue<uint32_t> vdecInpIndexQueue_;
    std::queue<uint32_t> vdecOutIndexQueue_;
    std::queue<bool> vdecOutFlagQueue_;
    std::queue<uint32_t> vencOutIndexQueue_;

    std::queue<uint32_t> vdecInpSizeQueue_;
    std::queue<uint32_t> vdecOutSizeQueue_;
    std::queue<uint32_t> vencOutSizeQueue_;
    std::queue<bool> vencOutFlagQueue_;

    std::atomic<bool> isVdecFlushing_ = false;
    std::atomic<bool> isVencFlushing_ = false;
    std::atomic<bool> isVdecStop_ = false;
    std::atomic<bool> isVencStop_ = false;

    std::atomic<bool> isVdecEOS_ = false;
    std::atomic<bool> isVencEOS_ = false;
    

};

class VCodecNdkSample : public NoCopyable {
public:
    VCodecNdkSample() = default;
    ~VCodecNdkSample();
    void RunVideoCodec();

private:
    struct AVCodec* CreateVideoDecoder(void);
    struct AVCodec* CreateVideoEncoder(void);
    int32_t VDecConfigure(struct AVFormat *format);
    int32_t VEncConfigure(struct AVFormat *format);
    int32_t CreateVencSurface();
    int32_t SetVdecSurface();
    int32_t SetSurface();

    int32_t VDecPrepare();
    int32_t VEncPrepare();

    int32_t VDecStart();
    int32_t VEncStart();

    int32_t VDecStop();
    int32_t VEncStop();

    int32_t VDecFlush();
    int32_t VEncFlush();

    int32_t VDecReset();
    int32_t VEncReset();

    int32_t VDecRelease();
    int32_t VEncRelease();

    void VDecInpLoopFunc();
    void VDecOutLoopFunc();
    void VEncOutLoopFunc();

    int32_t VDecSetParameter(AVFormat *format);
    int32_t VEncSetParameter(AVFormat *format);
    struct AVFormat *CreateDecFormat(void);
    struct AVFormat *CreateEncFormat(void);
    void GenerateData(uint32_t count, uint32_t fps);

    void ClearIntQueue(std::queue<uint32_t> &q);
    void ClearBufferQueue(std::queue<AVMemory *> &q);
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> readLoop_;
    std::unique_ptr<std::thread> vdecOutLoop_;
    std::unique_ptr<std::thread> vencOutLoop_;

    struct AVCodec* vdec_;
    struct AVCodec* venc_;
    VIDCodecSignal* signal_;
    struct AVCodecAsyncCallback vdec_cb_;
    struct AVCodecAsyncCallback venc_cb_;
    int64_t timestampNs_ = 0;
    int64_t timestamp_ = 0;
    uint32_t vdecFrameCount_ = 0;
    uint32_t vencFrameCount_ = 0;
    sptr<Surface> surface_ = nullptr;
    std::atomic<bool> isVdecRunning_ = false;
    std::atomic<bool> isVencRunning_ = false;
    bool isFirstDecFrame_ = true;
};
}
}
#endif // VIDEODEC_NATIVE_SAMPLE_H
