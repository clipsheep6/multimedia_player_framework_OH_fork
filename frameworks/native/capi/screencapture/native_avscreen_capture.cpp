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
#include "native_player_magic.h"
#include "surface_buffer_impl.h"
#include "native_avscreen_capture.h"
#include "native_window.h"

namespace {
constexpr int MAX_WINDOWS_LEN = 1000;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeScreenCapture"};
}

typedef struct NativeWindow OHNativeWindow;

using namespace OHOS::Media;
class NativeScreenCaptureCallback;

struct ScreenCaptureObject : public OH_AVScreenCapture {
    explicit ScreenCaptureObject(const std::shared_ptr<ScreenCapture> &capture)
        : screenCapture_(capture) {}
    ~ScreenCaptureObject() = default;

    const std::shared_ptr<ScreenCapture> screenCapture_ = nullptr;
    std::shared_ptr<NativeScreenCaptureCallback> callback_ = nullptr;
};

class NativeScreenCaptureCallback : public ScreenCaptureCallBack {
public:
    NativeScreenCaptureCallback(struct OH_AVScreenCapture *capture, struct OH_AVScreenCaptureCallback callback)
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

    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override
    {
        MEDIA_LOGD("OnAudioBufferAvailable() is called, isReady:%{public}d", isReady);
        std::unique_lock<std::mutex> lock(mutex_);
        if (capture_ != nullptr && callback_.onAudioBufferAvailable != nullptr) {
            callback_.onAudioBufferAvailable(capture_, isReady, static_cast<OH_AudioCaptureSourceType>(type));
        }
    }

    void OnVideoBufferAvailable(bool isReady) override
    {
        MEDIA_LOGD("OnVideoBufferAvailable() is called, isReady:%{public}d", isReady);
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
    struct OH_AVScreenCapture *capture_;
    struct OH_AVScreenCaptureCallback callback_;
    std::mutex mutex_;
};

struct OH_AVScreenCapture *OH_AVScreenCapture_Create(void)
{
    std::shared_ptr<ScreenCapture> screenCapture = ScreenCaptureFactory::CreateScreenCapture();
    CHECK_AND_RETURN_RET_LOG(screenCapture != nullptr, nullptr, "failed to ScreenCaptureFactory::CreateScreenCapture");

    struct ScreenCaptureObject *object = new(std::nothrow) ScreenCaptureObject(screenCapture);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new ScreenCaptureObject");

    return object;
}

AVScreenCaptureConfig OH_AVScreenCapture_Convert(OH_AVScreenCaptureConfig config)
{
    AVScreenCaptureConfig config_;
    config_.captureMode = static_cast<CaptureMode>(config.captureMode);
    config_.dataType = static_cast<DataType>(config.dataType);
    config_.audioInfo.micCapInfo = {
        .audioSampleRate = config.audioInfo.micCapInfo.audioSampleRate,
        .audioChannels = config.audioInfo.micCapInfo.audioChannels,
        .audioSource = static_cast<AudioCaptureSourceType>(config.audioInfo.micCapInfo.audioSource)
    };
    config_.audioInfo.innerCapInfo = {
        .audioSampleRate = config.audioInfo.innerCapInfo.audioSampleRate,
        .audioChannels = config.audioInfo.innerCapInfo.audioChannels,
        .audioSource = static_cast<AudioCaptureSourceType>(config.audioInfo.innerCapInfo.audioSource)
    };
    config_.audioInfo.audioEncInfo.audioBitrate = config.audioInfo.audioEncInfo.audioBitrate;
    config_.audioInfo.audioEncInfo.audioCodecformat =
        static_cast<AudioCodecFormat>(config.audioInfo.audioEncInfo.audioCodecformat);
    config_.videoInfo.videoCapInfo.displayId = config.videoInfo.videoCapInfo.displayId;
    int32_t *taskIds = config.videoInfo.videoCapInfo.missionIDs;
    int32_t size = config.videoInfo.videoCapInfo.missionIDsLen;
    size = size >= MAX_WINDOWS_LEN ? MAX_WINDOWS_LEN : size;
    while (size > 0) {
        if (taskIds == nullptr) {
            break;
        }
        config_.videoInfo.videoCapInfo.taskIDs.push_back(*(taskIds));
        taskIds++;
        size--;
    }
    config_.videoInfo.videoCapInfo.videoFrameWidth = config.videoInfo.videoCapInfo.videoFrameWidth;
    config_.videoInfo.videoCapInfo.videoFrameHeight = config.videoInfo.videoCapInfo.videoFrameHeight;
    config_.videoInfo.videoCapInfo.videoSource =
        static_cast<VideoSourceType>(config.videoInfo.videoCapInfo.videoSource);
    config_.videoInfo.videoEncInfo = {
        .videoCodec = static_cast<VideoCodecFormat>(config.videoInfo.videoEncInfo.videoCodec),
        .videoBitrate = config.videoInfo.videoEncInfo. videoBitrate,
        .videoFrameRate = config.videoInfo.videoEncInfo.videoFrameRate
    };
    if (config.recorderInfo.url != nullptr) {
        config_.recorderInfo.url = config.recorderInfo.url;
    }
    if (config.recorderInfo.fileFormat == CFT_MPEG_4A) {
        config_.recorderInfo.fileFormat = ContainerFormatType::CFT_MPEG_4A;
    } else if (config.recorderInfo.fileFormat == CFT_MPEG_4) {
        config_.recorderInfo.fileFormat = ContainerFormatType::CFT_MPEG_4;
    }
    return config_;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_Init(struct OH_AVScreenCapture *capture, OH_AVScreenCaptureConfig config)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    AVScreenCaptureConfig config_ = OH_AVScreenCapture_Convert(config);
    int32_t ret = screenCaptureObj->screenCapture_->Init(config_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "screenCapture init failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_StartScreenCapture(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StartScreenCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StartScreenCapture failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_StartScreenCaptureWithSurface(struct OH_AVScreenCapture *capture,
    OHNativeWindow* window)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input window is nullptr!");
    CHECK_AND_RETURN_RET_LOG(window->surface != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL,
        "Input window surface is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StartScreenCaptureWithSurface(window->surface);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StartScreenCapture failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_StopScreenCapture(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StopScreenCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StopScreenCapture failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_StartScreenRecording(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StartScreenRecording();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StartScreenRecording failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_StopScreenRecording(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StopScreenRecording();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StopScreenRecording failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_AcquireAudioBuffer(struct OH_AVScreenCapture *capture,
    OH_AudioBuffer **audiobuffer, OH_AudioCaptureSourceType type)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    std::shared_ptr<AudioBuffer> aBuffer;
    int32_t ret =
        screenCaptureObj->screenCapture_->AcquireAudioBuffer(aBuffer, static_cast<AudioCaptureSourceType>(type));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "AcquireAudioBuffer failed!");
    if ((aBuffer == nullptr) || (audiobuffer == nullptr)) {
        return AV_SCREEN_CAPTURE_ERR_NO_MEMORY;
    }
    if (aBuffer->buffer != nullptr) {
        (*audiobuffer)->buf = std::move(aBuffer->buffer);
        aBuffer->buffer = nullptr;
    }
    (*audiobuffer)->size = aBuffer->length;
    (*audiobuffer)->timestamp = aBuffer->timestamp;
    (*audiobuffer)->type = static_cast<OH_AudioCaptureSourceType>(aBuffer->sourcetype);
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_NativeBuffer* OH_AVScreenCapture_AcquireVideoBuffer(struct OH_AVScreenCapture *capture,
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

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ReleaseVideoBuffer(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->ReleaseVideoBuffer();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "ReleaseVideoBuffer failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ReleaseAudioBuffer(struct OH_AVScreenCapture *capture,
    OH_AudioCaptureSourceType type)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->ReleaseAudioBuffer(static_cast<AudioCaptureSourceType>(type));
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "ReleaseSurfaceBuffer failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetCallback(struct OH_AVScreenCapture *capture,
    struct OH_AVScreenCaptureCallback callback)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    screenCaptureObj->callback_ = std::make_shared<NativeScreenCaptureCallback>(capture, callback);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->callback_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "videoEncoder_ is nullptr!");

    int32_t ret = screenCaptureObj->screenCapture_->SetScreenCaptureCallback(screenCaptureObj->callback_);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
        "SetScreenCaptureCallback failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_Release(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);

    if (screenCaptureObj != nullptr && screenCaptureObj->screenCapture_ != nullptr) {
        if (screenCaptureObj->callback_ != nullptr) {
            screenCaptureObj->callback_->StopCallback();
        }
        int32_t ret = screenCaptureObj->screenCapture_->Release();
        if (ret != MSERR_OK) {
            MEDIA_LOGE("screen capture Release failed!");
            capture = nullptr;
            return AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT;
        }
    } else {
        MEDIA_LOGD("screen capture is nullptr!");
    }

    delete capture;
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetMicrophoneEnabled(struct OH_AVScreenCapture *capture,
    bool isMicrophone)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->SetMicrophoneEnabled(isMicrophone);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "setMicrophoneEnable failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetScreenCanvasRotation(struct OH_AVScreenCapture *capture,
    bool canvasRotation)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
                             AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->SetScreenCanvasRotation(canvasRotation);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
                             "SetScreenCanvasRotation failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}