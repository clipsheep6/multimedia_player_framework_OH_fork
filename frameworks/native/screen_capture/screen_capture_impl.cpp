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

#include "screen_capture_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "i_media_service.h"
#include "string_ex.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "ScreenCaptureImpl"};
}
namespace OHOS {
namespace Media {
using namespace OHOS::HiviewDFX;
std::shared_ptr<ScreenCapture> ScreenCaptureFactory::CreateScreenCapture()
{
    std::shared_ptr<ScreenCaptureImpl> impl = std::make_shared<ScreenCaptureImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new ScreenCaptureImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init ScreenCaptureImpl");

    return impl;
}

int32_t ScreenCaptureImpl::Init()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " Init in", FAKE_POINTER(this));
    HiTraceChain::SetId(traceId_);
    screenCaptureService_ = MediaServiceFactory::GetInstance().CreateScreenCaptureService();
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_UNKNOWN,
        "failed to create ScreenCapture service");
    return MSERR_OK;
}

int32_t ScreenCaptureImpl::SetScreenCaptureCallback(const std::shared_ptr<ScreenCaptureCallBack> &callback)
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " SetScreenCaptureCallback in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "input callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr,  MSERR_INVALID_OPERATION,
        "screen capture service does not exist..");
    return screenCaptureService_->SetScreenCaptureCallback(callback);
}

ScreenCaptureImpl::ScreenCaptureImpl()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    traceId_ = HiTraceChain::Begin("AVScreenCapture", HITRACE_FLAG_DEFAULT);
}

ScreenCaptureImpl::~ScreenCaptureImpl()
{
    if (screenCaptureService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyScreenCaptureService(screenCaptureService_);
        screenCaptureService_ = nullptr;
    }
    HiTraceChain::End(traceId_);
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureImpl::Release()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " Release in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    screenCaptureService_->Release();
    MEDIA_LOGD("ScreenCaptureImpl: 0x%{public}06" PRIXPTR "Release end.", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t ScreenCaptureImpl::ExcludeContent(ScreenCaptureContentFilter &contentFilter)
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " ExcludeContent in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->ExcludeContent(contentFilter);
}

int32_t ScreenCaptureImpl::SetPrivacyAuthorityEnabled()
{
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    isPrivacyAuthorityEnabled_ = true;
    MEDIA_LOGI("ScreenCaptureImpl:0x%{public}06" PRIXPTR " isPrivacyAuthorityEnabled:%{public}d",
        FAKE_POINTER(this), isPrivacyAuthorityEnabled_);
    return MSERR_OK;
}

bool ScreenCaptureImpl::IsAudioCapInfoIgnored(const AudioCaptureInfo &audioCapInfo)
{
    return audioCapInfo.audioChannels == 0 && audioCapInfo.audioSampleRate == 0;
}

bool ScreenCaptureImpl::IsVideoCapInfoIgnored(const VideoCaptureInfo &videoCapInfo)
{
    return videoCapInfo.videoFrameWidth == 0 && videoCapInfo.videoFrameHeight == 0;
}

int32_t ScreenCaptureImpl::SetMicrophoneEnabled(bool isMicrophone)
{
    MEDIA_LOGD("SetMicrophoneEnabled:0x%{public}06" PRIXPTR " init in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->SetMicrophoneEnabled(isMicrophone);
}

int32_t ScreenCaptureImpl::SetCanvasRotation(bool canvasRotation)
{
    MEDIA_LOGD("SetCanvasRotation:0x%{public}06" PRIXPTR " init in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
                             "screen capture service does not exist..");
    return screenCaptureService_->SetCanvasRotation(canvasRotation);
}

int32_t ScreenCaptureImpl::Init(AVScreenCaptureConfig config)
{
    MEDIA_LOGD("InitScreenCapture:0x%{public}06" PRIXPTR " init in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");

    dataType_ = config.dataType;
    int32_t ret = MSERR_OK;
    ret = screenCaptureService_->SetCaptureMode(config.captureMode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetCaptureMode failed");

    ret = screenCaptureService_->SetDataType(config.dataType);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetDataType failed");

    switch (config.dataType) {
        case ORIGINAL_STREAM: {
            ret = InitOriginalStream(config);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitOriginalStream failed");
            break;
        }
        case ENCODED_STREAM:
            MEDIA_LOGI("start cap encoded stream,still not support");
            return MSERR_UNSUPPORT;
        case CAPTURE_FILE: {
            ret = InitCaptureFile(config);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitCaptureFile failed");
            break;
        }
        default:
            MEDIA_LOGI("invaild type");
            return MSERR_INVALID_VAL;
    }
    MEDIA_LOGD("InitScreenCapture: 0x%{public}06" PRIXPTR "Init end.", FAKE_POINTER(this));
    return ret;
}

int32_t ScreenCaptureImpl::InitOriginalStream(AVScreenCaptureConfig config)
{
    // For original stream:
    // 1. Any of innerCapInfo/videoCapInfo should be not invalid and should not be both ignored
    // 2. micCapInfo should not be invalid
    MEDIA_LOGI("ScreenCaptureImpl: 0x%{public}06" PRIXPTR
        "InitOriginalStream start, innerCapInfo.audioSampleRate:%{public}d, innerCapInfo.audioChannels::%{public}d, "
        "micCapInfo.audioSampleRate:%{public}d, micCapInfo.audioChannels:%{public}d, "
        "videoCapInfo.videoFrameWidth:%{public}d, videoCapInfo.videoFrameHeight:%{public}d.", FAKE_POINTER(this),
        config.audioInfo.innerCapInfo.audioSampleRate, config.audioInfo.innerCapInfo.audioChannels,
        config.audioInfo.micCapInfo.audioSampleRate, config.audioInfo.micCapInfo.audioChannels,
        config.videoInfo.videoCapInfo.videoFrameWidth, config.videoInfo.videoCapInfo.videoFrameHeight);
    int32_t ret = MSERR_OK;
    bool isInnerAudioCapInfoIgnored = IsAudioCapInfoIgnored(config.audioInfo.innerCapInfo);
    bool isVideoCapInfoIgnored = IsVideoCapInfoIgnored(config.videoInfo.videoCapInfo);
    CHECK_AND_RETURN_RET_LOG(!(isInnerAudioCapInfoIgnored && isVideoCapInfoIgnored), MSERR_INVALID_VAL,
        "init cap failed, both innerAudioCap and videoCap Info ignored is not allowed");
    if (!isInnerAudioCapInfoIgnored) {
        ret = screenCaptureService_->InitAudioCap(config.audioInfo.innerCapInfo);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init innerAudioCap failed");
    }
    if (!isVideoCapInfoIgnored) {
        ret = screenCaptureService_->InitVideoCap(config.videoInfo.videoCapInfo);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init videoCap failed");
    }
    if (!IsAudioCapInfoIgnored(config.audioInfo.micCapInfo)) {
        ret = screenCaptureService_->InitAudioCap(config.audioInfo.micCapInfo);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init micAudioCap failed");
    }
    MEDIA_LOGI("ScreenCaptureImpl: 0x%{public}06" PRIXPTR "InitOriginalStream end.", FAKE_POINTER(this));
    return ret;
}

int32_t ScreenCaptureImpl::InitCaptureFile(AVScreenCaptureConfig config)
{
    // For capture file:
    // 1. All of innerCapInfo/videoCapInfo/audioEncInfo/videoEncInfo should be be valid
    // 2. micCapInfo should not be invalid
    MEDIA_LOGI("ScreenCaptureImpl: 0x%{public}06" PRIXPTR "InitCaptureFile start, url:%{public}s, "
        "videoEncInfo.audioBitrate:%{public}d, videoEncInfo.audioCodecformat:%{public}d, "
        "innerCapInfo.audioSampleRate:%{public}d, innerCapInfo.audioChannels::%{public}d, "
        "micCapInfo.audioSampleRate:%{public}d, micCapInfo.audioChannels:%{public}d, "
        "videoCapInfo.displayId:%{public}" PRIu64 ", videoCapInfo.taskIDs.size:%{public}zu, "
        "videoCapInfo.videoSource:%{public}d.", FAKE_POINTER(this), config.recorderInfo.url.c_str(),
        config.audioInfo.audioEncInfo.audioBitrate, config.audioInfo.audioEncInfo.audioCodecformat,
        config.audioInfo.innerCapInfo.audioSampleRate, config.audioInfo.innerCapInfo.audioChannels,
        config.audioInfo.micCapInfo.audioSampleRate, config.audioInfo.micCapInfo.audioChannels,
        config.videoInfo.videoCapInfo.displayId, config.videoInfo.videoCapInfo.taskIDs.size(),
        config.videoInfo.videoCapInfo.videoSource);
    int32_t ret = MSERR_OK;
    ret = screenCaptureService_->SetRecorderInfo(config.recorderInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetRecorderInfo failed");
    const std::string fdHead = "fd://";
    CHECK_AND_RETURN_RET_LOG(config.recorderInfo.url.find(fdHead) != std::string::npos, MSERR_INVALID_VAL,
        "check url failed");
    int32_t outputFd = -1;
    std::string inputFd = config.recorderInfo.url.substr(fdHead.size());
    CHECK_AND_RETURN_RET_LOG(StrToInt(inputFd, outputFd) == true && outputFd >= 0, MSERR_INVALID_VAL,
        "open file failed");
    ret = screenCaptureService_->SetOutputFile(outputFd);
    close(outputFd);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetOutputFile failed");
    ret = screenCaptureService_->InitAudioEncInfo(config.audioInfo.audioEncInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitAudioEncInfo failed");

    bool isInnerAudioCapInfoIgnored = IsAudioCapInfoIgnored(config.audioInfo.innerCapInfo);
    CHECK_AND_RETURN_RET_LOG(!isInnerAudioCapInfoIgnored, MSERR_INVALID_VAL,
        "init innerCapInfo failed, innerCapInfo ignored is now allowed");
    bool isMicAudioCapInfoIgnored = IsAudioCapInfoIgnored(config.audioInfo.micCapInfo);
    if (!isMicAudioCapInfoIgnored) {
        if ((config.audioInfo.micCapInfo.audioChannels != config.audioInfo.innerCapInfo.audioChannels) ||
            (config.audioInfo.micCapInfo.audioSampleRate != config.audioInfo.innerCapInfo.audioSampleRate)) {
            MEDIA_LOGE("InitCaptureFile error, inner and mic param not consistent");
            return MSERR_INVALID_VAL;
        }
        ret = screenCaptureService_->InitAudioCap(config.audioInfo.micCapInfo);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init micAudioCap failed");
    }
    ret = screenCaptureService_->InitAudioCap(config.audioInfo.innerCapInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init innerCapInfo failed, innerCapInfo should be valid");

    ret = screenCaptureService_->InitVideoEncInfo(config.videoInfo.videoEncInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoEncInfo failed");
    ret = screenCaptureService_->InitVideoCap(config.videoInfo.videoCapInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoCap failed");
    return ret;
}

int32_t ScreenCaptureImpl::StartScreenCapture()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StartScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_UNKNOWN,
        "screen capture service does not exist..");
    if (dataType_ == ORIGINAL_STREAM) {
        return screenCaptureService_->StartScreenCapture(isPrivacyAuthorityEnabled_);
    } else {
        MEDIA_LOGE("ScreenCaptureImpl::StartScreenCapture error , dataType_ : %{public}d", dataType_);
        return MSERR_INVALID_VAL;
    }
}

int32_t ScreenCaptureImpl::StartScreenCaptureWithSurface(sptr<Surface> surface)
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StartScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_UNKNOWN,
        "screen capture service does not exist..");
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_UNKNOWN, "surface is nullptr");
    if (dataType_ == ORIGINAL_STREAM) {
        return screenCaptureService_->StartScreenCaptureWithSurface(surface, isPrivacyAuthorityEnabled_);
    } else {
        MEDIA_LOGE("ScreenCaptureImpl::StartScreenCaptureWithSurface error , dataType_ : %{public}d", dataType_);
        return MSERR_INVALID_VAL;
    }
}

int32_t ScreenCaptureImpl::StopScreenCapture()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StopScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_UNKNOWN,
        "screen capture service does not exist..");
    if (dataType_ == ORIGINAL_STREAM) {
        isPrivacyAuthorityEnabled_ = false;
        return screenCaptureService_->StopScreenCapture();
    } else {
        MEDIA_LOGE("ScreenCaptureImpl::StopScreenCapture error , dataType_ : %{public}d", dataType_);
        return MSERR_INVALID_VAL;
    }
}

int32_t ScreenCaptureImpl::StartScreenRecording()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StartScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_UNKNOWN,
        "screen capture service does not exist..");
    if (dataType_ == CAPTURE_FILE) {
        return screenCaptureService_->StartScreenCapture(isPrivacyAuthorityEnabled_);
    } else {
        MEDIA_LOGE("ScreenCaptureImpl::StartScreenRecording error , dataType_ : %{public}d", dataType_);
        return MSERR_INVALID_VAL;
    }
}

int32_t ScreenCaptureImpl::StopScreenRecording()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StopScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_UNKNOWN,
        "screen capture service does not exist..");
    if (dataType_ == CAPTURE_FILE) {
        isPrivacyAuthorityEnabled_ = false;
        return screenCaptureService_->StopScreenCapture();
    } else {
        MEDIA_LOGE("ScreenCaptureImpl::StopScreenRecording error , dataType_ : %{public}d", dataType_);
        return MSERR_INVALID_VAL;
    }
}

int32_t ScreenCaptureImpl::AcquireAudioBuffer(std::shared_ptr<AudioBuffer> &audiobuffer, AudioCaptureSourceType type)
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " AcquireAudioBuffer in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->AcquireAudioBuffer(audiobuffer, type);
}

sptr<OHOS::SurfaceBuffer> ScreenCaptureImpl::AcquireVideoBuffer(int32_t &fence, int64_t &timestamp, OHOS::Rect &damage)
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " AcquireVideoBuffer in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, nullptr,
        "screen capture service does not exist..");

    sptr<OHOS::SurfaceBuffer> surfacebuffer = new SurfaceBufferImpl(0);
    int32_t ret = screenCaptureService_->AcquireVideoBuffer(surfacebuffer, fence, timestamp, damage);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("AcquireVideoBuffer get failed");
        return nullptr;
    }
    MEDIA_LOGD("ScreenCaptureImpl: 0x%{public}06" PRIXPTR "AcquireVideoBuffer end.", FAKE_POINTER(this));
    return surfacebuffer;
}

int32_t ScreenCaptureImpl::ReleaseAudioBuffer(AudioCaptureSourceType type)
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " ReleaseAudioBuffer in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->ReleaseAudioBuffer(type);
}

int32_t ScreenCaptureImpl::ReleaseVideoBuffer()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " ReleaseVideoBuffer in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->ReleaseVideoBuffer();
}
} // namespace Media
} // namespace OHOS