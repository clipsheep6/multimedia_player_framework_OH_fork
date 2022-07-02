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
#include "avcodec_audio_encoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioEncoderObject"};
}

using namespace OHOS::Media;

struct AudioEncoderObject : public AVCodec {
    explicit AudioEncoderObject(const std::shared_ptr<AVCodecAudioEncoder> &decoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_AUDIO_DECODER), audioEncoder_(decoder) {}
    ~AudioEncoderObject() = default;

    const std::shared_ptr<AVCodecAudioEncoder> audioEncoder_;
    std::list<OHOS::sptr<AVMemory>> memoryObjList_;
    OHOS::sptr<AVFormat> outputFormat_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
};

class NdkAudioEncoderCallback : public AVCodecCallback {
public:
    NdkAudioEncoderCallback(AVCodec *codec, struct AVCodecOnAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NdkAudioEncoderCallback() = default;

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

        struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "context audioEncoder is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioEncObj->audioEncoder_->GetInputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

        for (auto &memoryObj : audioEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        audioEncObj->memoryObjList_.push_back(object);
        MEDIA_LOGD("AVCodec AudioEncoder GetInputBuffer memoryList size:%{public}zu", audioEncObj->memoryObjList_.size());
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }

    AVMemory* GetOutputData(struct AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

        struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "context audioEncoder is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = audioEncObj->audioEncoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : audioEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        audioEncObj->memoryObjList_.push_back(object);
        MEDIA_LOGD("AVCodec AudioEncoder GetOutputBuffer memoryList size:%{public}zu", audioEncObj->memoryObjList_.size());
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }

    struct AVCodec *codec_;
    struct AVCodecOnAsyncCallback callback_;
    void *userData_;
};

struct AVCodec* OH_AVCODEC_CreateAudioEncoderByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecAudioEncoder> audioEncoder = AudioEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(audioEncoder != nullptr, nullptr, "failed to AudioEncoderFactory::CreateByMime");

    struct AudioEncoderObject *object = new(std::nothrow) AudioEncoderObject(audioEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioEncoderObject");

    return object;
}

struct AVCodec* OH_AVCODEC_CreateAudioEncoderByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecAudioEncoder> audioEncoder = AudioEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(audioEncoder != nullptr, nullptr, "failed to AudioEncoderFactory::CreateByMime");

    struct AudioEncoderObject *object = new(std::nothrow) AudioEncoderObject(audioEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AudioEncoderObject");

    return object; 
}

AVErrCode OH_AVCODEC_DestroyAudioEncoder(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);

    if (audioEncObj != nullptr && audioEncObj->audioEncoder_ != nullptr) {
        audioEncObj->memoryObjList_.clear();
        int32_t ret = audioEncObj->audioEncoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("audioEncoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("context audioEncoder is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderConfigure(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Configure failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderPrepare(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Prepare failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderStart(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Start failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderStop(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Stop failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderFlush(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Flush failed!");

    audioEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderReset(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder Reset failed!");

    audioEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderPushInputData(struct AVCodec *codec, uint32_t index, AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.presentationTimeUs;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = audioEncObj->audioEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder QueueInputBuffer failed!");

    return AV_ERR_OK;
}

AVFormat* OH_AVCODEC_AudioEncoderGetOutputMediaDescription(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, nullptr, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, nullptr, "context audioEncoder is nullptr!");

    Format format;
    int32_t ret = audioEncObj->audioEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "audioEncoder GetOutputFormat failed!");

    audioEncObj->outputFormat_ = new(std::nothrow) AVFormat(format);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->outputFormat_ != nullptr, nullptr, "failed to new AVFormat");

    return reinterpret_cast<AVFormat *>(audioEncObj->outputFormat_.GetRefPtr());
}

AVErrCode OH_AVCODEC_AudioEncoderFreeOutputData(struct AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderSetParameter(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder SetParameter failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_AudioEncoderSetCallback(struct AVCodec *codec, struct AVCodecOnAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_AUDIO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct AudioEncoderObject *audioEncObj = reinterpret_cast<AudioEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->audioEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    audioEncObj->callback_ = std::make_shared<NdkAudioEncoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(audioEncObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "context audioEncoder is nullptr!");

    int32_t ret = audioEncObj->audioEncoder_->SetCallback(audioEncObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "audioEncoder SetCallback failed!");

    return AV_ERR_OK;
}