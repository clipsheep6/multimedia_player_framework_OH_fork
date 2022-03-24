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

#include "avcodec_aenc_demo.h"
#include <iostream>
#include <unistd.h>
#include "audio_info.h"
#include "securec.h"
#include "demo_log.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
    constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
    constexpr uint32_t DEFAULT_CHANNELS = 2;
    constexpr uint32_t SAMPLE_DURATION_US = 20000;
    constexpr uint32_t SAMPLE_SIZE = 3840; // 20ms
}

void AEncDemo::RunCase()
{
    DEMO_CHECK_AND_RETURN_LOG(CreateAenc() == MSERR_OK, "Fatal: CreateAenc fail");

    Format format;
    format.PutIntValue("channel_count", DEFAULT_CHANNELS);
    format.PutIntValue("sample_rate", DEFAULT_SAMPLE_RATE);
    format.PutIntValue("audio_sample_format", AudioStandard::SAMPLE_S16LE);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == MSERR_OK, "Fatal: Configure fail");

    DEMO_CHECK_AND_RETURN_LOG(Prepare() == MSERR_OK, "Fatal: Prepare fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == MSERR_OK, "Fatal: Start fail");
    sleep(3); // start run 3s
    DEMO_CHECK_AND_RETURN_LOG(Stop() == MSERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == MSERR_OK, "Fatal: Release fail");
}

int32_t AEncDemo::CreateAenc()
{
    aenc_ = AudioEncoderFactory::CreateByMime("audio/opus");
    DEMO_CHECK_AND_RETURN_RET_LOG(aenc_ != nullptr, MSERR_UNKNOWN, "Fatal: CreateByMime fail");

    signal_ = make_shared<AEncSignal>();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    cb_ = make_unique<AEncDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(aenc_->SetCallback(cb_) == MSERR_OK, MSERR_UNKNOWN, "Fatal: SetCallback fail");

    return MSERR_OK;
}

int32_t AEncDemo::Configure(const Format &format)
{
    return aenc_->Configure(format);
}

int32_t AEncDemo::Prepare()
{
    return aenc_->Prepare();
}

int32_t AEncDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AEncDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEncDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, MSERR_UNKNOWN, "Fatal: No memory");

    return aenc_->Start();
}

int32_t AEncDemo::Stop()
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

    return aenc_->Stop();
}

int32_t AEncDemo::Flush()
{
    return aenc_->Flush();
}

int32_t AEncDemo::Reset()
{
    return aenc_->Reset();
}

int32_t AEncDemo::Release()
{
    return aenc_->Release();
}

void AEncDemo::InputFunc()
{
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
        auto buffer = aenc_->GetInputBuffer(index);
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");

        if (memset_s(buffer->GetBase(), buffer->GetSize(), sample_, SAMPLE_SIZE) != EOK) {
            cout << "Fatal: memset fail" << endl;
            break;
        }

        AVCodecBufferInfo info;
        info.size = SAMPLE_SIZE;
        info.offset = 0;
        info.presentationTimeUs = timeStamp_;

        int32_t ret = aenc_->QueueInputBuffer(index, info, AVCODEC_BUFFER_FLAG_NONE);

        sample_ = sample_ <= 0 ? 0xFF : (sample_ - 1);
        timeStamp_ += SAMPLE_DURATION_US;
        signal_->inQueue_.pop();

        if (ret != MSERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
}

void AEncDemo::OutputFunc()
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
        auto buffer = aenc_->GetOutputBuffer(index);
        if (buffer == nullptr) {
            cout << "Fatal: GetOutputBuffer fail" << endl;
            break;
        }
        if (aenc_->ReleaseOutputBuffer(index) != MSERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
    }
}

AEncDemoCallback::AEncDemoCallback(shared_ptr<AEncSignal> signal)
    : signal_(signal)
{
}

void AEncDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void AEncDemoCallback::OnOutputFormatChanged(const Format &format)
{
    cout << "OnOutputFormatChanged received" << endl;
}

void AEncDemoCallback::OnInputBufferAvailable(uint32_t index)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inCond_.notify_all();
}

void AEncDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->outCond_.notify_all();
}