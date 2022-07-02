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

#include "audioenc_ndk_sample.h"
#include "audio_info.h"

// #include "ndk_av_memory.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

namespace {
    constexpr uint32_t DEFAULT_SAMPLE_RATE = 44100;
    constexpr uint32_t DEFAULT_CHANNELS = 2;
    constexpr uint32_t SAMPLE_DURATION_US = 23000;
    constexpr uint32_t SAMPLE_SIZE = 4096;
    constexpr uint32_t ES_LENGTH = 1500;
    constexpr bool NEED_DUMP = true;
    const string MIME_TYPE = "audio/mp4a-latm";
    const char * INP_DIR = "/data/media/S16LE.pcm";
    const char * OUT_DIR = "/data/media/S16LE_out.es";
}

void AencAsyncError(AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

void AencAsyncStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

void AencAsyncNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    AEncSignal* signal_ = static_cast<AEncSignal *>(userData);
    cout << "InputAvailable, index = " << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void AencAsyncNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data, AVCodecBufferAttr *attr, void *userData)
{
    AEncSignal* signal_ = static_cast<AEncSignal *>(userData);
    cout << "OutputAvailable, index = " << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->sizeQueue_.push(attr->size);
    signal_->outBufferQueue_.push(data);
    signal_->outCond_.notify_all();
}


AEncNdkSample::~AEncNdkSample()
{
    Release();

    delete signal_;
    signal_ = nullptr;
}

void AEncNdkSample::RunAudioEnc(void)
{
    aenc_ = CreateAudioEncoder();
    NDK_CHECK_AND_RETURN_LOG(aenc_ != nullptr, "Fatal: CreateAudioEncoder fail");
    struct AVFormat *format = CreateFormat();
    NDK_CHECK_AND_RETURN_LOG(format != nullptr, "Fatal: CreateFormat fail");

    NDK_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
    NDK_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
    NDK_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: Start fail");
    // sleep(10); // start run 10s
    while (isRunning_.load()) {};
    NDK_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Start fail");
    NDK_CHECK_AND_RETURN_LOG(Release() == AV_ERR_OK, "Fatal: Release fail");
}

struct AVCodec* AEncNdkSample::CreateAudioEncoder(void)
{
    struct AVCodec *codec = OH_AVCODEC_CreateAudioEncoderByMime(MIME_TYPE.c_str());
    NDK_CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Fatal: OH_AVCODEC_CreateAudioEncoderByMime");

    signal_ = new AEncSignal();
    NDK_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, nullptr, "Fatal: No Memory");

    cb_.onAsyncError = AencAsyncError;
    cb_.onAsyncStreamChanged = AencAsyncStreamChanged;
    cb_.onAsyncNeedInputData = AencAsyncNeedInputData;
    cb_.onAsyncNewOutputData = AencAsyncNewOutputData;
    int32_t ret = OH_AVCODEC_AudioEncoderSetCallback(codec, cb_, static_cast<void *>(signal_));
    NDK_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_AVCODEC_AudioEncoderSetCallback");
    return codec;
}

struct AVFormat* AEncNdkSample::CreateFormat(void)
{
    struct AVFormat *format = OH_AV_CreateFormat();
    string channel_count = "channel_count";
    string sample_rate = "sample_rate";
    string audio_sample_format = "audio_sample_format";

    if (format != NULL) {
        (void)OH_AV_FormatPutIntValue(format, channel_count.c_str(), DEFAULT_CHANNELS);
        (void)OH_AV_FormatPutIntValue(format, sample_rate.c_str(), DEFAULT_SAMPLE_RATE);
        (void)OH_AV_FormatPutIntValue(format, audio_sample_format.c_str(), AudioStandard::SAMPLE_S16LE);
    }
    return format;
}


int32_t AEncNdkSample::Configure(struct AVFormat *format)
{
    return OH_AVCODEC_AudioEncoderConfigure(aenc_, format);
}


int32_t AEncNdkSample::Prepare()
{
    return OH_AVCODEC_AudioEncoderPrepare(aenc_);
}

int32_t AEncNdkSample::Start()
{
    isRunning_.store(true);

    testFile_ = std::make_unique<std::ifstream>();
    NDK_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    testFile_->open(INP_DIR, std::ios::in | std::ios::binary);

    inputLoop_ = make_unique<thread>(&AEncNdkSample::InputFunc, this);
    NDK_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEncNdkSample::OutputFunc, this);
    NDK_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AVCODEC_AudioEncoderStart(aenc_);
}

int32_t AEncNdkSample::Stop()
{
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inQueue_.push(10000);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outQueue_.push(10000);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
    }

    return OH_AVCODEC_AudioEncoderStop(aenc_);
}

int32_t AEncNdkSample::Flush()
{
    return OH_AVCODEC_AudioEncoderFlush(aenc_);
}

int32_t AEncNdkSample::Reset()
{
    return OH_AVCODEC_AudioEncoderReset(aenc_);
}

int32_t AEncNdkSample::Release()
{
    AVErrCode ret = AV_ERR_OK;
    if (aenc_ != nullptr) {
        ret = OH_AVCODEC_DestroyAudioEncoder(aenc_);
        aenc_ = (ret == AV_ERR_OK) ? nullptr : aenc_;
    }
    return ret;
}

int32_t AEncNdkSample::SetParameter(AVFormat *format)
{
    return OH_AVCODEC_AudioEncoderSetParameter(aenc_, format);
}

void AEncNdkSample::InputFunc()
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
        AVMemory *buffer = reinterpret_cast<AVMemory *>(signal_->inBufferQueue_.front());
        NDK_CHECK_AND_RETURN_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        NDK_CHECK_AND_RETURN_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");
        if (frameCount_ < ES_LENGTH) {
            char *fileBuffer = (char *)malloc(sizeof(char) * SAMPLE_SIZE + 1);
            NDK_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail");

            (void)testFile_->read(fileBuffer, SAMPLE_SIZE);
            if (testFile_->eof()) {
                free(fileBuffer);
                cout << "Finish" << endl;
                break;
            }

            if (memcpy_s(OH_AV_MemoryGetAddr(buffer), OH_AV_MemoryGetSize(buffer), fileBuffer, SAMPLE_SIZE) != EOK) {
                free(fileBuffer);
                cout << "Fatal: memcpy fail" << endl;
                break;
            }
            free(fileBuffer);
        }

        struct AVCodecBufferAttr attr;
        attr.offset = 0;
        if (frameCount_ == ES_LENGTH) {
            attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            attr.size = 0;
            attr.presentationTimeUs = 0;
            cout << "EOS Frame, frameCount = " << frameCount_ << endl;
            isRunning_.store(false);
        } else {
            attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            attr.size = SAMPLE_SIZE;
            attr.presentationTimeUs = timeStamp_;
        }

        AVErrCode ret = OH_AVCODEC_AudioEncoderPushInputData(aenc_, index, attr);

        timeStamp_ += SAMPLE_DURATION_US;
        frameCount_ ++;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();

        if (ret != AV_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
}

void AEncNdkSample::OutputFunc()
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
        auto buffer = signal_->outBufferQueue_.front();
        if (buffer == nullptr) {
            cout << "getOutPut Buffer fail" << endl;
        }
        uint32_t size = signal_->sizeQueue_.front();
        cout << "GetOutputBuffer size : " << size << " frameCount_ =  " << frameCount_ << endl;
        if (NEED_DUMP) {
            FILE *outFile;
            outFile = fopen(OUT_DIR, "a");
            if (outFile == nullptr) {
                cout << "dump data fail" << endl;
            } else {
                fwrite(OH_AV_MemoryGetAddr(buffer), 1, size, outFile);
            }
            fclose(outFile);
        }

        if (OH_AVCODEC_AudioEncoderFreeOutputData(aenc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
        signal_->sizeQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}
