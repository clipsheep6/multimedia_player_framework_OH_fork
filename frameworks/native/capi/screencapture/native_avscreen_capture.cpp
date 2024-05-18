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

#include "native_avscreen_capture.h"

#include <mutex>
#include <queue>
#include <shared_mutex>
#include <set>

#include "buffer/avbuffer.h"
#include "common/native_mfmagic.h"
#include "media_log.h"
#include "media_errors.h"
#include "native_avbuffer.h"
#include "native_player_magic.h"
#include "surface_buffer_impl.h"
#include "native_window.h"

namespace {
constexpr int MAX_WINDOWS_LEN = 1000;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "NativeScreenCapture"};
}

typedef struct NativeWindow OHNativeWindow;

using namespace OHOS::Media;
static std::queue<OH_NativeBuffer*> referencedBuffer_;
class NativeScreenCaptureCallback;

struct ScreenCaptureObject : public OH_AVScreenCapture {
    explicit ScreenCaptureObject(const std::shared_ptr<ScreenCapture> &capture)
        : screenCapture_(capture) {}
    ~ScreenCaptureObject() = default;

    const std::shared_ptr<ScreenCapture> screenCapture_ = nullptr;
    std::shared_ptr<NativeScreenCaptureCallback> callback_ = nullptr;
};

class NativeScreenCaptureStateChangeCallback {
public:
    NativeScreenCaptureStateChangeCallback(OH_AVScreenCapture_OnStateChange callback, void *userData)
        : callback_(callback), userData_(userData) {}
    virtual ~NativeScreenCaptureStateChangeCallback() = default;

    void OnStateChange(struct OH_AVScreenCapture *capture, AVScreenCaptureStateCode infoType)
    {
        CHECK_AND_RETURN(capture != nullptr && callback_ != nullptr);
        callback_(capture, static_cast<OH_AVScreenCaptureStateCode>(infoType), userData_);
    }

private:
    OH_AVScreenCapture_OnStateChange callback_;
    void *userData_;
};

class NativeScreenCaptureErrorCallback {
public:
    NativeScreenCaptureErrorCallback(OH_AVScreenCapture_OnError callback, void *userData)
        : callback_(callback), userData_(userData) {}
    virtual ~NativeScreenCaptureErrorCallback() = default;

    void OnError(struct OH_AVScreenCapture *capture, int32_t errorCode)
    {
        CHECK_AND_RETURN(capture != nullptr && callback_ != nullptr);
        callback_(capture, errorCode, userData_);
    }

private:
    OH_AVScreenCapture_OnError callback_;
    void *userData_;
};

class NativeScreenCaptureDataCallback {
public:
    NativeScreenCaptureDataCallback(OH_AVScreenCapture_OnBufferAvailable callback, void *userData)
        : callback_(callback), userData_(userData) {}
    virtual ~NativeScreenCaptureDataCallback() = default;

    void OnBufferAvailable(struct OH_AVScreenCapture *capture, OH_AVScreenCaptureBufferType bufferType)
    {
        CHECK_AND_RETURN(capture != nullptr && callback_ != nullptr);
        switch (bufferType) {
            case OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO:
                OnProcessVideoBuffer(capture);
                return;
            case OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER: // fall-through
            case OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC:
                OnProcessAudioBuffer(capture, bufferType);
                return;
            default:
                MEDIA_LOGD("OnBufferAvailable() is called, invalid bufferType:%{public}d", bufferType);
                return;
        }
    }

private:
    static OH_AVSCREEN_CAPTURE_ErrCode AcquireAudioBuffer(const std::shared_ptr<ScreenCapture> &screenCapture,
        OHOS::sptr<OH_AVBuffer> &ohAvBuffer, int64_t &timestamp, AudioCaptureSourceType type)
    {
        std::shared_ptr<AudioBuffer> aBuffer;
        int32_t ret = screenCapture->AcquireAudioBuffer(aBuffer, type);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
            "AcquireAudioBuffer failed not permit! ret:%{public}d", ret);
        CHECK_AND_RETURN_RET_LOG(aBuffer != nullptr && aBuffer->buffer != nullptr, AV_SCREEN_CAPTURE_ERR_NO_MEMORY,
            "AcquireAudioBuffer failed aBuffer no memory!");
        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(aBuffer->buffer, aBuffer->length,
            aBuffer->length);
        CHECK_AND_RETURN_RET_LOG(avBuffer != nullptr, AV_SCREEN_CAPTURE_ERR_NO_MEMORY,
            "AcquireAudioBuffer failed avBuffer no memory!");
        aBuffer->buffer = nullptr; // memory control has transfered to AVBuffer
        timestamp = aBuffer->timestamp;
        ohAvBuffer = new(std::nothrow) OH_AVBuffer(avBuffer);

        CHECK_AND_RETURN_RET_LOG(ohAvBuffer != nullptr, AV_SCREEN_CAPTURE_ERR_NO_MEMORY,
            "AcquireAudioBuffer failed ohAvBuffer no memory!");
        return AV_SCREEN_CAPTURE_ERR_OK;
    }

    static OH_AVSCREEN_CAPTURE_ErrCode ReleaseAudioBuffer(const std::shared_ptr<ScreenCapture> &screenCapture,
        AudioCaptureSourceType type)
    {
        int32_t ret = screenCapture->ReleaseAudioBuffer(type);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
            "ReleaseAudioBuffer failed! ret:%{public}d", ret);
        return AV_SCREEN_CAPTURE_ERR_OK;
    }

    OH_AVSCREEN_CAPTURE_ErrCode OnProcessAudioBuffer(struct OH_AVScreenCapture *capture,
        OH_AVScreenCaptureBufferType bufferType)
    {
        CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
        struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
        CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
            AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture is null");

        MEDIA_LOGD("OnProcessAudioBuffer() is called, bufferType %{public}d", bufferType);
        AudioCaptureSourceType audioSourceType;
        if (bufferType == OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER) {
            audioSourceType = AudioCaptureSourceType::ALL_PLAYBACK;
        } else if (bufferType == OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC) {
            audioSourceType = AudioCaptureSourceType::MIC;
        } else {
            return AV_SCREEN_CAPTURE_ERR_INVALID_VAL;
        }
        OHOS::sptr<OH_AVBuffer> ohAvBuffer;
        int64_t timestamp = 0;
        OH_AVSCREEN_CAPTURE_ErrCode errCode =
            AcquireAudioBuffer(screenCaptureObj->screenCapture_, ohAvBuffer, timestamp, audioSourceType);
        if (errCode == AV_SCREEN_CAPTURE_ERR_OK) {
            callback_(capture, reinterpret_cast<OH_AVBuffer *>(ohAvBuffer.GetRefPtr()), bufferType, timestamp,
                userData_);
            free(ohAvBuffer->buffer_->memory_->GetAddr());
        }
        errCode = ReleaseAudioBuffer(screenCaptureObj->screenCapture_, audioSourceType);
        return errCode;
    }
    static OH_AVSCREEN_CAPTURE_ErrCode AcquireVideoBuffer(const std::shared_ptr<ScreenCapture> &screenCapture,
        OHOS::sptr<OH_AVBuffer> &ohAvBuffer, int64_t &timestamp)
    {
        int32_t fence;
        OHOS::Rect damage;
        OHOS::sptr<OHOS::SurfaceBuffer> surfaceBuffer =
            screenCapture->AcquireVideoBuffer(fence, timestamp, damage);
        CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, AV_SCREEN_CAPTURE_ERR_NO_MEMORY,
            "AcquireVideoBuffer failed surfaceBuffer no memory!");
        std::shared_ptr<AVBuffer> avBuffer = AVBuffer::CreateAVBuffer(surfaceBuffer);
        CHECK_AND_RETURN_RET_LOG(avBuffer != nullptr, AV_SCREEN_CAPTURE_ERR_NO_MEMORY,
            "AcquireVideoBuffer failed avBuffer no memory!");

        ohAvBuffer = new(std::nothrow) OH_AVBuffer(avBuffer);
        CHECK_AND_RETURN_RET_LOG(ohAvBuffer != nullptr, AV_SCREEN_CAPTURE_ERR_NO_MEMORY,
            "AcquireVideoBuffer failed ohAvBuffer no memory!");
        return AV_SCREEN_CAPTURE_ERR_OK;
    }

    static OH_AVSCREEN_CAPTURE_ErrCode ReleaseVideoBuffer(const std::shared_ptr<ScreenCapture> &screenCapture)
    {
        int32_t ret = screenCapture->ReleaseVideoBuffer();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
            "ReleaseVideoBuffer failed! ret:%{public}d", ret);
        return AV_SCREEN_CAPTURE_ERR_OK;
    }

    OH_AVSCREEN_CAPTURE_ErrCode OnProcessVideoBuffer(struct OH_AVScreenCapture *capture)
    {
        CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
        struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
        CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
            AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture is null");

        OHOS::sptr<OH_AVBuffer> hoAvBuffer;
        int64_t timestamp = 0;
        OH_AVSCREEN_CAPTURE_ErrCode errCode =
            AcquireVideoBuffer(screenCaptureObj->screenCapture_, hoAvBuffer, timestamp);
        if (errCode == AV_SCREEN_CAPTURE_ERR_OK) {
            callback_(capture, reinterpret_cast<OH_AVBuffer *>(hoAvBuffer.GetRefPtr()),
                OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO, timestamp, userData_);
        }
        errCode = ReleaseVideoBuffer(screenCaptureObj->screenCapture_);
        return errCode;
    }

private:
    OH_AVScreenCapture_OnBufferAvailable callback_;
    void *userData_;
};

class NativeScreenCaptureCallback : public ScreenCaptureCallBack {
public:
    NativeScreenCaptureCallback(struct OH_AVScreenCapture *capture, struct OH_AVScreenCaptureCallback callback)
        : capture_(capture), callback_(callback) {}
    virtual ~NativeScreenCaptureCallback() = default;

    void OnError(ScreenCaptureErrorType errorType, int32_t errorCode) override
    {
        MEDIA_LOGE("OnError() is called, errorType %{public}d, errorCode %{public}d", errorType, errorCode);
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN(capture_ != nullptr);

        if (errorCallback_ != nullptr) {
            errorCallback_->OnError(capture_, errorCode);
            return;
        }

        if (callback_.onError != nullptr) {
            callback_.onError(capture_, errorCode);
            return;
        }
    }

    void OnStateChange(AVScreenCaptureStateCode stateCode) override
    {
        MEDIA_LOGI("OnStateChange() is called, stateCode %{public}d", stateCode);
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN(capture_ != nullptr);

        if (stateChangeCallback_ != nullptr) {
            stateChangeCallback_->OnStateChange(capture_, stateCode);
            return;
        }
    }

    void OnAudioBufferAvailable(bool isReady, AudioCaptureSourceType type) override
    {
        MEDIA_LOGD("OnAudioBufferAvailable() is called, isReady:%{public}d, type:%{public}d", isReady, type);
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN(capture_ != nullptr);

        if (dataCallback_ != nullptr) {
            if (!isReady) {
                return;
            }
            if (type == AudioCaptureSourceType::SOURCE_DEFAULT || type == AudioCaptureSourceType::MIC) {
                dataCallback_->OnBufferAvailable(capture_,
                    OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_MIC);
            } else if (type == AudioCaptureSourceType::ALL_PLAYBACK || type == AudioCaptureSourceType::APP_PLAYBACK) {
                dataCallback_->OnBufferAvailable(capture_,
                    OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_AUDIO_INNER);
            } else {
                MEDIA_LOGD("OnAudioBufferAvailable() is called, invalid audio source type:%{public}d", type);
            }
            return;
        }
        if (callback_.onAudioBufferAvailable != nullptr) {
            callback_.onAudioBufferAvailable(capture_, isReady, static_cast<OH_AudioCaptureSourceType>(type));
            return;
        }
    }

    void OnVideoBufferAvailable(bool isReady) override
    {
        MEDIA_LOGD("OnVideoBufferAvailable() is called, isReady:%{public}d", isReady);
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN(capture_ != nullptr);

        if (dataCallback_ != nullptr) {
            if (!isReady) {
                return;
            }
            dataCallback_->OnBufferAvailable(capture_,
                OH_AVScreenCaptureBufferType::OH_SCREEN_CAPTURE_BUFFERTYPE_VIDEO);
            return;
        }
        if (capture_ != nullptr && callback_.onVideoBufferAvailable != nullptr) {
            callback_.onVideoBufferAvailable(capture_, isReady);
        }
    }

    void StopCallback()
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        capture_ = nullptr;
    }

    void SetCallback(struct OH_AVScreenCaptureCallback callback)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        callback_ = callback;
    }

    bool IsDataCallbackEnabled()
    {
        return dataCallback_ != nullptr;
    }

    bool IsStateChangeCallbackEnabled()
    {
        return stateChangeCallback_ != nullptr;
    }

    bool SetStateChangeCallback(OH_AVScreenCapture_OnStateChange callback, void *userData)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        stateChangeCallback_ = std::make_shared<NativeScreenCaptureStateChangeCallback>(callback, userData);
        return stateChangeCallback_ != nullptr;
    }

    bool SetErrorCallback(OH_AVScreenCapture_OnError callback, void *userData)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        errorCallback_ = std::make_shared<NativeScreenCaptureErrorCallback>(callback, userData);
        return errorCallback_ != nullptr;
    }

    bool SetDataCallback(OH_AVScreenCapture_OnBufferAvailable callback, void *userData)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        dataCallback_ = std::make_shared<NativeScreenCaptureDataCallback>(callback, userData);
        return dataCallback_ != nullptr;
    }

private:
    std::shared_mutex mutex_;
    struct OH_AVScreenCapture *capture_ = nullptr;
    struct OH_AVScreenCaptureCallback callback_;
    std::shared_ptr<NativeScreenCaptureStateChangeCallback> stateChangeCallback_ = nullptr;
    std::shared_ptr<NativeScreenCaptureErrorCallback> errorCallback_ = nullptr;
    std::shared_ptr<NativeScreenCaptureDataCallback> dataCallback_ = nullptr;
};

struct ScreenCaptureContentFilterObject : public OH_AVScreenCapture_ContentFilter {
    ScreenCaptureContentFilterObject() = default;
    ~ScreenCaptureContentFilterObject() = default;

    ScreenCaptureContentFilter screenCaptureContentFilter;
};

struct OH_AVScreenCapture_ContentFilter *OH_AVScreenCapture_CreateContentFilter(void)
{
    struct ScreenCaptureContentFilterObject *object = new(std::nothrow) ScreenCaptureContentFilterObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "failed to new ScreenCaptureContentFilterObject");
    return object;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ReleaseContentFilter(struct OH_AVScreenCapture_ContentFilter *filter)
{
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input filter is nullptr!");
    struct ScreenCaptureContentFilterObject *contentFilterObj =
        reinterpret_cast<ScreenCaptureContentFilterObject *>(filter);
    delete contentFilterObj;
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ContentFilter_AddAudioContent(
    struct OH_AVScreenCapture_ContentFilter *filter, OH_AVScreenCaptureFilterableAudioContent content)
{
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input filter is nullptr!");
    struct ScreenCaptureContentFilterObject *contentFilterObj =
        reinterpret_cast<ScreenCaptureContentFilterObject *>(filter);
    
    CHECK_AND_RETURN_RET_LOG(
        content >= OH_AVScreenCaptureFilterableAudioContent::OH_SCREEN_CAPTURE_NOTIFICATION_AUDIO ||
        content <= OH_AVScreenCaptureFilterableAudioContent::OH_SCREEN_CAPTURE_CURRENT_APP_AUDIO,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input content invalid!");
    contentFilterObj->screenCaptureContentFilter.filteredAudioContents.insert(
        static_cast<AVScreenCaptureFilterableAudioContent>(content));
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ExcludeContent(struct OH_AVScreenCapture *capture,
    struct OH_AVScreenCapture_ContentFilter *filter)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture is null");

    CHECK_AND_RETURN_RET_LOG(filter != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input filter is nullptr!");
    struct ScreenCaptureContentFilterObject *contentFilterObj =
        reinterpret_cast<ScreenCaptureContentFilterObject *>(filter);

    int32_t ret = screenCaptureObj->screenCapture_->ExcludeContent(contentFilterObj->screenCaptureContentFilter);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_UNSUPPORT, "StartScreenCapture failed!");
    return AV_SCREEN_CAPTURE_ERR_OK;
}

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

static int32_t SetPrivacyAuthorityEnabled(struct ScreenCaptureObject *screenCaptureObj)
{
    if (screenCaptureObj->callback_ != nullptr && screenCaptureObj->callback_->IsStateChangeCallbackEnabled()) {
        int32_t ret = screenCaptureObj->screenCapture_->SetPrivacyAuthorityEnabled();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetPrivacyAuthorityEnabled failed!");
    }
    return MSERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_StartScreenCapture(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = SetPrivacyAuthorityEnabled(screenCaptureObj);
    CHECK_AND_RETURN_RET(ret == AV_SCREEN_CAPTURE_ERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT);
    ret = screenCaptureObj->screenCapture_->StartScreenCapture();
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

    int32_t ret = SetPrivacyAuthorityEnabled(screenCaptureObj);
    CHECK_AND_RETURN_RET(ret == AV_SCREEN_CAPTURE_ERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT);
    ret = screenCaptureObj->screenCapture_->StartScreenCaptureWithSurface(window->surface);
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

    int32_t ret = SetPrivacyAuthorityEnabled(screenCaptureObj);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT);
    ret = screenCaptureObj->screenCapture_->StartScreenRecording();
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
    CHECK_AND_RETURN_RET_LOG(audiobuffer != nullptr && (*audiobuffer != nullptr), AV_SCREEN_CAPTURE_ERR_INVALID_VAL,
        "input OH_AudioBuffer **audiobuffer is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    if (screenCaptureObj->callback_ != nullptr && screenCaptureObj->callback_->IsDataCallbackEnabled()) {
        MEDIA_LOGE("AcquireAudioBuffer() not permit for has set DataCallback");
        return AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT;
    }
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

    if (screenCaptureObj->callback_ != nullptr && screenCaptureObj->callback_->IsDataCallbackEnabled()) {
        MEDIA_LOGE("AcquireVideoBuffer() not permit for has set DataCallback");
        return nullptr;
    }
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
    referencedBuffer_.push(nativebuffer);
    MEDIA_LOGD("return and reference the nativebuffer");

    return nativebuffer;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ReleaseVideoBuffer(struct OH_AVScreenCapture *capture)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    if (screenCaptureObj->callback_ != nullptr && screenCaptureObj->callback_->IsDataCallbackEnabled()) {
        MEDIA_LOGE("ReleaseVideoBuffer() not permit for has set DataCallback");
        return AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT;
    }

    if (!referencedBuffer_.empty()) {
        OH_NativeBuffer* nativebuffer = referencedBuffer_.front();
        OH_NativeBuffer_Unreference(nativebuffer);
        referencedBuffer_.pop();
        MEDIA_LOGD("unreference the front nativebuffer");
    }

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

    if (screenCaptureObj->callback_ != nullptr && screenCaptureObj->callback_->IsDataCallbackEnabled()) {
        MEDIA_LOGE("ReleaseAudioBuffer() not permit for has set DataCallback");
        return AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT;
    }
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

    if (screenCaptureObj->callback_ == nullptr) {
        screenCaptureObj->callback_ = std::make_shared<NativeScreenCaptureCallback>(capture, callback);
        CHECK_AND_RETURN_RET_LOG(screenCaptureObj->callback_ != nullptr,
            AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "callback_ is nullptr!");
    } else {
        screenCaptureObj->callback_->SetCallback(callback);
    }

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

static OH_AVSCREEN_CAPTURE_ErrCode AVScreenCaptureSetCallback(struct OH_AVScreenCapture *capture,
    struct ScreenCaptureObject *screenCaptureObj)
{
    MEDIA_LOGD("AVScreenCaptureSetCallback S");
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj != nullptr && screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is nullptr!");
    if (screenCaptureObj->callback_ == nullptr) {
        MEDIA_LOGD("AVScreenCaptureSetCallback new NativeScreenCaptureCallback");
        OH_AVScreenCaptureCallback dummyCallback = {
            .onError = nullptr,
            .onAudioBufferAvailable = nullptr,
            .onVideoBufferAvailable = nullptr
        };
        screenCaptureObj->callback_ = std::make_shared<NativeScreenCaptureCallback>(capture, dummyCallback);
        CHECK_AND_RETURN_RET_LOG(screenCaptureObj->callback_ != nullptr,
            AV_SCREEN_CAPTURE_ERR_NO_MEMORY, "callback_ is nullptr!");

        int32_t ret = screenCaptureObj->screenCapture_->SetScreenCaptureCallback(screenCaptureObj->callback_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
            "SetScreenCaptureCallback failed!");
    }
    MEDIA_LOGD("AVScreenCaptureSetCallback E");
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetStateCallback(struct OH_AVScreenCapture *capture,
    OH_AVScreenCapture_OnStateChange callback, void *userData)
{
    MEDIA_LOGD("OH_AVScreenCapture_SetStateCallback S");
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input stateCallback is nullptr!");
    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    OH_AVSCREEN_CAPTURE_ErrCode errCode = AVScreenCaptureSetCallback(capture, screenCaptureObj);
    CHECK_AND_RETURN_RET_LOG(errCode == AV_SCREEN_CAPTURE_ERR_OK, errCode, "SetStateCallback is null");

    if (screenCaptureObj->callback_ == nullptr ||
        !screenCaptureObj->callback_->SetStateChangeCallback(callback, userData)) {
        MEDIA_LOGE("OH_AVScreenCapture_SetStateCallback error");
        return AV_SCREEN_CAPTURE_ERR_NO_MEMORY;
    }
    MEDIA_LOGD("OH_AVScreenCapture_SetStateCallback E");
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetErrorCallback(struct OH_AVScreenCapture *capture,
    OH_AVScreenCapture_OnError callback, void *userData)
{
    MEDIA_LOGD("OH_AVScreenCapture_SetErrorCallback S");
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input errorCallback is nullptr!");
    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    OH_AVSCREEN_CAPTURE_ErrCode errCode = AVScreenCaptureSetCallback(capture, screenCaptureObj);
    CHECK_AND_RETURN_RET_LOG(errCode == AV_SCREEN_CAPTURE_ERR_OK, errCode, "SetErrorCallback is null");

    if (screenCaptureObj->callback_ == nullptr || !screenCaptureObj->callback_->SetErrorCallback(callback, userData)) {
        MEDIA_LOGE("OH_AVScreenCapture_SetErrorCallback error");
        return AV_SCREEN_CAPTURE_ERR_NO_MEMORY;
    }
    MEDIA_LOGD("OH_AVScreenCapture_SetErrorCallback E");
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetDataCallback(struct OH_AVScreenCapture *capture,
    OH_AVScreenCapture_OnBufferAvailable callback, void *userData)
{
    MEDIA_LOGD("OH_AVScreenCapture_SetDataCallback E");
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input dataCallback is nullptr!");
    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
        AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    OH_AVSCREEN_CAPTURE_ErrCode errCode = AVScreenCaptureSetCallback(capture, screenCaptureObj);
    CHECK_AND_RETURN_RET_LOG(errCode == AV_SCREEN_CAPTURE_ERR_OK, errCode, "SetDataCallback is null");

    if (screenCaptureObj->callback_ == nullptr ||
        !screenCaptureObj->callback_->SetDataCallback(callback, userData)) {
        MEDIA_LOGE("OH_AVScreenCapture_SetDataCallback error");
        return AV_SCREEN_CAPTURE_ERR_NO_MEMORY;
    }
    MEDIA_LOGD("OH_AVScreenCapture_SetDataCallback E");
    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_SetCanvasRotation(struct OH_AVScreenCapture *capture,
    bool canvasRotation)
{
    CHECK_AND_RETURN_RET_LOG(capture != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input capture is nullptr!");

    struct ScreenCaptureObject *screenCaptureObj = reinterpret_cast<ScreenCaptureObject *>(capture);
    CHECK_AND_RETURN_RET_LOG(screenCaptureObj->screenCapture_ != nullptr,
                             AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "screenCapture_ is null");

    int32_t ret = screenCaptureObj->screenCapture_->SetCanvasRotation(canvasRotation);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT,
                             "SetCanvasRotation failed!");

    return AV_SCREEN_CAPTURE_ERR_OK;
}

OH_AVSCREEN_CAPTURE_ErrCode OH_AVScreenCapture_ContentFilter_AddWindowContent(
    struct OH_AVScreenCapture_ContentFilter *filter, int32_t *windowIDs, int32_t windowCount)
{
    CHECK_AND_RETURN_RET_LOG(filter != nullptr, AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input filter is nullptr!");
    struct ScreenCaptureContentFilterObject *contentFilterObj =
            reinterpret_cast<ScreenCaptureContentFilterObject *>(filter);
    CHECK_AND_RETURN_RET_LOG(windowIDs != nullptr && windowCount > 0 && windowCount < MAX_WINDOWS_LEN,
                             AV_SCREEN_CAPTURE_ERR_INVALID_VAL, "input window invalid!");
    std::vector<int32_t> vec;
    for (int32_t i = 0; i < windowCount; i++) {
        vec.push_back(static_cast<int32_t>(*(windowIDs + i)));
    }
    contentFilterObj->screenCaptureContentFilter.windowIDsVec = vec;
    return AV_SCREEN_CAPTURE_ERR_OK;
}