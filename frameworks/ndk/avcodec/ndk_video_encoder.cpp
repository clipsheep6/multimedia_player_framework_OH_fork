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
#include "avcodec_video_encoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoEncoderObject"};
}

using namespace OHOS::Media;

struct VideoEncoderObject : public AVCodec {
    explicit VideoEncoderObject(const std::shared_ptr<AVCodecVideoEncoder> &encoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_ENCODER), videoEncoder_(encoder) {}
    ~VideoEncoderObject() = default;

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
    std::list<OHOS::sptr<AVMemory>> memoryObjList_;
    OHOS::sptr<AVFormat> outputFormat_ = nullptr;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
};

class NdkVideoEncoderCallback : public AVCodecCallback {
public:
    NdkVideoEncoderCallback(AVCodec *codec, struct AVCodecOnAsyncCallback cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NdkVideoEncoderCallback() = default;

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
        callback_.onAsyncNeedInputData(codec_, index, nullptr, userData_);
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
    AVMemory* GetOutputData(struct AVCodec *codec, uint32_t index)
    {
        CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
        CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, nullptr, "magic error!");

        struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
        CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "context videoEncoder is nullptr!");

        std::shared_ptr<AVSharedMemory> memory = videoEncObj->videoEncoder_->GetOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get output buffer is nullptr!");

        for (auto &memoryObj : videoEncObj->memoryObjList_) {
            if (memoryObj->IsEqualMemory(memory)) {
                return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
            }
        }

        OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
        CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

        videoEncObj->memoryObjList_.push_back(object);
        MEDIA_LOGD("AVCodec VideoEncoder GetOutputBuffer memoryList size:%{public}zu", videoEncObj->memoryObjList_.size());
        return reinterpret_cast<AVMemory *>(object.GetRefPtr());
    }

    struct AVCodec *codec_;
    struct AVCodecOnAsyncCallback callback_;
    void *userData_;
};

struct AVCodec* OH_AVCODEC_CreateVideoEncoderByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "failed to VideoEncoderFactory::CreateByMime");

    struct VideoEncoderObject *object = new(std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoEncoderObject");

    return object;
}

struct AVCodec* OH_AVCODEC_CreateVideoEncoderByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecVideoEncoder> videoEncoder = VideoEncoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, nullptr, "failed to VideoEncoderFactory::CreateByMime");

    struct VideoEncoderObject *object = new(std::nothrow) VideoEncoderObject(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoEncoderObject");

    return object; 
}

AVErrCode OH_AVCODEC_DestroyVideoEncoder(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);

    if (videoEncObj != nullptr && videoEncObj->videoEncoder_ != nullptr) {
        videoEncObj->memoryObjList_.clear();
        int32_t ret = videoEncObj->videoEncoder_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("videoEncoder Release failed!");
            delete codec;
            return AV_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("context videoEncoder is nullptr!");
    }

    delete codec;
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderConfigure(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Configure failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderPrepare(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Prepare failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderStart(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Start failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderStop(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Stop failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderFlush(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Flush failed!");

    videoEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderReset(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder Reset failed!");

    videoEncObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderGetInputSurface(struct AVCodec *codec, struct NativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_ERR_INVALID_VAL, "input window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_INVALID_VAL, "input surface is nullptr!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    window->surface = videoEncObj->videoEncoder_->CreateInputSurface();
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder CreateInputSurface failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderPushInputData(struct AVCodec *codec, uint32_t index, AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.presentationTimeUs;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoEncObj->videoEncoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder QueueInputBuffer failed!");

    return AV_ERR_OK;
}

AVFormat* OH_AVCODEC_VideoEncoderGetOutputMediaDescription(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, nullptr, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, nullptr, "context videoEncoder is nullptr!");

    Format format;
    int32_t ret = videoEncObj->videoEncoder_->GetOutputFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "videoEncoder GetOutputFormat failed!");

    videoEncObj->outputFormat_ = new(std::nothrow) AVFormat(format);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->outputFormat_ != nullptr, nullptr, "failed to new AVFormat");

    return reinterpret_cast<AVFormat *>(videoEncObj->outputFormat_.GetRefPtr());
}

AVErrCode OH_AVCODEC_VideoEncoderFreeOutputData(struct AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->ReleaseOutputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderSetParameter(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder SetParameter failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoEncoderSetCallback(struct AVCodec *codec, struct AVCodecOnAsyncCallback callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_ENCODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoEncoderObject *videoEncObj = reinterpret_cast<VideoEncoderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->videoEncoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    videoEncObj->callback_ = std::make_shared<NdkVideoEncoderCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(videoEncObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "context videoEncoder is nullptr!");

    int32_t ret = videoEncObj->videoEncoder_->SetCallback(videoEncObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoEncoder SetCallback failed!");

    return AV_ERR_OK;
}