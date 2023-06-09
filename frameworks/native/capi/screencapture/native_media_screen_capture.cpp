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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeScreenCapture"};
}

using namespace OHOS::Media;
class NativeScreenCaptureCallback;

class NativeScreenCaptureCallback : public ScreenCaptureCallBack  {
public:
    NativeScreenCaptureCallback(struct OH_ScreenCapture *capture, struct OH_ScreenCaptureCallback callback)
        : capture_(capture), callback_(callback){}
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
            callback_.onAudioBufferAvailable(capture_, isReady, type);
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

    void StopCallback() {
        std::unique_lock<std::mutex> lock(mutex_);
        capture_ = nullptr;
    }

private:
    struct OH_ScreenCapture *capture_;
    struct OH_ScreenCaptureCallback callback_;
    std::mutex mutex_;
};

struct ScreenCaptureObject : public OH_ScreenCapture {
    explicit ScreenCaptureObject(const std::shared_ptr<ScreenCapture> &capture)
        : screenCapture_(capture) {}
    ~ScreenCaptureObject() = default;

    const std::shared_ptr<ScreenCapture> screenCapture_ = nullptr;
    std::shared_ptr<NativeScreenCaptureCallback> callback_ = nullptr;
};

struct OH_ScreenCapture *OH_ScreenCapture_Create() {
    std::shared_ptr<ScreenCapture> screenCapture = ScreenCaptureFactory::CreateScreenCapture();
    CHECK_AND_RETURN_RET_LOG(screenCapture != nullptr, nullptr, "failed to ScreenCaptureFactory::CreateScreenCapture");

    struct ScreenCaptureObject *object = new(std::nothrow) ScreenCaptureObject(screenCapture);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new ScreenCaptureObject");

    return object;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Init(struct OH_ScreenCapture *capture, AVScreenCaptureConfig config) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->Init(config);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "screenCapture init failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StartScreenCapture(struct OH_ScreenCapture *capture, DataType type) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StartScreenCapture(type);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StartScreenCapture failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_StopScreenCapture(struct OH_ScreenCapture *capture) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->StopScreenCapture();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "StopScreenCapture failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_AcquireAudioBuffer(struct OH_ScreenCapture *capture,
    std::shared_ptr<AudioBuffer> &audiobuffer, AudioCapSourceType type) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->AcquireAudioBuffer(audiobuffer, type);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "AcquireAudioBuffer failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_NativeBuffer* OH_ScreenCapture_AcquireVideoBuffer(struct OH_ScreenCapture *capture,
    int32_t &fence, int64_t &timestamp, OHOS::Rect &damage) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, nullptr, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr, nullptr, "screenCapture_ is null");

    OHOS::sptr<OHOS::SurfaceBuffer> sufacebuffer =
        screenCaptureObj->screenCapture_->AcquireVideoBuffer(fence, timestamp, damage);
    CHECK_AND_RETURN_RET_LOG(sufacebuffer != nullptr, nullptr, "AcquireVideoBuffer failed!");

    OH_NativeBuffer* nativebuffer = sufacebuffer->SurfaceBufferToNativeBuffer();
    OH_NativeBuffer_Reference(nativebuffer);
    return nativebuffer;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseVideoBuffer(struct OH_ScreenCapture *capture) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->ReleaseVideoBuffer();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "ReleaseVideoBuffer failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_ReleaseAudioBuffer(struct OH_ScreenCapture *capture,
    AudioCapSourceType type) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->ReleaseAudioBuffer(type);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "ReleaseSurfaceBuffer failed!");

    return SCREEN_CAPTURE_ERR_OK;
}

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetCallback(struct OH_ScreenCapture *capture,
    struct OH_ScreenCaptureCallback callback) {
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

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_Realease(struct OH_ScreenCapture *capture) {
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

OH_SCREEN_CAPTURE_ErrCode OH_ScreenCapture_SetMicrophoneEnable(struct OH_ScreenCapture *capture, bool isMicrophone) {
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->SetMicrophoneEnable(isMicrophone);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT, "setMicrophoneEnable failed!");

    return SCREEN_CAPTURE_ERR_OK;
}