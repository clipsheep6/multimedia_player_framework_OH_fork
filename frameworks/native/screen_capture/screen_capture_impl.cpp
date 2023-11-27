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
}

ScreenCaptureImpl::~ScreenCaptureImpl()
{
    if (screenCaptureService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyScreenCaptureService(screenCaptureService_);
        screenCaptureService_ = nullptr;
    }
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t ScreenCaptureImpl::Release()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " CheckScreenCapturePermssion in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    screenCaptureService_->Release();
    return MSERR_OK;
}

bool ScreenCaptureImpl::NeedStartInnerAudio(AudioCaptureSourceType type)
{
    if ((type == ALL_PLAYBACK) || (type == APP_PLAYBACK)) {
        return true;
    }
    MEDIA_LOGI("No need start inner audiocapture");
    return false;
}

int32_t ScreenCaptureImpl::SetMicrophoneEnabled(bool isMicrophone)
{
    MEDIA_LOGD("SetMicrophoneEnabled:0x%{public}06" PRIXPTR " init in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->SetMicrophoneEnabled(isMicrophone);
}

int32_t ScreenCaptureImpl::Init(AVScreenCaptureConfig config)
{
    MEDIA_LOGD("InitScreenCapture:0x%{public}06" PRIXPTR " init in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");

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
    return ret;
}

int32_t ScreenCaptureImpl::InitOriginalStream(AVScreenCaptureConfig config)
{
    CHECK_AND_RETURN_RET_LOG(config.audioInfo.micCapInfo.audioSource == AudioCaptureSourceType::SOURCE_DEFAULT ||
        config.audioInfo.micCapInfo.audioSource == AudioCaptureSourceType::MIC, MSERR_INVALID_VAL,
        "audioSource source type error");
    int32_t ret = MSERR_OK;
    ret = screenCaptureService_->InitAudioCap(config.audioInfo.micCapInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "initMicAudioCap failed");
    ret = screenCaptureService_->InitVideoCap(config.videoInfo.videoCapInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "initVideoCap failed");

    if (NeedStartInnerAudio(config.audioInfo.innerCapInfo.audioSource)) {
        ret = screenCaptureService_->InitAudioCap(config.audioInfo.innerCapInfo);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "initInnerAudioCap failed");
    }
    return ret;
}

int32_t ScreenCaptureImpl::InitCaptureFile(AVScreenCaptureConfig config)
{
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
    int32_t retMic = MSERR_OK;
    int32_t retInner = MSERR_OK;
    AudioCaptureSourceType type = config.audioInfo.micCapInfo.audioSource;
    if (type == AudioCaptureSourceType::SOURCE_DEFAULT || type == AudioCaptureSourceType::MIC) {
        config.audioInfo.micCapInfo.audioSource = AudioCaptureSourceType::MIC;
        retMic = screenCaptureService_->InitAudioCap(config.audioInfo.micCapInfo);
    } else {
        return MSERR_INVALID_VAL;
    }
    type = config.audioInfo.innerCapInfo.audioSource;
    if (type == AudioCaptureSourceType::ALL_PLAYBACK || type == AudioCaptureSourceType::APP_PLAYBACK) {
        retInner = screenCaptureService_->InitAudioCap(config.audioInfo.innerCapInfo);
    } else if (type == AudioCaptureSourceType::SOURCE_DEFAULT) {
        retInner = MSERR_INVALID_VAL;
    } else {
        return MSERR_INVALID_VAL;
    }
    CHECK_AND_RETURN_RET_LOG(retMic == MSERR_OK || retInner == MSERR_OK, MSERR_INVALID_VAL, "InitAudioCap failed");
    ret = screenCaptureService_->InitVideoEncInfo(config.videoInfo.videoEncInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoEncInfo failed");
    ret = screenCaptureService_->InitVideoCap(config.videoInfo.videoCapInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "InitVideoCap failed");
    return ret;
}

int32_t ScreenCaptureImpl::StartScreenCapture()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StartScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->StartScreenCapture();
}

int32_t ScreenCaptureImpl::StopScreenCapture()
{
    MEDIA_LOGD("ScreenCaptureImpl:0x%{public}06" PRIXPTR " StopScreenCapture in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(screenCaptureService_ != nullptr, MSERR_NO_MEMORY,
        "screen capture service does not exist..");
    return screenCaptureService_->StopScreenCapture();
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