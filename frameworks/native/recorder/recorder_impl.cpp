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

#include "recorder_impl.h"
#include <map>
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<Recorder> RecorderFactory::CreateRecorder()
{
    std::shared_ptr<RecorderImpl> impl = std::make_shared<RecorderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new RecorderImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init RecorderImpl");
    return impl;
}

int32_t RecorderImpl::Init()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Init in", FAKE_POINTER(this));
    HiTraceChain::SetId(traceId_);
    recorderService_ = MediaServiceFactory::GetInstance().CreateRecorderService();
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_NO_MEMORY, "failed to create recorder service");
    return MSERR_OK;
}

int32_t RecorderImpl::GetAVRecorderConfig(ConfigMap &configMap)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->GetAVRecorderConfig(configMap);
}

int32_t RecorderImpl::GetLocation(Location &location)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->GetLocation(location);
}

RecorderImpl::RecorderImpl()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    traceId_ = HiTraceChain::Begin("AVRecorder", HITRACE_FLAG_DEFAULT);
}

RecorderImpl::~RecorderImpl()
{
    if (recorderService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyRecorderService(recorderService_);
        recorderService_ = nullptr;
    }
    HiTraceChain::End(traceId_);
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t RecorderImpl::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoSource in, source is %{public}d, sourceId is %{public}d",
        FAKE_POINTER(this), source, sourceId);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoSource(source, sourceId);
}

int32_t RecorderImpl::SetVideoEncoder(int32_t sourceId, VideoCodecFormat encoder)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoEncoder in, sourceId is %{public}d, "
        "encoder is %{public}d", FAKE_POINTER(this), sourceId, encoder);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoEncoder(sourceId, encoder);
}

int32_t RecorderImpl::SetVideoSize(int32_t sourceId, int32_t width, int32_t height)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoSize in, sourceId is %{public}d, width is %{public}d, "
        "height is %{public}d", FAKE_POINTER(this), sourceId, width, height);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoSize(sourceId, width, height);
}

int32_t RecorderImpl::SetVideoFrameRate(int32_t sourceId, int32_t frameRate)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoFrameRate in, sourceId is %{public}d, "
        "frameRate is %{public}d", FAKE_POINTER(this), sourceId, frameRate);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoFrameRate(sourceId, frameRate);
}

int32_t RecorderImpl::RecorderImpl::SetVideoEncodingBitRate(int32_t sourceId, int32_t rate)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoEncodingBitRate in, sourceId is %{public}d, "
        "rate is %{public}d", FAKE_POINTER(this), sourceId, rate);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoEncodingBitRate(sourceId, rate);
}

int32_t RecorderImpl::RecorderImpl::SetVideoIsHdr(int32_t sourceId, bool isHdr)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoIsHdr in, sourceId is %{public}d, isHdr is %{public}d",
        FAKE_POINTER(this), sourceId, isHdr);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoIsHdr(sourceId, isHdr);
}

int32_t RecorderImpl::RecorderImpl::SetVideoEnableTemporalScale(int32_t sourceId, bool enableTemporalScale)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetVideoEnableTemporalScale in, sourceId is %{public}d, "
        "enableTemporalScale is %{public}d", FAKE_POINTER(this), sourceId, enableTemporalScale);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetVideoEnableTemporalScale(sourceId, enableTemporalScale);
}

int32_t RecorderImpl::SetCaptureRate(int32_t sourceId, double fps)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetCaptureRate in, sourceId is %{public}d, "
        "fps is %{public}lf", FAKE_POINTER(this), sourceId, fps);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetCaptureRate(sourceId, fps);
}

sptr<OHOS::Surface> RecorderImpl::GetSurface(int32_t sourceId)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " GetSurface in, sourceId is %{public}d",
        FAKE_POINTER(this), sourceId);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, nullptr, "recorder service does not exist..");
    surface_ = recorderService_->GetSurface(sourceId);
    return surface_;
}

int32_t RecorderImpl::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetAudioSource in, source is %{public}d, sourceId is %{public}d",
        FAKE_POINTER(this), source, sourceId);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioSource(source, sourceId);
}

int32_t RecorderImpl::SetAudioEncoder(int32_t sourceId, AudioCodecFormat encoder)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetAudioEncoder in, sourceId is %{public}d, "
        "encoder is %{public}d", FAKE_POINTER(this), sourceId, encoder);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioEncoder(sourceId, encoder);
}

int32_t RecorderImpl::SetAudioSampleRate(int32_t sourceId, int32_t rate)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetAudioSampleRate in, sourceId is %{public}d, "
        "rate is %{public}d", FAKE_POINTER(this), sourceId, rate);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioSampleRate(sourceId, rate);
}

int32_t RecorderImpl::SetAudioChannels(int32_t sourceId, int32_t num)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetAudioChannels in, sourceId is %{public}d, num is %{public}d",
        FAKE_POINTER(this), sourceId, num);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioChannels(sourceId, num);
}

int32_t RecorderImpl::SetAudioEncodingBitRate(int32_t sourceId, int32_t bitRate)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetAudioEncodingBitRate in, sourceId is %{public}d, "
        "bitRate is %{public}d", FAKE_POINTER(this), sourceId, bitRate);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetAudioEncodingBitRate(sourceId, bitRate);
}

int32_t RecorderImpl::SetDataSource(DataSourceType dataType, int32_t &sourceId)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetDataSource(dataType, sourceId);
}

int32_t RecorderImpl::SetUserCustomInfo(Meta &userCustomInfo)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetUserCustomInfo(userCustomInfo);
}

int32_t RecorderImpl::SetGenre(std::string &genre)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetGenre(genre);
}

int32_t RecorderImpl::SetMaxDuration(int32_t duration)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetMaxDuration in, duration is %{public}d",
        FAKE_POINTER(this), duration);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetMaxDuration(duration);
}

int32_t RecorderImpl::SetOutputFormat(OutputFormatType format)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetOutputFormat in, format is %{public}d",
        FAKE_POINTER(this), format);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetOutputFormat(format);
}

int32_t RecorderImpl::SetOutputFile(int32_t fd)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetOutputFile in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetOutputFile(fd);
}

int32_t RecorderImpl::SetNextOutputFile(int32_t fd)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetNextOutputFile in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetNextOutputFile(fd);
}

int32_t RecorderImpl::SetMaxFileSize(int64_t size)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetMaxFileSize in, size is %{public}" PRIi64 "",
        FAKE_POINTER(this), size);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetMaxFileSize(size);
}

void RecorderImpl::SetLocation(float latitude, float longitude)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetLocation in", FAKE_POINTER(this));
    CHECK_AND_RETURN_LOG(recorderService_ != nullptr, "recorder service does not exist..");
    recorderService_->SetLocation(latitude, longitude);
    return;
}

void RecorderImpl::SetOrientationHint(int32_t rotation)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetMaxDuration in, rotation is %{public}d",
        FAKE_POINTER(this), rotation);
    CHECK_AND_RETURN_LOG(recorderService_ != nullptr, "recorder service does not exist..");
    recorderService_->SetOrientationHint(rotation);
    return;
}

int32_t RecorderImpl::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " SetRecorderCallback in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "input callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetRecorderCallback(callback);
}

int32_t RecorderImpl::Prepare()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Prepare in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Prepare();
}

int32_t RecorderImpl::Start()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Start in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Start();
}

int32_t RecorderImpl::Pause()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Pause in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Pause();
}

int32_t RecorderImpl::Resume()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Resume in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Resume();
}

int32_t RecorderImpl::Stop(bool block)
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Stop in, block is %{public}d",
        FAKE_POINTER(this), block);
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Stop(block);
}

int32_t RecorderImpl::Reset()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Reset in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->Reset();
}

int32_t RecorderImpl::Release()
{
    MEDIA_LOGI("RecorderImpl:0x%{public}06" PRIXPTR " Release in", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    (void)recorderService_->Release();
    (void)MediaServiceFactory::GetInstance().DestroyRecorderService(recorderService_);
    recorderService_ = nullptr;
    surface_ = nullptr;
    return MSERR_OK;
}

int32_t RecorderImpl::SetFileSplitDuration(FileSplitType type, int64_t timestamp, uint32_t duration)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetFileSplitDuration(type, timestamp, duration);
}

int32_t RecorderImpl::SetParameter(int32_t sourceId, const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->SetParameter(sourceId, format);
}

int32_t RecorderImpl::GetCurrentCapturerChangeInfo(AudioRecorderChangeInfo &changeInfo)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->GetCurrentCapturerChangeInfo(changeInfo);
}

int32_t RecorderImpl::GetAvailableEncoder(std::vector<EncoderCapabilityData> &encoderInfo)
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->GetAvailableEncoder(encoderInfo);
}

int32_t RecorderImpl::GetMaxAmplitude()
{
    CHECK_AND_RETURN_RET_LOG(recorderService_ != nullptr, MSERR_INVALID_OPERATION, "recorder service does not exist..");
    return recorderService_->GetMaxAmplitude();
}

} // namespace Media
} // namespace OHOS
