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

#include "audioenc_native_sample.h"
#include "audio_info.h"

// #include "native_avmemory.h"

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
    unique_lock<mutex> lock(signal_->inMutex_);
    if (signal_->isFlushing_.load() || signal_->isStop_.load()) {
        return;
    }
    cout << "InputAvailable, index = " << index << endl;

    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void AencAsyncNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data, AVCodecBufferAttr *attr, void *userData)
{
    AEncSignal* signal_ = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal_->outMutex_);
    if (signal_->isFlushing_.load() || signal_->isStop_.load()) {
        return;
    }
    cout << "OutputAvailable, index = " << index << endl;
    signal_->outQueue_.push(index);
    signal_->outSizeQueue_.push(attr->size);
    signal_->outBufferQueue_.push(data);
    signal_->outCond_.notify_all();
}


AEncNdkSample::~AEncNdkSample()
{
    Release();

    delete signal_;
    signal_ = nullptr;
}

// // start->flush->start->stop->release
// void AEncNdkSample::RunAudioEnc(void)
// {
//     aenc_ = CreateAudioEncoder();
//     NATIVE_CHECK_AND_RETURN_LOG(aenc_ != nullptr, "Fatal: CreateAudioEncoder fail");
//     struct AVFormat *format = CreateFormat();
//     NATIVE_CHECK_AND_RETURN_LOG(format != nullptr, "Fatal: CreateFormat fail");

//     NATIVE_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
//     NATIVE_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
//     NATIVE_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: Start fail");
//     usleep(80000); // start run 80ms
//     cout << "Go flush" << endl;
//     NATIVE_CHECK_AND_RETURN_LOG(Flush() == AV_ERR_OK, "Fatal: Flush fail");
//     cout << "Flush success" << endl;
//     cout << "Restart:" << endl;
//     NATIVE_CHECK_AND_RETURN_LOG(OH_AudioEncoder_Start(aenc_) == AV_ERR_OK, "Fatal: ReStart fail");
//     while (signal_->isEOS_.load()) {};
//     NATIVE_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Start fail");
//     NATIVE_CHECK_AND_RETURN_LOG(Release() == AV_ERR_OK, "Fatal: Release fail");
// }

// start->stop->start->stop->release
// void AEncNdkSample::RunAudioEnc(void)
// {
//     aenc_ = CreateAudioEncoder();
//     NATIVE_CHECK_AND_RETURN_LOG(aenc_ != nullptr, "Fatal: CreateAudioEncoder fail");
//     struct AVFormat *format = CreateFormat();
//     NATIVE_CHECK_AND_RETURN_LOG(format != nullptr, "Fatal: CreateFormat fail");

//     NATIVE_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
//     NATIVE_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
//     NATIVE_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: Start fail");
//     usleep(80000); // start run 80ms
//     cout << "Go Stop" << endl;
//     NATIVE_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Stop fail");
//     cout << "Stop success" << endl;
//     cout << "Restart:" << endl;
//     NATIVE_CHECK_AND_RETURN_LOG(OH_AudioEncoder_Start(aenc_) == AV_ERR_OK, "Fatal: ReStart fail");
//     while (signal_->isEOS_.load()) {};
//     NATIVE_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Start fail");
//     NATIVE_CHECK_AND_RETURN_LOG(Release() == AV_ERR_OK, "Fatal: Release fail");
// }

// start->flush->reset->configure->prepare->start->stop->release
void AEncNdkSample::RunAudioEnc(void)
{
    aenc_ = CreateAudioEncoder();
    NATIVE_CHECK_AND_RETURN_LOG(aenc_ != nullptr, "Fatal: CreateAudioEncoder fail");
    struct AVFormat *format = CreateFormat();
    NATIVE_CHECK_AND_RETURN_LOG(format != nullptr, "Fatal: CreateFormat fail");

    NATIVE_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
    NATIVE_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
    NATIVE_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: Start fail");
    usleep(80000); // start run 80ms

    cout << "Go flush" << endl;
    NATIVE_CHECK_AND_RETURN_LOG(Flush() == AV_ERR_OK, "Fatal: Flush fail");
    cout << "Flush success" << endl;

    cout << "Go reset" << endl;
    NATIVE_CHECK_AND_RETURN_LOG(Reset() == AV_ERR_OK, "Fatal: Reset fail");
    cout << "Reset success" << endl;

    NATIVE_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
    cout << "Configure success" << endl;
    NATIVE_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
    cout << "Prepare success" << endl;

    cout << "Restart:" << endl;
    // NATIVE_CHECK_AND_RETURN_LOG(OH_AudioEncoder_Start(aenc_) == AV_ERR_OK, "Fatal: ReStart fail");
    NATIVE_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: ReStart fail");
    while (signal_->isEOS_.load()) {};
    NATIVE_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Start fail");
    NATIVE_CHECK_AND_RETURN_LOG(Release() == AV_ERR_OK, "Fatal: Release fail");
}

struct AVCodec* AEncNdkSample::CreateAudioEncoder(void)
{
    struct AVCodec *codec = OH_AudioEncoder_CreateByMime(MIME_TYPE.c_str());
    NATIVE_CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Fatal: OH_AudioEncoder_CreateByMime");

    signal_ = new AEncSignal();
    NATIVE_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, nullptr, "Fatal: No Memory");

    cb_.onError = AencAsyncError;
    cb_.onStreamChanged = AencAsyncStreamChanged;
    cb_.onNeedInputData = AencAsyncNeedInputData;
    cb_.onNeedOutputData = AencAsyncNewOutputData;
    int32_t ret = OH_AudioEncoder_SetCallback(codec, cb_, static_cast<void *>(signal_));
    NATIVE_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_AudioEncoder_SetCallback");
    return codec;
}

struct AVFormat *AEncNdkSample::CreateFormat(void)
{
    struct AVFormat *format = OH_AVFormat_Create();
    string channel_count = "channel_count";
    string sample_rate = "sample_rate";
    string audio_sample_format = "audio_sample_format";

    if (format != NULL) {
        (void)OH_AVFormat_SetIntValue(format, channel_count.c_str(), DEFAULT_CHANNELS);
        (void)OH_AVFormat_SetIntValue(format, sample_rate.c_str(), DEFAULT_SAMPLE_RATE);
        (void)OH_AVFormat_SetIntValue(format, audio_sample_format.c_str(), AudioStandard::SAMPLE_S16LE);
    }
    return format;
}

int32_t AEncNdkSample::Configure(struct AVFormat *format)
{
    return OH_AudioEncoder_Configure(aenc_, format);
}

int32_t AEncNdkSample::Prepare()
{
    return OH_AudioEncoder_Prepare(aenc_);
}

int32_t AEncNdkSample::Start()
{
    unique_lock<mutex> outLock(signal_->outMutex_);
    unique_lock<mutex> inLock(signal_->inMutex_);
    isRunning_.store(true);
    signal_->isEOS_.store(false);
    signal_->isStop_.store(false);

    outLock.unlock();
    inLock.unlock();

    if (testFile_ == nullptr) {
        testFile_ = std::make_unique<std::ifstream>();
        NATIVE_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
        testFile_->open(INP_DIR, std::ios::in | std::ios::binary);
    }

    if (inputLoop_ == nullptr) {
        inputLoop_ = make_unique<thread>(&AEncNdkSample::InputFunc, this);
        NATIVE_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    }

    if (outputLoop_ == nullptr) {
        outputLoop_ = make_unique<thread>(&AEncNdkSample::OutputFunc, this);
        NATIVE_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    }
    return OH_AudioEncoder_Start(aenc_);
}

int32_t AEncNdkSample::Stop()
{
    unique_lock<mutex> outLock0(signal_->outMutex_);
    unique_lock<mutex> inLock0(signal_->inMutex_);
    signal_->isStop_.store(true);
    outLock0.unlock();
    inLock0.unlock();
    int32_t ret = OH_AudioEncoder_Stop(aenc_); // 不能上锁，否则会阻塞回调，导致执行结束后，阻塞的回调往队列里push无效的index
    unique_lock<mutex> outLock(signal_->outMutex_);
    unique_lock<mutex> inLock(signal_->inMutex_);

    ClearIntQueue(signal_->outQueue_);
    ClearIntQueue(signal_->outSizeQueue_);
    ClearBufferQueue(signal_->outBufferQueue_);
    ClearIntQueue(signal_->inQueue_);
    ClearBufferQueue(signal_->inBufferQueue_);

    return ret;
}

void AEncNdkSample::ClearIntQueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void AEncNdkSample::ClearBufferQueue(std::queue<AVMemory *> &q)
{
    std::queue<AVMemory *> empty;
    swap(empty, q);
}

int32_t AEncNdkSample::Flush()
{
    unique_lock<mutex> outLock0(signal_->outMutex_);
    unique_lock<mutex> inLock0(signal_->inMutex_);
    signal_->isFlushing_.store(true);
    outLock0.unlock();
    inLock0.unlock();
    int32_t ret = OH_AudioEncoder_Flush(aenc_);
    unique_lock<mutex> outLock(signal_->outMutex_);
    unique_lock<mutex> inLock(signal_->inMutex_);

    cout << "flushing .."<< endl;

    ClearIntQueue(signal_->outQueue_);
    ClearIntQueue(signal_->outSizeQueue_);
    ClearBufferQueue(signal_->outBufferQueue_);
    ClearIntQueue(signal_->inQueue_);
    ClearBufferQueue(signal_->inBufferQueue_);

    signal_->isFlushing_.store(false);

    return ret;
}

int32_t AEncNdkSample::Reset()
{
    unique_lock<mutex> outLock0(signal_->outMutex_);
    unique_lock<mutex> inLock0(signal_->inMutex_);
    signal_->isStop_.store(true);
    outLock0.unlock();
    inLock0.unlock();
    int32_t ret = OH_AudioEncoder_Reset(aenc_);
    unique_lock<mutex> outLock(signal_->outMutex_);
    unique_lock<mutex> inLock(signal_->inMutex_);

    ClearIntQueue(signal_->outQueue_);
    ClearIntQueue(signal_->outSizeQueue_);
    ClearBufferQueue(signal_->outBufferQueue_);
    ClearIntQueue(signal_->inQueue_);
    ClearBufferQueue(signal_->inBufferQueue_);

    return ret;
}

int32_t AEncNdkSample::Release()
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

    AVErrCode ret = AV_ERR_OK;
    if (aenc_ != nullptr) {
        ret = OH_AudioEncoder_Destroy(aenc_);
        aenc_ = (ret == AV_ERR_OK) ? nullptr : aenc_;
    }
    return ret;
}

int32_t AEncNdkSample::SetParameter(AVFormat *format)
{
    return OH_AudioEncoder_SetParameter(aenc_, format);
}

void AEncNdkSample::InputFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            return signal_->inQueue_.size() > 0;
        });

        if (!isRunning_.load()) {
            break;
        }
        AVMemory *buffer = reinterpret_cast<AVMemory *>(signal_->inBufferQueue_.front());

        uint32_t index = signal_->inQueue_.front();
        if (signal_->isFlushing_.load() || signal_->isStop_.load() || signal_->isEOS_.load() || buffer == nullptr) {
            signal_->inQueue_.pop();
            signal_->inBufferQueue_.pop();
            cout << "isFlushing_,  isStop_, OutputFunc pop" << endl;
            continue;
        }

        NATIVE_CHECK_AND_RETURN_LOG(testFile_ != nullptr && testFile_->is_open(), "Fatal: open file fail");
        if (frameCount_ < ES_LENGTH) {
            char *fileBuffer = (char *)malloc(sizeof(char) * SAMPLE_SIZE + 1);
            NATIVE_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail");

            (void)testFile_->read(fileBuffer, SAMPLE_SIZE);
            if (testFile_->eof()) {
                free(fileBuffer);
                cout << "Finish" << endl;
                break;
            }

            if (memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, SAMPLE_SIZE) != EOK) {
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
            attr.pts = 0;
            cout << "EOS Frame, frameCount = " << frameCount_ << endl;
            signal_->isEOS_.store(true);
        } else {
            attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            attr.size = SAMPLE_SIZE;
            attr.pts = timeStamp_;
        }

        AVErrCode ret = OH_AudioEncoder_PushInputData(aenc_, index, attr);

        timeStamp_ += SAMPLE_DURATION_US;
        frameCount_ ++;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();

        if (ret != AV_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    cout <<"EXIT InputFunc ... " << endl; 
}

void AEncNdkSample::OutputFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            return signal_->outQueue_.size() > 0;
            });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        if (signal_->isFlushing_.load() || signal_->isStop_.load() ) {
            signal_->outQueue_.pop();
            signal_->outSizeQueue_.pop();
            signal_->outBufferQueue_.pop();
            cout << "isFlushing_ or isStop_, OutputFunc pop" << endl;
            continue;
        }
        auto buffer = signal_->outBufferQueue_.front();
        if (buffer == nullptr) {
            cout << "getOutPut Buffer fail" << endl;
        }
        uint32_t size = signal_->outSizeQueue_.front();
        cout << "GetOutputBuffer size : " << size << " frameCount_ =  " << frameCount_ << endl;
        if (NEED_DUMP) {
            FILE *outFile;
            outFile = fopen(OUT_DIR, "a");
            if (outFile == nullptr) {
                cout << "dump data fail" << endl;
            } else {
                fwrite(OH_AVMemory_GetAddr(buffer), 1, size, outFile);
            }
            fclose(outFile);
        }

        if (OH_AudioEncoder_FreeOutputData(aenc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
        signal_->outSizeQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
    cout <<"EXIT OutputFunc ... " << endl; 
}
