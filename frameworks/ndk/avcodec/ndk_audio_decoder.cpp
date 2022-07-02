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

#include <list>
#include "ndk_av_codec.h"
#include "ndk_av_magic.h"
#include "native_window.h"
#include "avcodec_audio_decoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioDecoderObject"};
}

using namespace OHOS::Media;

struct AudioDecoderObject : public AVCodec {
    explicit AudioDecoderObject(const std::shared_ptr<AVCodecAudioDecoder> &decoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_AUDIO_DECODER), audioDecoder_(decoder) {}
    ~AudioDecoderObject() = default;

    const std::shared_ptr<AVCodecAudioDecoder> audioDecoder_;
    std::list<OHOS::sptr<AVMemory>> memoryObjList_;
    OHOS::sptr<AVFormat> outputFormat_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
};

class NdkAudioDecoderCallback : public AVCodecCallback {
public:
    NdkAudioDecoderCallback(AVCodec *codec, struct AVCodecOnAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NdkAudioDecoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        (void)errorType;
        if (codec_ != nullptr) {
            int32_t extErr = MSErrorToExtError(static_cast<MediaServiceErrCode>(errorCode));
            callback_.onAsyncError(codec_, extErr, userData_);
        }
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        if (codec_ != nullptr) {
            OHOS::sptr<AVFormat> object = new(std::nothrow) AVFormat(format);
            // The object lifecycle is controlled by the current function stack
            callback_.onAsyncStreamChanged(codec_, reinterpret_cast<AVFormat *>(object.GetRefPtr()), userData_);
        }
    }

    void OnInputBufferAvailable(uint32_t index) override
    {
        if (codec_ != nullptr) {
            AVMemory *data = GetInputData(codec_, index);
            if (data != nullptr) {
                callback_.onAsyncNeedInputData(codec_, index, data, userData_);
            }
        }
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override
    {
        if (codec_ != nullptr) {
            struct AVCodecBufferAttr bufferInfo;
            bufferInfo.presentationTimeUs = info.presentationTimeUs;
            bufferInfo.size = info.size;
            bufferInfo.offset = info.offset;
            bufferInfo.flags = flag;
            // The bufferInfo lifecycle is controlled by the current function stack
            AVMemory *data = GetOutputData(codec_, index);
            callback_.onAsyncNewOutputData(codec_, index, data, &bufferInfo, userData_);
        }
    }

private:
    AVMemory* GetInputData(struct AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

        struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, nullptr, "context audioDecoder is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioDecObj->audioDecoder_->GetInputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

        for (auto &memoryObj : audioDecObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        audioDecObj->memoryObjList_.push_back(object);
        MEDIA_LOGD("AVCodec AudioDecoder GetInputBuffer memoryList size:%{public}zu", audioDecObj->memoryObjList_.size());
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }

    AVMemory* GetOutputData(struct AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

        struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, nullptr, "context audioDecoder is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioDecObj->audioDecoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : audioDecObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        audioDecObj->memoryObjList_.push_back(object);
        MEDIA_LOGD("AVCodec AudioDecoder GetOutputBuffer memoryList size:%{public}zu", audioDecObj->memoryObjList_.size());
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }


    struct AVCodec *codec_;
    struct AVCodecOnAsyncCallback callback_;
    void *userData_;
};

struct AVCodec* OH_AVCODEC_CreateAudioDecoderByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecAudioDecoder> audioDecoder = AudioDecoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(audioDecoder != nullptr, nullptr, "failed to AudioDecoderFactory::CreateByMime");

    struct AudioDecoderObject *object = new(std::nothrow) AudioDecoderObject(audioDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioDecoderObject");

    return object;
}

struct AVCodec* OH_AVCODEC_CreateAudioDecoderByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecAudioDecoder> audioDecoder = AudioDecoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(audioDecoder != nullptr, nullptr, "failed to AudioDecoderFactory::CreateByMime");

    struct AudioDecoderObject *object = new(std::nothrow) AudioDecoderObject(audioDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioDecoderObject");

    return object; 
}

AVErrCode OH_AVCODEC_DestroyAudioDecoder(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);

    if (audioDecObj != nullptr && audioDecObj->audioDecoder_ != nullptr) {
        audioDecObj->memoryObjList_.clear();
        int32_t ret = audioDecObj->audioDecoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("audioDecoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("context audioDecoder is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderConfigure(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Configure failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderPrepare(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Prepare failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderStart(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Start failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderStop(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Stop failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderFlush(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Flush failed!");

    audioDecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderReset(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder Reset failed!");

    audioDecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderPushInputData(struct AVCodec *codec, uint32_t index, AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.presentationTimeUs;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = audioDecObj->audioDecoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder QueueInputBuffer failed!");

    return AV_ERR_OK;
}

AVFormat* OH_AVCODEC_AudioDecoderGetOutputMediaDescription(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, nullptr, "context audioDecoder is nullptr!");

    Format format;
    int32_t ret = audioDecObj->audioDecoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "audioDecoder GetOutputFormat failed!");

    audioDecObj->outputFormat_ = new(std::nothrow) AVFormat(format);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->outputFormat_ != nullptr, nullptr, "failed to new AVFormat");

    return reinterpret_cast<AVFormat *>(audioDecObj->outputFormat_.GetRefPtr());
}

AVErrCode OH_AVCODEC_AudioDecoderFreeOutputData(struct AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderSetParameter(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder SetParameter failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioDecoderSetCallback(struct AVCodec *codec, struct AVCodecOnAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioDecoderObject *audioDecObj = reinterpret_cast<AudioDecoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->audioDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    audioDecObj->callback_ = std::make_shared<NdkAudioDecoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(audioDecObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "context audioDecoder is nullptr!");

    int32_t ret = audioDecObj->audioDecoder_->SetCallback(audioDecObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioDecoder SetCallback failed!");

    return AV_ERR_OK;
}