/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "screen_capture_capi_mock.h"
#include "native_mfmagic.h"
#include "native_window.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::ScreenCaptureTestParam;
std::mutex ScreenCaptureCapiMock::mutex_;
std::map<OH_AVScreenCapture *, std::shared_ptr<ScreenCaptureCallBackMock>> ScreenCaptureCapiMock::mockCbMap_;

typedef struct NativeWindow OHNativeWindow;

void ScreenCaptureCapiMock::OnError(OH_AVScreenCapture *screenCapture, int32_t errorCode)
{
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb = GetCallback(screenCapture);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode);
    }
}

void ScreenCaptureCapiMock::OnAudioBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady,
    OH_AudioCaptureSourceType type)
{
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb = GetCallback(screenCapture);
    if (mockCb != nullptr) {
        mockCb->OnAudioBufferAvailable(isReady, static_cast<AudioCaptureSourceType>(type));
    }
}

void ScreenCaptureCapiMock::OnVideoBufferAvailable(OH_AVScreenCapture *screenCapture, bool isReady)
{
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb = GetCallback(screenCapture);
    if (mockCb != nullptr) {
        mockCb->OnVideoBufferAvailable(isReady);
    }
}

void ScreenCaptureCapiMock::OnErrorNew(OH_AVScreenCapture *screenCapture, int32_t errorCode, void *userData)
{
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb = GetCallback(screenCapture);
    if (mockCb != nullptr) {
        mockCb->OnError(errorCode, userData);
    }
}

void ScreenCaptureCapiMock::OnBufferAvailable(OH_AVScreenCapture *screenCapture, OH_AVBuffer *buffer,
    OH_AVScreenCaptureBufferType bufferType, int64_t timestamp, void *userData)
{
    UNITTEST_CHECK_AND_RETURN_LOG(buffer != nullptr, "OH_AVBuffer buffer == nullptr");
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb = GetCallback(screenCapture);
    if (mockCb != nullptr) {
        mockCb->OnBufferAvailable(buffer->buffer_, static_cast<AVScreenCaptureBufferType>(bufferType), timestamp);
    }
}

void ScreenCaptureCapiMock::OnStateChange(OH_AVScreenCapture *screenCapture,
    OH_AVScreenCaptureStateCode stateCode, void *userData)
{
    std::shared_ptr<ScreenCaptureCallBackMock> mockCb = GetCallback(screenCapture);
    if (mockCb != nullptr) {
        mockCb->OnStateChange(static_cast<AVScreenCaptureStateCode>(stateCode));
    }
}

OH_AVScreenCaptureConfig ScreenCaptureCapiMock::Convert(AVScreenCaptureConfig config)
{
    OH_AVScreenCaptureConfig tempConfig;
    tempConfig.captureMode = static_cast<OH_CaptureMode>(config.captureMode);
    tempConfig.dataType = static_cast<OH_DataType>(config.dataType);
    tempConfig.audioInfo.micCapInfo = {
        .audioSampleRate = config.audioInfo.micCapInfo.audioSampleRate,
        .audioChannels = config.audioInfo.micCapInfo.audioChannels,
        .audioSource = static_cast<OH_AudioCaptureSourceType>(config.audioInfo.micCapInfo.audioSource)
    };
    tempConfig.audioInfo.innerCapInfo = {
        .audioSampleRate = config.audioInfo.innerCapInfo.audioSampleRate,
        .audioChannels = config.audioInfo.innerCapInfo.audioChannels,
        .audioSource = static_cast<OH_AudioCaptureSourceType>(config.audioInfo.innerCapInfo.audioSource)
    };
    tempConfig.audioInfo.audioEncInfo.audioBitrate = config.audioInfo.audioEncInfo.audioBitrate;
    tempConfig.audioInfo.audioEncInfo.audioCodecformat =
        static_cast<OH_AudioCodecFormat>(config.audioInfo.audioEncInfo.audioCodecformat);
    tempConfig.videoInfo.videoCapInfo.displayId = config.videoInfo.videoCapInfo.displayId;
    std::list<int32_t> taskIds = config.videoInfo.videoCapInfo.taskIDs;
    if (taskIds.size() > 0) {
        int32_t *taskIds_temp = static_cast<int*>(malloc(sizeof(int)));
        for (std::list<int32_t>::iterator its = taskIds.begin(); its != taskIds.end(); ++its) {
            *taskIds_temp = *its;
            taskIds_temp++;
        }
        tempConfig.videoInfo.videoCapInfo.missionIDs = taskIds_temp;
    }
    tempConfig.videoInfo.videoCapInfo.missionIDsLen = taskIds.size();
    tempConfig.videoInfo.videoCapInfo.videoFrameWidth = config.videoInfo.videoCapInfo.videoFrameWidth;
    tempConfig.videoInfo.videoCapInfo.videoFrameHeight = config.videoInfo.videoCapInfo.videoFrameHeight;
    tempConfig.videoInfo.videoCapInfo.videoSource =
        static_cast<OH_VideoSourceType>(config.videoInfo.videoCapInfo.videoSource);
    tempConfig.videoInfo.videoEncInfo = {
        .videoCodec = static_cast<OH_VideoCodecFormat>(config.videoInfo.videoEncInfo.videoCodec),
        .videoBitrate = static_cast<OH_VideoCodecFormat>(config.videoInfo.videoEncInfo.videoBitrate),
        .videoFrameRate = static_cast<OH_VideoCodecFormat>(config.videoInfo.videoEncInfo.videoFrameRate)
    };
    std::string url = config.recorderInfo.url;
    if (!(url.empty())) {
        tempConfig.recorderInfo.url = const_cast<char *>(config.recorderInfo.url.c_str());
        tempConfig.recorderInfo.urlLen = config.recorderInfo.url.size();
    }
    if (config.recorderInfo.fileFormat == ContainerFormatType::CFT_MPEG_4A) {
        tempConfig.recorderInfo.fileFormat = OH_ContainerFormatType::CFT_MPEG_4A;
    } else if (config.recorderInfo.fileFormat == ContainerFormatType::CFT_MPEG_4) {
        tempConfig.recorderInfo.fileFormat = OH_ContainerFormatType::CFT_MPEG_4;
    }
    return tempConfig;
}

std::shared_ptr<ScreenCaptureCallBackMock> ScreenCaptureCapiMock::GetCallback(OH_AVScreenCapture *screenCapture)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.empty()) {
        return nullptr;
    }
    if (mockCbMap_.find(screenCapture) != mockCbMap_.end()) {
        return mockCbMap_.at(screenCapture);
    }
    return nullptr;
}

void ScreenCaptureCapiMock::DelCallback(OH_AVScreenCapture *screenCapture)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (mockCbMap_.empty()) {
        return;
    }
    auto it = mockCbMap_.find(screenCapture);
    if (it != mockCbMap_.end()) {
        mockCbMap_.erase(it);
    }
}

void ScreenCaptureCapiMock::SetScreenCaptureCallback(OH_AVScreenCapture *screencapture,
    std::shared_ptr<ScreenCaptureCallBackMock> cb)
{
    std::lock_guard<std::mutex> lock(mutex_);
    mockCbMap_[screencapture] = cb;
}

int32_t ScreenCaptureCapiMock::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBackMock>& callback,
    const bool isErrorCallBackEnabled, const bool isDataCallBackEnabled, const bool isStateChangeCallBackEnabled)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    if (callback == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("ScreenCaptureCapiMock SetScreenCaptureCallback");
    SetScreenCaptureCallback(screenCapture_, callback);
    struct OH_AVScreenCaptureCallback ohCallback;
    ohCallback.onError = ScreenCaptureCapiMock::OnError;
    ohCallback.onAudioBufferAvailable = ScreenCaptureCapiMock::OnAudioBufferAvailable;
    ohCallback.onVideoBufferAvailable = ScreenCaptureCapiMock::OnVideoBufferAvailable;
    int ret = OH_AVScreenCapture_SetCallback(screenCapture_, ohCallback);
    if (ret != AV_SCREEN_CAPTURE_ERR_OK) {
        MEDIA_LOGE("ScreenCaptureCapiMock SetCallback failed, ret: %{public}d", ret);
        return MSERR_UNKNOWN;
    }
    if (isErrorCallBackEnabled) {
        MEDIA_LOGD("ScreenCaptureCapiMock SetErrorCallback");
        isErrorCallBackEnabled_ = isErrorCallBackEnabled;
        ret = OH_AVScreenCapture_SetErrorCallback(screenCapture_, ScreenCaptureCapiMock::OnErrorNew, this);
        if (ret != AV_SCREEN_CAPTURE_ERR_OK) {
            MEDIA_LOGE("ScreenCaptureCapiMock SetErrorCallback failed, ret: %{public}d", ret);
            return MSERR_UNKNOWN;
        }
    }
    if (isDataCallBackEnabled) {
        MEDIA_LOGD("ScreenCaptureCapiMock SetDataCallback");
        isDataCallBackEnabled_ = isDataCallBackEnabled;
        ret = OH_AVScreenCapture_SetDataCallback(screenCapture_, ScreenCaptureCapiMock::OnBufferAvailable, this);
        if (ret != AV_SCREEN_CAPTURE_ERR_OK) {
            MEDIA_LOGE("ScreenCaptureCapiMock SetDataCallback failed, ret: %{public}d", ret);
            return MSERR_UNKNOWN;
        }
    }
    if (isStateChangeCallBackEnabled) {
        MEDIA_LOGD("ScreenCaptureCapiMock SetStateCallback");
        isStateChangeCallBackEnabled_ = isStateChangeCallBackEnabled;
        ret = OH_AVScreenCapture_SetStateCallback(screenCapture_, ScreenCaptureCapiMock::OnStateChange, this);
        if (ret != AV_SCREEN_CAPTURE_ERR_OK) {
            MEDIA_LOGE("ScreenCaptureCapiMock SetStateCallback failed, ret: %{public}d", ret);
            return MSERR_UNKNOWN;
        }
    }
    return MSERR_OK;
}

int32_t ScreenCaptureCapiMock::StartScreenCapture()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_StartScreenCapture(screenCapture_);
}

int32_t ScreenCaptureCapiMock::StartScreenCaptureWithSurface(const std::any& value)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    sptr<Surface> surface = std::any_cast<sptr<Surface>>(value);
    OHNativeWindow* nativeWindow = new OHNativeWindow();
    nativeWindow->surface = surface;
    return OH_AVScreenCapture_StartScreenCaptureWithSurface(screenCapture_, nativeWindow);
}

int32_t ScreenCaptureCapiMock::Init(AVScreenCaptureConfig config)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    OH_AVScreenCaptureConfig tempConfig = Convert(config);
    return OH_AVScreenCapture_Init(screenCapture_, tempConfig);
}

int32_t ScreenCaptureCapiMock::StopScreenCapture()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_StopScreenCapture(screenCapture_);
}

int32_t ScreenCaptureCapiMock::StartScreenRecording()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_StartScreenRecording(screenCapture_);
}

int32_t ScreenCaptureCapiMock::StopScreenRecording()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_StopScreenRecording(screenCapture_);
}

int32_t ScreenCaptureCapiMock::Release()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    DelCallback(screenCapture_);
    int32_t ret = OH_AVScreenCapture_Release(screenCapture_);
    screenCapture_ = nullptr;
    return ret;
}

int32_t ScreenCaptureCapiMock::SetMicrophoneEnabled(bool isMicrophone)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_SetMicrophoneEnabled(screenCapture_, isMicrophone);
}

int32_t ScreenCaptureCapiMock::SetCanvasRotation(bool canvasRotation)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_SetCanvasRotation(screenCapture_, canvasRotation);
}

int32_t ScreenCaptureCapiMock::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audioBuffer,
    AudioCaptureSourceType type)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    OH_AudioBuffer *buffer = static_cast<OH_AudioBuffer *>(malloc(sizeof(OH_AudioBuffer)));
    if (buffer == nullptr) {
        return MSERR_INVALID_OPERATION;
    }
    auto t_type = static_cast<OH_AudioCaptureSourceType>(type);
    int32_t ret = OH_AVScreenCapture_AcquireAudioBuffer(screenCapture_, &buffer, t_type);
    if (ret == AV_SCREEN_CAPTURE_ERR_OK) {
        // If ret is not AV_SCREEN_CAPTURE_ERR_OK, the object referenced by buffer is not initialized and can't be used.
        audioBuffer =
            std::make_shared<AudioBuffer>(buffer->buf, buffer->size, buffer->timestamp,
                static_cast<AudioCaptureSourceType>(buffer->type));
    }
    free(buffer);
    buffer = nullptr;
    return ret;
}

sptr<OHOS::SurfaceBuffer> ScreenCaptureCapiMock::AcquireVideoBuffer(int32_t &fence, int64_t &timestamp,
    OHOS::Rect &damage)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, nullptr, "screenCapture_ == nullptr");
    OH_Rect damage_ = {
        .x = damage.x,
        .y = damage.y,
        .width = damage.w,
        .height = damage.h,
    };
    OH_NativeBuffer* buffer = OH_AVScreenCapture_AcquireVideoBuffer(screenCapture_, &fence, &timestamp, &damage_);
    sptr<OHOS::SurfaceBuffer> surfacebuffer;
    if (buffer != nullptr) {
        surfacebuffer =  OHOS::SurfaceBuffer::NativeBufferToSurfaceBuffer(buffer);
    }
    return surfacebuffer;
}

int32_t ScreenCaptureCapiMock::ReleaseAudioBuffer(OHOS::Media::AudioCaptureSourceType type)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    auto t_type = static_cast<OH_AudioCaptureSourceType>(type);
    return OH_AVScreenCapture_ReleaseAudioBuffer(screenCapture_, t_type);
}

int32_t ScreenCaptureCapiMock::ReleaseVideoBuffer()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(screenCapture_ != nullptr, MSERR_INVALID_OPERATION, "screenCapture_ == nullptr");
    return OH_AVScreenCapture_ReleaseVideoBuffer(screenCapture_);
}