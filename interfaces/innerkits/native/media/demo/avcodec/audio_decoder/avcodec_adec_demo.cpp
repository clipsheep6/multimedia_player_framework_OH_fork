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

#include "avcodec_adec_demo.h"
#include <iostream>
#include <unistd.h>
#include "securec.h"
#include "demo_log.h"
#include "media_errors.h"

static const int32_t ES[1] =
    { 0 };

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
    constexpr uint32_t DEFAULT_CHANNELS = 2;
    constexpr uint32_t DEFAULT_AUDIO_RAW_FORMAT = 1; // SAMPLE_FORMAT_S16LE
    constexpr uint32_t DEFAULT_SAMPLE_COUNT = 50;
    constexpr uint32_t SAMPLE_DURATION_US = 0;
}

void ADecDemo::RunCase()
{
    DEMO_CHECK_AND_RETURN_LOG(CreateAdec() == MSERR_OK, "Fatal: CreateAdec fail");

    Format format;
    format.PutIntValue("channel_count", DEFAULT_CHANNELS);
    format.PutIntValue("sample_rate", DEFAULT_SAMPLE_RATE);
    format.PutIntValue("audio_sample_format", DEFAULT_AUDIO_RAW_FORMAT);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == MSERR_OK, "Fatal: Configure fail");

    DEMO_CHECK_AND_RETURN_LOG(Prepare() == MSERR_OK, "Fatal: Prepare fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == MSERR_OK, "Fatal: Start fail");
    sleep(3); // start run 3s
    DEMO_CHECK_AND_RETURN_LOG(Stop() == MSERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == MSERR_OK, "Fatal: Release fail");
}

int32_t ADecDemo::CreateAdec()
{
    adec_ = AudioDecoderFactory::CreateByMime("audio/opus");
    DEMO_CHECK_AND_RETURN_RET_LOG(adec_ != nullptr, MSERR_UNKNOWN, "Fatal: CreateByMime fail");

    signal_ = make_shared<ADecSignal>();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    cb_ = make_unique<ADecDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(adec_->SetCallback(cb_) == MSERR_OK, MSERR_UNKNOWN, "Fatal: SetCallback fail");

    return MSERR_OK;
}

int32_t ADecDemo::Configure(const Format &format)
{
    return adec_->Configure(format);
}

int32_t ADecDemo::Prepare()
{
    return adec_->Prepare();
}

int32_t ADecDemo::Start()
{
    isRunning_.store(true);

    testFile_ = std::make_unique<std::ifstream>();
    DEMO_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    testFile_->open("/data/media/opus.es", std::ios::in | std::ios::binary);

    inputLoop_ = make_unique<thread>(&ADecDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    return adec_->Start();
}

int32_t ADecDemo::Stop()
{
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inQueue_.push(10000); // wake up loop thread
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outQueue_.push(10000); // wake up loop thread
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }

    return adec_->Stop();
}

int32_t ADecDemo::Flush()
{
    return adec_->Flush();
}

int32_t ADecDemo::Reset()
{
    return adec_->Reset();
}

int32_t ADecDemo::Release()
{
    return adec_->Release();
}

void ADecDemo::InputFunc()
{
    const int32_t *frameLen = ES;

    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this](){ return signal_->inQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = adec_->GetInputBuffer(index);
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        DEMO_CHECK_AND_BREAK_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");

        char *fileBuffer = (char *)malloc(sizeof(char) * (*frameLen) + 1);
        DEMO_CHECK_AND_BREAK_LOG(fileBuffer != nullptr, "Fatal: malloc fail");

        (void)testFile_->read(fileBuffer, *frameLen);
        if (memcpy_s(buffer->GetBase(), buffer->GetSize(), fileBuffer, *frameLen) != EOK) {
            free(fileBuffer);
            cout << "Fatal: memcpy fail" << endl;
            break;
        }

        AVCodecBufferInfo info;
        info.size = *frameLen;
        info.offset = 0;
        info.presentationTimeUs = timeStamp_;

        int32_t ret = adec_->QueueInputBuffer(index, info, AVCODEC_BUFFER_FLAG_NONE);

        free(fileBuffer);
        frameLen++;
        timeStamp_ += SAMPLE_DURATION_US;
        signal_->inQueue_.pop();

        sampleCount_++;
        if (sampleCount_ == DEFAULT_SAMPLE_COUNT) {
            cout << "Finish decode, exit" << endl;
            break;
        }

        if (ret != MSERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
}

void ADecDemo::OutputFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this](){ return signal_->outQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        if (adec_->ReleaseOutputBuffer(index) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
    }
}

ADecDemoCallback::ADecDemoCallback(shared_ptr<ADecSignal> signal)
    : signal_(signal)
{
}

void ADecDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void ADecDemoCallback::OnOutputFormatChanged(const Format &format)
{
    cout << "OnOutputFormatChanged received" << endl;
}

void ADecDemoCallback::OnInputBufferAvailable(uint32_t index)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inCond_.notify_all();
}

void ADecDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->outCond_.notify_all();
}