/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <mutex>
#include "media_log.h"
#include "media_errors.h"
#include "native_media_screen_capture.h"
#include "native_screen_capture_magic.h"
#include "surface_buffer_impl.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeScreenCapture"};
}

using namespace OHOS::Media;
class NativeScreenCaptureCallback;

struct ScreenCaptureObject : public OH_ScreenCapture {
    explicit ScreenCaptureObject(const std::shared_ptr<ScreenCapture> &capture)
        : screenCapture_(capture) {}
    ~ScreenCaptureObject() = default;

    const std::shared_ptr<ScreenCapture> screenCapture_ = nullptr;
    std::shared_ptr<NativeScreenCaptureCallback> callback_ = nullptr;
};

class NativeScreenCaptureCallback : public ScreenCaptureCallBack {
public:
    NativeScreenCaptureCallback(struct OH_ScreenCapture *capture, struct OH_ScreenCaptureCallback callback)
        : capture_(capture), callback_(callback) {}
    virtual ~NativeScreenCaptureCallback() = default;

    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override
    {
        MEDIA_LOGI("OnError() is called, errorType %{public}d, errorCode %{public}d", errorType, errorCode);
        std::unique_lock<std::mutex> lock(mutex_);

        if (capture_ != nullptr && callback_.onError != nullptr) {
            callback_.onError(capture_, errorCode);
        }
    }

    void OnAudioBufferAvailable(bool isReady, AudioCapSourceType type) override
    {
        MEDIA_LOGI("OnAudioBufferAvailable() is called, isReady:%{public}d", isReady);
        std::unique_lock<std::mutex> lock(mutex_);
        if (capture_ != nullptr && callback_.onAudioBufferAvailable != nullptr) {
            callback_.onAudioBufferAvailable(capture_, isReady, static_cast<OH_AudioCapSourceType>(type));
        }
    }

    void OnVideoBufferAvailable(bool isReady) override
    {
        MEDIA_LOGI("OnVideoBufferAvailable() is called, isReady:%{public}d", isReady);
        std::unique_lock<std::mutex> lock(mutex_);
        if (capture_ != nullptr && callback_.onVideoBufferAvailable != nullptr) {
            callback_.onVideoBufferAvailable(capture_, isReady);
        }
    }

    void StopCallback()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        capture_ = nullptr;
    }

private:
    struct OH_ScreenCapture *capture_;
    struct OH_ScreenCaptureCallback callback_;
    std::mutex mutex_;
};

struct OH_ScreenCapture *OH_ScreenCapture_Create(void)
{
    std::shared_ptr<ScreenCapture> screenCapture = ScreenCaptureFactory::CreateScreenCapture();
    CHECK_AND_RETURN_RET_LOG(screenCapture != nullptr, nullptr, "failed to ScreenCaptureFactory::CreateScreenCapture");

    struct ScreenCaptureObject *object = new(std::nothrow) ScreenCaptureObject(screenCapture);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new ScreenCaptureObject");

    return object;
}

AVScreenCaptureConfig OH_ScreenCapture_Convert(OH_AVScreenCaptureConfig config)
{
    AVScreenCaptureConfig config_;
    config_.captureMode = config.captureMode;
    config_.dataType = static_cast<DataType>(config.dataType);
    config_.audioInfo.micCapInfo = {
        .audioSampleRate = config.audioInfo.micCapInfo.audioSampleRate,
        .audioChannels = config.audioInfo.micCapInfo.audioChannels,
        .audioSource = static_cast<AudioCapSourceType>(config.audioInfo.micCapInfo.audioSource)
    };
    config_.audioInfo.innerCapInfo = {
        .audioSampleRate = config.audioInfo.innerCapInfo.audioSampleRate,
        .audioChannels = config.audioInfo.innerCapInfo.audioChannels,
        .audioSource = static_cast<AudioCapSourceType>(config.audioInfo.innerCapInfo.audioSource)
    };
    config_.audioInfo.audioEncInfo.audioBitrate = config.audioInfo.audioEncInfo.audioBitrate;
    config_.audioInfo.audioEncInfo.audioCodecformat =
        static_cast<AudioCodecFormat>(config.audioInfo.audioEncInfo.audioCodecformat);
    config_.videoInfo.videoCapInfo.displayId = config.videoInfo.videoCapInfo.displayId;
    int32_t *taskIds = config.videoInfo.videoCapInfo.taskIDs;
    int size = config.videoInfo.videoCapInfo.taskIDsLen;
    while (size) {
        config_.videoInfo.videoCapInfo.taskIDs.push_back(*(taskIds++));
        size--;
    }
    size = 0;
    config_.videoInfo.videoCapInfo.videoFrameWidth = config.videoInfo.videoCapInfo.videoFrameWidth;
    config_.videoInfo.videoCapInfo.videoFrameHeight = config.videoInfo.videoCapInfo.videoFrameHeight;
    config_.videoInfo.videoCapInfo.videoSource =
        static_cast<VideoSourceType>(config.videoInfo.videoCapInfo.videoSource);
    config_.videoInfo.videoEncInfo = {
        .videoCodec = static_cast<VideoCodecFormat>(config_.videoInfo.videoEncInfo.videoCodec),
        .videoBitrate = static_cast<VideoCodecFormat>(config_.videoInfo.videoEncInfo. videoBitrate),
        .videoFrameRate = static_cast<VideoCodecFormat>(config_.videoInfo.videoEncInfo.videoFrameRate)
    };
    size = config.recorderInfo.urlLen;
    char *url = config.recorderInfo.url;
    while (size) {
        config_.recorderInfo.url.push_back(*(url++));
        size--;
    }
    if (config.recorderInfo.fileFormat == CFT_MPEG_4A) {
        config_.recorderInfo.fileFormat = ContainerFormatType::CFT_MPEG_4A;
    } else if (config.recorderInfo.fileFormat == CFT_MPEG_4){
        config_.recorderInfo.fileFormat = ContainerFormatType::CFT_MPEG_4;
    }
    return config_;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Init(struct OH_ScreenCapture *capture, OH_AVScreenCaptureConfig config)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    AVScreenCaptureConfig config_ = OH_ScreenCapture_Convert(config);
    int32_t ret = screenCaptureObj->screenCapture_->Init(config_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "screenCapture init failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StartScreenCapture(struct OH_ScreenCapture *capture, OH_DataType type)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StartScreenCapture(static_cast<DataType>(type));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StartScreenCapture failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StopScreenCapture(struct OH_ScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StopScreenCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StopScreenCapture failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_AcquireAudioBuffer(struct OH_ScreenCapture *capture,
    OH_AudioBuffer **audiobuffer, OH_AudioCapSourceType type)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    std::shared_ptr<AudioBuffer> aBuffer;
    int32_t ret = screenCaptureObj->screenCapture_->AcquireAudioBuffer(aBuffer, static_cast<AudioCapSourceType>(type));
    (*audiobuffer)->buf = std::move(aBuffer->buffer);
    (*audiobuffer)->size = aBuffer->length;
    (*audiobuffer)->timestamp = aBuffer->timestamp;
    (*audiobuffer)->type = static_cast<OH_AudioCapSourceType>(aBuffer->sourcetype);

    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "AcquireAudioBuffer failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_NativeBuffer* OH_ScreenCapture_AcquireVideoBuffer(struct OH_ScreenCapture *capture,
    int32_t *fence, int64_t *timestamp, struct OH_Rect *region)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, nullptr, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr, nullptr, "screenCapture_ is null");

    OHOS::Rect damage;
    OHOS::sptr<OHOS::SurfaceBuffer> sufacebuffer =
    screenCaptureObj->screenCapture_->AcquireVideoBuffer(*fence, *timestamp, damage);
    region->x = damage.x;
    region->y = damage.y;
    region->width = damage.w;
    region->height = damage.h;
    CHECK_AND_RETURN_RET_LOG(sufacebuffer != nullptr, nullptr, "AcquireVideoBuffer failed!");

    OH_NativeBuffer* nativebuffer = sufacebuffer->SurfaceBufferToNativeBuffer();
    OH_NativeBuffer_Reference(nativebuffer);
    return nativebuffer;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseVideoBuffer(struct OH_ScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->ReleaseVideoBuffer();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "ReleaseVideoBuffer failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseAudioBuffer(struct OH_ScreenCapture *capture,
    OH_AudioCapSourceType type)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->ReleaseAudioBuffer(static_cast<AudioCapSourceType>(type));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "ReleaseSurfaceBuffer failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetCallback(struct OH_ScreenCapture *capture,
    struct OH_ScreenCaptureCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    screenCaptureObj->callback_ = std::make_shared<NativeScreenCaptureCallback>(capture, callback);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->callback_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = screenCaptureObj->screenCapture_->SetScreenCaptureCallback(screenCaptureObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
        "SetScreenCaptureCallback failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Realease(struct OH_ScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);

    if (screenCaptureObj != nullptr && screenCaptureObj->screenCapture_ != nullptr) {
        if (screenCaptureObj->callback_ != nullptr) {
            screenCaptureObj->callback_->StopCallback();
        }
        int32_t ret = screenCaptureObj->screenCapture_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("screen capture Release failed!");
            capture = nullptr;
            return SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("screen capture is nullptr!");
    }

    delete capture;
    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetMicrophoneEnable(struct OH_ScreenCapture *capture, bool isMicrophone)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->SetMicrophoneEnable(isMicrophone);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "setMicrophoneEnable failed!");

    return SCREEN_CAPTURE_ERR_OK;
}