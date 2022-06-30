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

#include "audiodec_ndk_sample.h"
#include "audio_info.h"


// #include "ndk_av_memory.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

namespace {
    constexpr uint32_t DEFAULT_SAMPLE_RATE = 44100;
    constexpr uint32_t DEFAULT_CHANNELS = 2;
    constexpr uint32_t SAMPLE_DURATION_US = 23000;
    constexpr bool NEED_DUMP = true;
    const string MIME_TYPE = "audio/mp4a-latm";
    const char * INP_DIR = "/data/media/AAC_48000_32_1.aac";
    const char * OUT_DIR = "/data/media/AAC_48000_32_1_out.es";
    constexpr uint32_t ES[] = {283, 336, 291, 405, 438, 411, 215, 215, 313, 270, 342, 641, 554, 545, 545, 546,
             541, 540, 542, 552, 537, 533, 498, 472, 445, 430, 445, 427, 414, 386, 413, 370, 380,
             401, 393, 369, 391, 367, 395, 396, 396, 385, 391, 384, 395, 392, 386, 388, 384, 379,
             376, 381, 375, 373, 349, 391, 357, 384, 395, 384, 380, 386, 372, 386, 383, 378, 385,
             385, 384, 342, 390, 379, 387, 386, 393, 397, 362, 393, 394, 391, 383, 385, 377, 379,
             381, 369, 375, 379, 346, 382, 356, 361, 366, 394, 393, 385, 362, 406, 399, 384, 377,
             385, 389, 375, 346, 396, 388, 381, 383, 352, 357, 397, 382, 395, 376, 388, 373, 374,
             353, 383, 384, 393, 379, 348, 364, 389, 380, 381, 388, 423, 392, 381, 368, 351, 391,
             355, 358, 395, 390, 385, 382, 383, 388, 388, 389, 376, 379, 376, 384, 369, 354, 390,
             389, 396, 393, 382, 385, 353, 383, 381, 377, 411, 387, 390, 377, 349, 381, 390, 378,
             373, 375, 381, 351, 392, 381, 380, 381, 378, 387, 379, 383, 348, 386, 364, 386, 371,
             399, 399, 385, 380, 355, 397, 395, 382, 380, 386, 352, 387, 390, 373, 372, 388, 378,
             385, 368, 385, 370, 378, 373, 383, 368, 373, 388, 351, 384, 391, 387, 389, 383, 355,
             361, 392, 386, 354, 394, 392, 397, 392, 352, 381, 395, 349, 383, 390, 392, 350, 393,
             393, 385, 389, 393, 382, 378, 384, 378, 375, 373, 375, 389, 377, 383, 387, 373, 344,
             388, 379, 391, 373, 384, 358, 361, 391, 394, 363, 350, 361, 395, 399, 389, 398, 375,
             398, 400, 381, 354, 363, 366, 400, 400, 356, 370, 400, 394, 398, 385, 378, 372, 354,
             359, 393, 381, 363, 396, 396, 355, 390, 356, 355, 371, 399, 367, 406, 375, 377, 405,
             401, 390, 393, 392, 384, 386, 374, 358, 397, 389, 393, 385, 345, 379, 357, 388, 356,
             381, 389, 367, 358, 391, 360, 394, 396, 357, 395, 388, 394, 383, 357, 383, 392, 394,
             376, 379, 356, 386, 395, 387, 377, 377, 389, 377, 385, 351, 387, 350, 388, 384, 345,
             358, 368, 399, 394, 385, 384, 395, 378, 387, 386, 386, 376, 375, 382, 351, 359, 356,
             401, 388, 363, 406, 363, 374, 435, 366, 400, 393, 392, 371, 391, 359, 359, 397, 388,
             390, 420, 411, 369, 384, 382, 383, 383, 375, 381, 361, 380, 348, 379, 386, 379, 379,
             386, 371, 352, 378, 378, 388, 384, 385, 352, 355, 387, 383, 379, 362, 386, 399, 376,
             390, 350, 387, 357, 403, 398, 397, 360, 351, 394, 400, 399, 393, 388, 395, 370, 377,
             395, 360, 346, 381, 370, 390, 380, 391, 387, 382, 384, 383, 354, 349, 394, 358, 387,
             400, 386, 402, 354, 396, 387, 391, 365, 377, 359, 361, 365, 395, 388, 388, 384, 388,
             378, 374, 382, 376, 377, 389, 378, 341, 390, 376, 381, 375, 414, 368, 369, 387, 411,
             396, 391, 378, 389, 349, 383, 344, 381, 387, 380, 353, 361, 391, 365, 390, 396, 382,
             386, 385, 385, 409, 387, 386, 378, 372, 372, 374, 349, 388, 389, 348, 395, 380, 382,
             388, 375, 347, 383, 359, 389, 368, 361, 405, 398, 393, 395, 359, 360, 395, 395, 362,
             354, 388, 348, 388, 386, 390, 350, 388, 356, 369, 364, 404, 404, 391, 394, 385, 439,
             432, 375, 366, 441, 362, 367, 382, 374, 346, 391, 371, 354, 376, 390, 373, 382, 385,
             389, 378, 377, 347, 414, 338, 348, 385, 352, 385, 386, 381, 388, 387, 364, 465, 405,
             443, 387, 339, 376, 337, 379, 387, 370, 374, 358, 354, 357, 393, 356, 381, 357, 407,
             361, 397, 362, 394, 394, 392, 394, 391, 381, 386, 379, 354, 351, 392, 408, 393, 389,
             388, 385, 375, 388, 375, 388, 375, 354, 384, 379, 386, 394, 383, 359, 405, 395, 352,
             345, 403, 427, 373, 380, 350, 415, 378, 434, 385, 388, 387, 400, 405, 329, 391, 356,
             419, 358, 359, 375, 367, 391, 359, 369, 361, 376, 378, 379, 348, 390, 345, 388, 390,
             406, 349, 368, 364, 391, 384, 401, 384, 391, 361, 399, 359, 386, 392, 382, 386, 380,
             383, 345, 376, 393, 400, 395, 343, 352, 354, 381, 388, 357, 393, 389, 384, 389, 388,
             384, 404, 372, 358, 381, 352, 355, 485, 393, 371, 376, 389, 377, 391, 387, 376, 342,
             390, 375, 379, 396, 376, 402, 353, 392, 382, 383, 387, 386, 372, 377, 382, 388, 381,
             387, 357, 393, 385, 346, 389, 388, 357, 362, 404, 398, 397, 402, 371, 351, 370, 362,
             350, 388, 399, 402, 406, 377, 396, 359, 372, 390, 392, 368, 383, 346, 384, 381, 379,
             367, 384, 389, 381, 371, 358, 422, 372, 382, 374, 444, 412, 369, 362, 373, 389, 401,
             383, 380, 366, 365, 361, 379, 372, 345, 382, 375, 376, 375, 382, 356, 395, 383, 384,
             391, 361, 396, 407, 365, 351, 385, 378, 403, 344, 352, 387, 397, 399, 377, 371, 381,
             415, 382, 388, 368, 383, 405, 390, 386, 384, 374, 375, 381, 371, 372, 374, 377, 346,
             358, 381, 377, 359, 385, 396, 385, 390, 389, 391, 375, 357, 389, 390, 377, 370, 379,
             351, 381, 381, 380, 371, 386, 389, 389, 383, 362, 393, 388, 355, 396, 383, 352, 384,
             352, 383, 362, 396, 385, 396, 357, 388, 382, 377, 373, 379, 383, 386, 350, 393, 355,
             380, 401, 392, 391, 402, 391, 427, 407, 394, 332, 398, 367, 373, 343, 381, 383, 386,
             382, 349, 353, 393, 378, 386, 375, 390, 356, 392, 384, 387, 380, 381, 385, 386, 383,
             378, 379, 359, 381, 382, 388, 357, 357, 397, 358, 424, 382, 352, 409, 374, 368, 365,
             399, 352, 393, 389, 385, 352, 380, 398, 389, 385, 387, 387, 353, 402, 396, 386, 357,
             395, 368, 369, 407, 394, 383, 362, 380, 385, 368, 375, 365, 379, 377, 388, 380, 346,
             383, 381, 399, 359, 386, 455, 368, 406, 377, 339, 381, 377, 373, 371, 338}; // replace of self frame length
             constexpr uint32_t ES_LENGTH = sizeof(ES) / sizeof(uint32_t);
}

void AdecAsyncError(AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

void AdecAsyncStreamChanged(AVCodec *codec, AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

void AdecAsyncNeedInputData(AVCodec *codec, uint32_t index, AVMemory *data, void *userData)
{
    ADecSignal* signal_ = static_cast<ADecSignal *>(userData);
    cout << "InputAvailable, index = " << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void AdecAsyncNewOutputData(AVCodec *codec, uint32_t index, AVMemory *data, AVCodecBufferAttr *attr, void *userData)
{
    ADecSignal* signal_ = static_cast<ADecSignal *>(userData);
    cout << "OutputAvailable, index = " << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->sizeQueue_.push(attr->size);
    signal_->outBufferQueue_.push(data);
    signal_->outCond_.notify_all();
}

ADecNdkSample::~ADecNdkSample()
{
    OH_AVCODEC_DestroyAudioDecoder(adec_);
    signal_ = nullptr;
    delete signal_;
}

void ADecNdkSample::RunAudioDec(void)
{
    adec_ = CreateAudioDecoder();
    struct AVFormat *format = CreateFormat();
    if (adec_ == nullptr || format == nullptr) {
        OH_AVCODEC_DestroyAudioDecoder(adec_);
        OH_AV_DestroyFormat(format);
        return;
    }
    NDK_CHECK_AND_RETURN_LOG(Configure(format) == AV_ERR_OK, "Fatal: Configure fail");
    NDK_CHECK_AND_RETURN_LOG(Prepare() == AV_ERR_OK, "Fatal: Prepare fail");
    NDK_CHECK_AND_RETURN_LOG(Start() == AV_ERR_OK, "Fatal: Start fail");
    sleep(10); // start run 10s
    NDK_CHECK_AND_RETURN_LOG(Stop() == AV_ERR_OK, "Fatal: Start fail");
    Release();
}

struct AVCodec* ADecNdkSample::CreateAudioDecoder(void)
{
    struct AVCodec *codec = OH_AVCODEC_CreateAudioDecoderByMime(MIME_TYPE.c_str());
    // struct AVCodec *codec = OH_AVCODEC_CreateAudioDecoderByName("avdec_aac");
    NDK_CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "Fatal: OH_AVCODEC_CreateAudioDecoderByMime");

    signal_ = new ADecSignal();
    NDK_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, nullptr, "Fatal: No Memory");

    cb_.onAsyncError = AdecAsyncError;
    cb_.onAsyncStreamChanged = AdecAsyncStreamChanged;
    cb_.onAsyncNeedInputData = AdecAsyncNeedInputData;
    cb_.onAsyncNewOutputData = AdecAsyncNewOutputData;
    int32_t ret = OH_AVCODEC_AudioDecoderSetCallback(codec, cb_, static_cast<void *>(signal_));
    NDK_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, NULL, "Fatal: OH_AVCODEC_AudioDecoderSetCallback");
    return codec;
}

struct AVFormat* ADecNdkSample::CreateFormat(void)
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

int32_t ADecNdkSample::Configure(struct AVFormat *format)
{
    return OH_AVCODEC_AudioDecoderConfigure(adec_, format);
}

int32_t ADecNdkSample::Prepare()
{
    return OH_AVCODEC_AudioDecoderPrepare(adec_);
}

int32_t ADecNdkSample::Start()
{
    isRunning_.store(true);

    testFile_ = std::make_unique<std::ifstream>();
    NDK_CHECK_AND_RETURN_RET_LOG(testFile_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");
    testFile_->open(INP_DIR, std::ios::in | std::ios::binary);

    inputLoop_ = make_unique<thread>(&ADecNdkSample::InputFunc, this);
    NDK_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecNdkSample::OutputFunc, this);
    NDK_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AVCODEC_AudioDecoderStart(adec_);
}

int32_t ADecNdkSample::Stop()
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

    return OH_AVCODEC_AudioDecoderStop(adec_);
}

int32_t ADecNdkSample::Flush()
{
    return OH_AVCODEC_AudioDecoderFlush(adec_);
}

int32_t ADecNdkSample::Reset()
{
    return OH_AVCODEC_AudioDecoderReset(adec_);
}

void ADecNdkSample::Release()
{
    OH_AVCODEC_DestroyAudioDecoder(adec_);
}

int32_t ADecNdkSample::SetParameter(AVFormat *format)
{
    return OH_AVCODEC_AudioDecoderSetParameter(adec_, format);
}

void ADecNdkSample::InputFunc()
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

        uint32_t bufferSize = ES[frameCount_]; // replace with the actual size
        char *fileBuffer = (char *)malloc(sizeof(char) * bufferSize + 1);
        NDK_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail");

        (void)testFile_->read(fileBuffer, bufferSize);
        if (testFile_->eof()) {
            free(fileBuffer);
            cout << "Finish" << endl;
            break;
        }

        if (memcpy_s(OH_AV_MemoryGetAddr(buffer), OH_AV_MemoryGetSize(buffer), fileBuffer, bufferSize) != EOK) {
            free(fileBuffer);
            cout << "Fatal: memcpy fail" << endl;
            break;
        }

        struct AVCodecBufferAttr attr;
        if (frameCount_ == ES_LENGTH + 1) {
            attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            attr.presentationTimeUs = 0;
            attr.size = 0;
            attr.offset = 0;
            isRunning_.store(false);
        } else {
            attr.presentationTimeUs = timeStamp_;
            attr.size = bufferSize;
            attr.offset = 0;
            if (frameCount_ == 0 && MIME_TYPE == "audio/vorbis") {
                attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            } else {
                attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            }
        }

        AVErrCode ret = OH_AVCODEC_AudioDecoderPushInputData(adec_, index, attr);

        free(fileBuffer);
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


void ADecNdkSample::OutputFunc()
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

        if (OH_AVCODEC_AudioDecoderFreeOutputData(adec_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
        signal_->sizeQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}
