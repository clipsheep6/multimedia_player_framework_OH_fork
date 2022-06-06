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
#include "avcodec_video_decoder.h"
#include "avsharedmemory.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecorderObject"};
}

using namespace OHOS::Media;

struct VideoDecorderObject : public AVCodec {
    explicit VideoDecorderObject(const std::shared_ptr<AVCodecVideoDecoder> &decoder)
        : AVCodec(AVMagic::MEDIA_MAGIC_VIDEO_DECODER), videoDecoder_(decoder) {}
    ~VideoDecorderObject() = default;

    const std::shared_ptr<AVCodecVideoDecoder> videoDecoder_;
    std::list<OHOS::sptr<AVMemory>> memoryObjList_;
    std::shared_ptr<AVCodecCallback> callback_ = nullptr;
};

class NdkAVCodecCallback : public AVCodecCallback {
public:
    NdkAVCodecCallback(AVCodec *codec, struct AVCodecOnAsyncCallback *cb, void *userData)
        : codec_(codec), callback_(cb), userData_(userData) {}
    virtual ~NdkAVCodecCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (callback_ != nullptr && codec_ != nullptr) {
            // 这里要转换错误码native到ndk
            callback_->onAsyncError(codec_, errorCode, userData_);
        }
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        if (callback_ != nullptr && codec_ != nullptr) {
            OHOS::sptr<AVFormat> object = new(std::nothrow) AVFormat(format);
            // The object lifecycle is controlled by the current function stack
            callback_->onAsyncFormatChanged(codec_, reinterpret_cast<AVFormat *>(object.GetRefPtr()), userData_);
        }
    }

    void OnInputBufferAvailable(uint32_t index) override
    {
        if (callback_ != nullptr && codec_ != nullptr) {
            callback_->onAsyncInputAvailable(codec_, index, userData_);
        }
    }

    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override
    {
        if (callback_ != nullptr && codec_ != nullptr) {
            struct AVCodecBufferAttr bufferInfo;
            bufferInfo.presentationTimeUs = info.presentationTimeUs;
            bufferInfo.size = info.size;
            bufferInfo.offset = info.offset;
            bufferInfo.flags = flag;
            // The bufferInfo lifecycle is controlled by the current function stack
            callback_->onAsyncOutputAvailable(codec_, index, &bufferInfo, userData_);
        }
    }

private:
    struct AVCodec *codec_;
    struct AVCodecOnAsyncCallback *callback_;
    void *userData_;
};

struct AVCodec* OH_AVCODEC_CreateVideoDecoderByMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, nullptr, "input mime is nullptr!");

    std::shared_ptr<AVCodecVideoDecoder> videoDecoder = VideoDecoderFactory::CreateByMime(mime);
    CHECK_AND_RETURN_RET_LOG(videoDecoder != nullptr, nullptr, "failed to VideoDecoderFactory::CreateByMime");

    struct VideoDecorderObject *object = new(std::nothrow) VideoDecorderObject(videoDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoDecorderObject");

    return object;
}

struct AVCodec* OH_AVCODEC_CreateVideoDecoderByName(const char *name)
{
    CHECK_AND_RETURN_RET_LOG(name != nullptr, nullptr, "input name is nullptr!");

    std::shared_ptr<AVCodecVideoDecoder> videoDecoder = VideoDecoderFactory::CreateByName(name);
    CHECK_AND_RETURN_RET_LOG(videoDecoder != nullptr, nullptr, "failed to VideoDecoderFactory::CreateByMime");

    struct VideoDecorderObject *object = new(std::nothrow) VideoDecorderObject(videoDecoder);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new VideoDecorderObject");

    return object; 
}

void OH_AVCODEC_DestroyVideoDecoder(struct AVCodec *codec)
{
    CHECK_AND_RETURN_LOG(codec != nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, "magic error!");
    delete codec;
}

AVErrCode OH_AVCODEC_VideoDecoderConfigure(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Configure(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Configure failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderPrepare(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Prepare();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Prepare failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderStart(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Start failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderStop(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Stop failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderFlush(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Flush();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Flush failed!");

    videoDecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderReset(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Reset();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Reset failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderRelease(struct AVCodec *codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder Release failed!");

    videoDecObj->memoryObjList_.clear();
    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderSetOutputSurface(struct AVCodec *codec, struct NativeWindow *window)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_ERR_INVALID_VAL, "input window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_ERR_INVALID_VAL, "input surface is nullptr!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetOutputSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetOutputSurface failed!");

    return AV_ERR_OK;
}

AVMemory* OH_AVCODEC_VideoDecoderGetInputBuffer(struct AVCodec *codec, uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, nullptr, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, nullptr, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, nullptr, "context videoDecoder is nullptr!");

    std::shared_ptr<AVSharedMemory> memory = videoDecObj->videoDecoder_->GetInputBuffer(index);
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, nullptr, "get input buffer is nullptr!");

    for (auto &memoryObj : videoDecObj->memoryObjList_) {
        if (memoryObj->IsEqualMemory(memory)) {
            return reinterpret_cast<AVMemory *>(memoryObj.GetRefPtr());
        }
    }

    OHOS::sptr<AVMemory> object = new(std::nothrow) AVMemory(memory);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new AVMemory");

    videoDecObj->memoryObjList_.push_back(object);
    MEDIA_LOGD("AVCodec VideoDecoder GetInputBuffer memoryList size:%{public}zu", videoDecObj->memoryObjList_.size());
    return reinterpret_cast<AVMemory *>(object.GetRefPtr());
}

AVErrCode OH_AVCODEC_VideoDecoderQueueInputBuffer(struct AVCodec *codec, uint32_t index, AVCodecBufferAttr attr)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    struct AVCodecBufferInfo bufferInfo;
    bufferInfo.presentationTimeUs = attr.presentationTimeUs;
    bufferInfo.size = attr.size;
    bufferInfo.offset = attr.offset;
    enum AVCodecBufferFlag bufferFlag = static_cast<enum AVCodecBufferFlag>(attr.flags);

    int32_t ret = videoDecObj->videoDecoder_->QueueInputBuffer(index, bufferInfo, bufferFlag);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder QueueInputBuffer failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderGetOutputFormat(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->GetOutputFormat(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder GetOutputFormat failed!");

    return AV_ERR_OK;
}

// struct VideoCapsObject* OH_AVCODEC_VideoDecoderGetVideoDecoderCaps(struct AVCodec *codec);
AVErrCode OH_AVCODEC_VideoDecoderReleaseOutputBuffer(struct AVCodec *codec, uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->ReleaseOutputBuffer(index, render);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder ReleaseOutputBuffer failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderSetParameter(struct AVCodec *codec, struct AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == AVMagic::MEDIA_MAGIC_FORMAT, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetParameter(format->format_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetParameter failed!");

    return AV_ERR_OK;
}

AVErrCode OH_AVCODEC_VideoDecoderSetCallback(struct AVCodec *codec, struct AVCodecOnAsyncCallback *callback, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "input codec is nullptr!");
    CHECK_AND_RETURN_RET_LOG(codec->magic_ == AVMagic::MEDIA_MAGIC_VIDEO_DECODER, AV_ERR_INVALID_VAL, "magic error!");

    struct VideoDecorderObject *videoDecObj = reinterpret_cast<VideoDecorderObject *>(codec);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->videoDecoder_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    videoDecObj->callback_ = std::make_shared<NdkAVCodecCallback>(codec, callback, userData);
    CHECK_AND_RETURN_RET_LOG(videoDecObj->callback_ != nullptr, AV_ERR_INVALID_VAL, "context videoDecoder is nullptr!");

    int32_t ret = videoDecObj->videoDecoder_->SetCallback(videoDecObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "videoDecoder SetCallback failed!");

    return AV_ERR_OK;
}