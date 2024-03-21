/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy  of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "hirecorder_impl.h"
#include "meta/audio_types.h"
#include "sync_fence.h"
#include <sys/syscall.h>

namespace OHOS {
namespace Media {
class RecorderEventReceiver : public Pipeline::EventReceiver {
public:
    explicit RecorderEventReceiver(HiRecorderImpl *hiRecorderImpl)
    {
        hiRecorderImpl_ = hiRecorderImpl;
    }

    void OnEvent(const Event &event)
    {
        hiRecorderImpl_->OnEvent(event);
    }

private:
    HiRecorderImpl *hiRecorderImpl_;
};

class RecorderFilterCallback : public Pipeline::FilterCallback {
public:
    explicit RecorderFilterCallback(HiRecorderImpl *hiRecorderImpl)
    {
        hiRecorderImpl_ = hiRecorderImpl;
    }

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType)
    {
        return hiRecorderImpl_->OnCallback(filter, cmd, outType);
    }

private:
    HiRecorderImpl *hiRecorderImpl_;
};

class CapturerInfoChangeCallback : public AudioStandard::AudioCapturerInfoChangeCallback {
public:
    explicit CapturerInfoChangeCallback(HiRecorderImpl *hiRecorderImpl)
    {
        hiRecorderImpl_ = hiRecorderImpl;
    }

    void OnStateChange(const AudioStandard::AudioCapturerChangeInfo &capturerChangeInfo)
    {
        if (hiRecorderImpl_) {
            hiRecorderImpl_->OnAudioCaptureChange(capturerChangeInfo);
        }
    }

private:
    HiRecorderImpl *hiRecorderImpl_;
};

HiRecorderImpl::HiRecorderImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
    : appUid_(appUid), appPid_(appPid), appTokenId_(appTokenId), appFullTokenId_(appFullTokenId)
{
    pipeline_ = std::make_shared<Pipeline::Pipeline>();
    int tid = static_cast<pid_t>(::syscall(SYS_gettid));
    avRecorderTag_ = avRecorderTag_ + "[" + std::to_string(tid) + "]";
}

HiRecorderImpl::~HiRecorderImpl()
{
    Stop(false);
}

int32_t HiRecorderImpl::Init()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Init enter.", avRecorderTag_.c_str());
    recorderEventReceiver_ = std::make_shared<RecorderEventReceiver>(this);
    recorderCallback_ = std::make_shared<RecorderFilterCallback>(this);
    pipeline_->Init(recorderEventReceiver_, recorderCallback_);
    return (int32_t)Status::OK;
}

int32_t HiRecorderImpl::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    sourceId = INVALID_SOURCE_ID;
    FALSE_RETURN_V(source != VideoSourceType::VIDEO_SOURCE_BUTT,
        (int32_t)Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(videoCount_ < static_cast<int32_t>(VIDEO_SOURCE_MAX_COUNT),
        (int32_t)Status::ERROR_INVALID_OPERATION);
    auto tempSourceId = SourceIdGenerator::GenerateVideoSourceId(videoCount_);
    Status ret;
    if (source == VideoSourceType::VIDEO_SOURCE_SURFACE_YUV ||
        source == VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA) {
        videoSourceIsYuv_ = true;
        if (!videoEncoderFilter_) {
            videoEncoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::SurfaceEncoderFilter>
                ("videoEncoderFilter", Pipeline::FilterType::FILTERTYPE_VENC);
        }
        videoEncoderFilter_->SetLogTag(avRecorderTag_);
        ret = pipeline_->AddHeadFilters({videoEncoderFilter_});
        if (source == VideoSourceType::VIDEO_SOURCE_SURFACE_RGBA) {
            videoSourceIsRGBA_ = true;
        } else {
            videoSourceIsRGBA_ = false;
        }
        MEDIA_LOG_I(PUBLIC_LOG_S "SetVideoSource VIDEO_SOURCE_SURFACE_YUV.", avRecorderTag_.c_str());
    } else if (source == VideoSourceType::VIDEO_SOURCE_SURFACE_ES) {
        videoSourceIsYuv_ = false;
        videoSourceIsRGBA_ = false;
        videoCaptureFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::VideoCaptureFilter>
            ("videoEncoderFilter", Pipeline::FilterType::VIDEO_CAPTURE);
        videoCaptureFilter_->SetLogTag(avRecorderTag_);
        ret = pipeline_->AddHeadFilters({videoCaptureFilter_});
        MEDIA_LOG_I(PUBLIC_LOG_S "SetVideoSource VIDEO_SOURCE_SURFACE_ES.", avRecorderTag_.c_str());
    } else {
        ret = Status::OK;
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret,
        PUBLIC_LOG_S "AddFilters videoEncoder to pipeline fail", avRecorderTag_.c_str());
    if (ret == Status::OK) {
        MEDIA_LOG_I(PUBLIC_LOG_S "SetVideoSource success.", avRecorderTag_.c_str());
        videoCount_++;
        videoSourceId_ = tempSourceId;
        sourceId = videoSourceId_;
        OnStateChanged(StateId::RECORDING_SETTING);
    }
    return (int32_t)ret;
}

int32_t HiRecorderImpl::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetAudioSource enter.", avRecorderTag_.c_str());
    sourceId = INVALID_SOURCE_ID;
    FALSE_RETURN_V(CheckAudioSourceType(source), (int32_t)Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(audioCount_ < static_cast<int32_t>(AUDIO_SOURCE_MAX_COUNT),
        (int32_t)Status::ERROR_INVALID_OPERATION);
    auto tempSourceId = SourceIdGenerator::GenerateAudioSourceId(audioCount_);

    audioCaptureFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::AudioCaptureFilter>
        ("audioCaptureFilter", Pipeline::FilterType::AUDIO_CAPTURE);
    if (audioCaptureFilter_ == nullptr) {
        MEDIA_LOG_E(PUBLIC_LOG_S "HiRecorderImpl::audioCaptureFilter_ == nullptr", avRecorderTag_.c_str());
    }
    audioCaptureFilter_->SetLogTag(avRecorderTag_);
    audioCaptureFilter_->SetAudioSource(source);
    Status ret = pipeline_->AddHeadFilters({audioCaptureFilter_});
    FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)ret,
        PUBLIC_LOG_S "AddFilters audioCapture to pipeline fail", avRecorderTag_.c_str());
    if (ret == Status::OK) {
        MEDIA_LOG_I(PUBLIC_LOG_S "SetAudioSource success.", avRecorderTag_.c_str());
        audioCount_++;
        audioSourceId_ = tempSourceId;
        sourceId = static_cast<int32_t>(audioSourceId_);
        OnStateChanged(StateId::RECORDING_SETTING);
    }
    return (int32_t)ret;
}

int32_t HiRecorderImpl::SetOutputFormat(OutputFormatType format)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetOutputFormat enter. " PUBLIC_LOG_D32, avRecorderTag_.c_str(),
        static_cast<int32_t>(format));
    outputFormatType_ = format;
    OnStateChanged(StateId::RECORDING_SETTING);
    return (int32_t)Status::OK;
}

int32_t HiRecorderImpl::SetObs(const std::weak_ptr<IRecorderEngineObs> &obs)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetObs enter.", avRecorderTag_.c_str());
    obs_ = obs;
    return (int32_t)Status::OK;
}

int32_t HiRecorderImpl::Configure(int32_t sourceId, const RecorderParam &recParam)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Configure enter.", avRecorderTag_.c_str());
    FALSE_RETURN_V(outputFormatType_ != OutputFormatType::FORMAT_BUTT,
        (int32_t)Status::ERROR_INVALID_OPERATION);
    FALSE_RETURN_V(CheckParamType(sourceId, recParam), (int32_t)Status::ERROR_INVALID_PARAMETER);
    switch (recParam.type) {
        case RecorderPublicParamType::AUD_SAMPLERATE:
        case RecorderPublicParamType::AUD_CHANNEL:
        case RecorderPublicParamType::AUD_BITRATE:
        case RecorderPublicParamType::AUD_ENC_FMT:
            ConfigureAudio(recParam);
            break;
        case RecorderPublicParamType::VID_CAPTURERATE:
        case RecorderPublicParamType::VID_RECTANGLE:
        case RecorderPublicParamType::VID_BITRATE:
        case RecorderPublicParamType::VID_FRAMERATE:
        case RecorderPublicParamType::VID_ENC_FMT:
        case RecorderPublicParamType::VID_IS_HDR:
            ConfigureVideo(recParam);
            break;
        case RecorderPublicParamType::OUT_PATH:
        case RecorderPublicParamType::OUT_FD:
        case RecorderPublicParamType::VID_ORIENTATION_HINT:
        case RecorderPublicParamType::GEO_LOCATION:
            ConfigureMuxer(recParam);
            break;
        default:
            break;
    }
    OnStateChanged(StateId::RECORDING_SETTING);
    return (int32_t)Status::OK;
}

sptr<Surface> HiRecorderImpl::GetSurface(int32_t sourceId)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetSurface enter.", avRecorderTag_.c_str());
    if (producerSurface_) {
        return producerSurface_;
    }
    if (videoEncoderFilter_) {
        producerSurface_ = videoEncoderFilter_->GetInputSurface();
    }
    if (videoCaptureFilter_) {
        producerSurface_ = videoCaptureFilter_->GetInputSurface();
    }
    return producerSurface_;
}

int32_t HiRecorderImpl::Prepare()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Prepare enter.", avRecorderTag_.c_str());
    if (lseek(fd_, 0, SEEK_CUR) == -1) {
        MEDIA_LOG_E(PUBLIC_LOG_S "The fd is invalid.", avRecorderTag_.c_str());
        return (int32_t)Status::ERROR_UNKNOWN;
    }
    if (audioCaptureFilter_) {
        audioEncFormat_->Set<Tag::APP_TOKEN_ID>(appTokenId_);
        audioEncFormat_->Set<Tag::APP_UID>(appUid_);
        audioEncFormat_->Set<Tag::APP_PID>(appPid_);
        audioEncFormat_->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
        audioEncFormat_->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
        audioCaptureFilter_->SetParameter(audioEncFormat_);
        audioCaptureFilter_->Init(recorderEventReceiver_, recorderCallback_);
        CapturerInfoChangeCallback_ = std::make_shared<CapturerInfoChangeCallback>(this);
        audioCaptureFilter_->SetAudioCaptureChangeCallback(CapturerInfoChangeCallback_);
    }
    if (videoEncoderFilter_) {
        if (videoSourceIsRGBA_) {
            videoEncFormat_->Set<Tag::VIDEO_PIXEL_FORMAT>(Plugins::VideoPixelFormat::RGBA);
            videoEncFormat_->Set<Tag::VIDEO_ENCODE_BITRATE_MODE>(Plugins::VideoEncodeBitrateMode::CBR);
        }
        videoEncoderFilter_->SetCodecFormat(videoEncFormat_);
        videoEncoderFilter_->Init(recorderEventReceiver_, recorderCallback_);
        FALSE_RETURN_V_MSG_E(videoEncoderFilter_->Configure(videoEncFormat_) == Status::OK,
            ERR_UNKNOWN_REASON, PUBLIC_LOG_S "videoEncoderFilter Configure fail", avRecorderTag_.c_str());
    }
    if (videoCaptureFilter_) {
        videoCaptureFilter_->SetCodecFormat(videoEncFormat_);
        videoCaptureFilter_->Init(recorderEventReceiver_, recorderCallback_);
        FALSE_RETURN_V_MSG_E(videoCaptureFilter_->Configure(videoEncFormat_) == Status::OK,
            ERR_UNKNOWN_REASON, PUBLIC_LOG_S "videoCaptureFilter Configure fail", avRecorderTag_.c_str());
    }
    Status ret = pipeline_->Prepare();
    if (ret != Status::OK) {
        return (int32_t)ret;
    }
    return (int32_t)ret;
}

int32_t HiRecorderImpl::Start()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Start enter.", avRecorderTag_.c_str());
    Status ret = Status::OK;
    if (curState_ == StateId::PAUSE) {
        ret = pipeline_->Resume();
    } else {
        ret = pipeline_->Start();
    }
    if (ret == Status::OK) {
        OnStateChanged(StateId::RECORDING);
    }
    return (int32_t)ret;
}

int32_t HiRecorderImpl::Pause()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Pause enter.", avRecorderTag_.c_str());
    Status ret = Status::OK;
    if (curState_ != StateId::READY) {
        ret = pipeline_->Pause();
    }
    if (ret == Status::OK) {
        OnStateChanged(StateId::PAUSE);
    }
    return (int32_t)ret;
}

int32_t HiRecorderImpl::Resume()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Resume enter.", avRecorderTag_.c_str());
    Status ret = Status::OK;
    ret = pipeline_->Resume();
    if (ret == Status::OK) {
        OnStateChanged(StateId::RECORDING);
    }
    return (int32_t)ret;
}

int32_t HiRecorderImpl::Stop(bool isDrainAll)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Stop enter.", avRecorderTag_.c_str());
    Status ret = Status::OK;
    outputFormatType_ = OutputFormatType::FORMAT_BUTT;
    if (audioCaptureFilter_) {
        ret = audioCaptureFilter_->SendEos();
    }
    ret = pipeline_->Stop();
    if (ret == Status::OK) {
        OnStateChanged(StateId::INIT);
    }
    audioCount_ = 0;
    videoCount_ = 0;
    audioSourceId_ = 0;
    videoSourceId_ = 0;
    muxerFilter_ = nullptr;
    if (audioCaptureFilter_) {
        pipeline_->RemoveHeadFilter(audioCaptureFilter_);
    }
    if (videoEncoderFilter_) {
        pipeline_->RemoveHeadFilter(videoEncoderFilter_);
    }
    if (videoCaptureFilter_) {
        pipeline_->RemoveHeadFilter(videoCaptureFilter_);
    }
    FALSE_RETURN_V_MSG_E(curState_ == StateId::INIT, ERR_UNKNOWN_REASON,
        PUBLIC_LOG_S "stop fail", avRecorderTag_.c_str());
    return (int32_t)ret;
}

int32_t HiRecorderImpl::Reset()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Reset enter.", avRecorderTag_.c_str());
    return Stop(false);
}

int32_t HiRecorderImpl::SetParameter(int32_t sourceId, const RecorderParam &recParam)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetParameter enter.", avRecorderTag_.c_str());
    return Configure(sourceId, recParam);
}

void HiRecorderImpl::OnEvent(const Event &event)
{
    switch (event.type) {
        case EventType::EVENT_ERROR: {
            MEDIA_LOG_I(PUBLIC_LOG_S "EVENT_ERROR.", avRecorderTag_.c_str());
            OnStateChanged(StateId::ERROR);
            auto ptr = obs_.lock();
            if (ptr != nullptr) {
                switch (AnyCast<Status>(event.param)) {
                    case Status::ERROR_AUDIO_INTERRUPT:
                        ptr->OnError(IRecorderEngineObs::ErrorType::ERROR_INTERNAL, MSERR_AUD_INTERRUPT);
                        break;
                    default:
                        ptr->OnError(IRecorderEngineObs::ErrorType::ERROR_INTERNAL, MSERR_EXT_API9_INVALID_PARAMETER);
                        break;
                }
            }
            break;
        }
        case EventType::EVENT_READY: {
            OnStateChanged(StateId::READY);
            break;
        }
        case EventType::EVENT_COMPLETE: {
            break;
        }
        default:
            break;
    }
}

Status HiRecorderImpl::OnCallback(std::shared_ptr<Pipeline::Filter> filter, const Pipeline::FilterCallBackCommand cmd,
    Pipeline::StreamType outType)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnCallback enter.", avRecorderTag_.c_str());
    if (cmd == Pipeline::FilterCallBackCommand::NEXT_FILTER_NEEDED) {
        switch (outType) {
            case Pipeline::StreamType::STREAMTYPE_RAW_AUDIO:
                audioEncoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::AudioEncoderFilter>
                    ("audioEncoderFilter", Pipeline::FilterType::FILTERTYPE_AENC);
                audioEncoderFilter_->SetLogTag(avRecorderTag_);
                audioEncoderFilter_->SetCodecFormat(audioEncFormat_);
                audioEncoderFilter_->Init(recorderEventReceiver_, recorderCallback_);
                audioEncoderFilter_->Configure(audioEncFormat_);
                pipeline_->LinkFilters(filter, {audioEncoderFilter_}, outType);
                break;
            case Pipeline::StreamType::STREAMTYPE_ENCODED_AUDIO:
                if (muxerFilter_ == nullptr) {
                    muxerFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::MuxerFilter>
                        ("muxerFilter", Pipeline::FilterType::FILTERTYPE_MUXER);
                    muxerFilter_->SetLogTag(avRecorderTag_);
                    muxerFilter_->Init(recorderEventReceiver_, recorderCallback_);
                    muxerFilter_->SetOutputParameter(appUid_, appPid_, fd_, outputFormatType_);
                    muxerFilter_->SetParameter(muxerFormat_);
                    close(fd_);
                    fd_ = -1;
                }
                pipeline_->LinkFilters(filter, {muxerFilter_}, outType);
                break;
            case Pipeline::StreamType::STREAMTYPE_ENCODED_VIDEO:
                if (muxerFilter_ == nullptr) {
                    muxerFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::MuxerFilter>
                        ("muxerFilter", Pipeline::FilterType::FILTERTYPE_MUXER);
                    muxerFilter_->SetLogTag(avRecorderTag_);
                    muxerFilter_->Init(recorderEventReceiver_, recorderCallback_);
                    muxerFilter_->SetOutputParameter(appUid_, appPid_, fd_, outputFormatType_);
                    muxerFilter_->SetParameter(muxerFormat_);
                    close(fd_);
                    fd_ = -1;
                }
                pipeline_->LinkFilters(filter, {muxerFilter_}, outType);
                break;
            default:
                break;
        }
    }
    return Status::OK;
}

void HiRecorderImpl::OnAudioCaptureChange(const AudioStandard::AudioCapturerChangeInfo &capturerChangeInfo)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnAudioCaptureChange enter.", avRecorderTag_.c_str());
    auto ptr = obs_.lock();
    if (ptr != nullptr) {
        ptr->OnAudioCaptureChange(ConvertCapturerChangeInfo(capturerChangeInfo));
    }
}

int32_t HiRecorderImpl::GetCurrentCapturerChangeInfo(AudioRecorderChangeInfo &changeInfo)
{
    if (audioCaptureFilter_ == nullptr) {
        MEDIA_LOG_E(PUBLIC_LOG_S "audioCaptureFilter_ is nullptr, cannot get audio capturer change info",
            avRecorderTag_.c_str());
        return (int32_t)Status::ERROR_INVALID_OPERATION;
    }
    AudioStandard::AudioCapturerChangeInfo audioChangeInfo;
    Status ret = audioCaptureFilter_->GetCurrentCapturerChangeInfo(audioChangeInfo);
    changeInfo = ConvertCapturerChangeInfo(audioChangeInfo);
    return (int32_t)ret;
}

int32_t HiRecorderImpl::GetAvailableEncoder(std::vector<EncoderCapabilityData> &encoderInfo)
{
    if (!codecCapabilityAdapter_) {
        codecCapabilityAdapter_ = std::make_shared<Pipeline::CodecCapabilityAdapter>();
    }
    codecCapabilityAdapter_->Init();

    std::vector<MediaAVCodec::CapabilityData*> encoderCapData;
    Status ret = codecCapabilityAdapter_->GetAvailableEncoder(encoderCapData);
    
    encoderInfo = ConvertEncoderInfo(encoderCapData);
    return (int32_t)ret;
}

int32_t HiRecorderImpl::GetMaxAmplitude()
{
    if (!audioCaptureFilter_) {
        MEDIA_LOG_E(PUBLIC_LOG_S "audioCaptureFilter_ is null, cannot get audio max amplitude",
            avRecorderTag_.c_str());
        return (int32_t)Status::ERROR_INVALID_OPERATION;
    }
    return audioCaptureFilter_->GetMaxAmplitude();
}

void HiRecorderImpl::ConfigureAudio(const RecorderParam &recParam)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "ConfigureAudio enter.", avRecorderTag_.c_str());
    switch (recParam.type) {
        case RecorderPublicParamType::AUD_SAMPLERATE: {
            AudSampleRate audSampleRate = static_cast<const AudSampleRate&>(recParam);
            audioEncFormat_->Set<Tag::AUDIO_SAMPLE_RATE>(audSampleRate.sampleRate);
            break;
        }
        case RecorderPublicParamType::AUD_CHANNEL: {
            AudChannel audChannel = static_cast<const AudChannel&>(recParam);
            audioEncFormat_->Set<Tag::AUDIO_CHANNEL_COUNT>(audChannel.channel);
            break;
        }
        case RecorderPublicParamType::AUD_BITRATE: {
            AudBitRate audBitRate = static_cast<const AudBitRate&>(recParam);
            if (audBitRate.bitRate <= 0) {
                OnEvent({"audioBitRate", EventType::EVENT_ERROR, Status::ERROR_INVALID_PARAMETER});
            }
            audioEncFormat_->Set<Tag::MEDIA_BITRATE>(audBitRate.bitRate);
            break;
        }
        case RecorderPublicParamType::AUD_ENC_FMT: {
            AudEnc audEnc = static_cast<const AudEnc&>(recParam);
            switch (audEnc.encFmt) {
                case OHOS::Media::AudioCodecFormat::AUDIO_DEFAULT:
                case OHOS::Media::AudioCodecFormat::AAC_LC:
                    audioEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
                    audioEncFormat_->Set<Tag::AUDIO_AAC_PROFILE>(Plugins::AudioAacProfile::LC);
                    break;
                default:
                    break;
            }
            break;
        }
        default: {
            break;
        }
    }
}

void HiRecorderImpl::ConfigureVideo(const RecorderParam &recParam)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "ConfigureVideo enter.", avRecorderTag_.c_str());
    switch (recParam.type) {
        case RecorderPublicParamType::VID_RECTANGLE: {
            VidRectangle vidRectangle = static_cast<const VidRectangle&>(recParam);
            videoEncFormat_->Set<Tag::VIDEO_WIDTH>(vidRectangle.width);
            videoEncFormat_->Set<Tag::VIDEO_HEIGHT>(vidRectangle.height);
            break;
        }
        case RecorderPublicParamType::VID_CAPTURERATE: {
            CaptureRate captureRate = static_cast<const CaptureRate&>(recParam);
            videoEncFormat_->Set<Tag::VIDEO_CAPTURE_RATE>(captureRate.capRate);
            break;
        }
        case RecorderPublicParamType::VID_BITRATE: {
            VidBitRate vidBitRate = static_cast<const VidBitRate&>(recParam);
            if (vidBitRate.bitRate <= 0) {
                OnEvent({"videoBitRate", EventType::EVENT_ERROR, Status::ERROR_INVALID_PARAMETER});
            }
            videoEncFormat_->Set<Tag::MEDIA_BITRATE>(vidBitRate.bitRate);
            break;
        }
        case RecorderPublicParamType::VID_FRAMERATE: {
            VidFrameRate vidFrameRate = static_cast<const VidFrameRate&>(recParam);
            videoEncFormat_->Set<Tag::VIDEO_FRAME_RATE>(vidFrameRate.frameRate);
            break;
        }
        case RecorderPublicParamType::VID_ENC_FMT: {
            videoEncFormat_ = std::make_shared<Meta>();
            ConfigureVideoEncoderFormat(recParam);
            break;
        }
        case RecorderPublicParamType::VID_IS_HDR: {
            VidIsHdr vidIsHdr = static_cast<const VidIsHdr&>(recParam);
            if (vidIsHdr.isHdr) {
                videoEncFormat_->Set<Tag::VIDEO_H265_PROFILE>(Plugins::HEVCProfile::HEVC_PROFILE_MAIN_10);
            }
            break;
        }
        default:
            break;
    }
}

void HiRecorderImpl::ConfigureVideoEncoderFormat(const RecorderParam &recParam)
{
    VidEnc vidEnc = static_cast<const VidEnc&>(recParam);
    switch (vidEnc.encFmt) {
        case OHOS::Media::VideoCodecFormat::H264:
            videoEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
            videoEncFormat_->Set<Tag::VIDEO_H264_PROFILE>(Plugins::VideoH264Profile::BASELINE);
            videoEncFormat_->Set<Tag::VIDEO_H264_LEVEL>(32); // 32: LEVEL 3.2
            break;
        case OHOS::Media::VideoCodecFormat::MPEG4:
            videoEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
            break;
        case OHOS::Media::VideoCodecFormat::H265:
            MEDIA_LOG_I(PUBLIC_LOG_S "ConfigureVideo H265 enter", avRecorderTag_.c_str());
            videoEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_HEVC);
            break;
        default:
            break;
    }
}

bool HiRecorderImpl::CheckAudioSourceType(AudioSourceType sourceType)
{
    switch (sourceType) {
        case AUDIO_SOURCE_DEFAULT:
        case AUDIO_MIC:
        case AUDIO_SOURCE_VOICE_CALL:
        case AUDIO_INNER:
            return true;
        case AUDIO_SOURCE_INVALID:
        default:
            break;
    }
    return false;
}

void HiRecorderImpl::ConfigureMuxer(const RecorderParam &recParam)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "ConfigureMuxer enter", avRecorderTag_.c_str());
    switch (recParam.type) {
        case RecorderPublicParamType::OUT_FD: {
            OutFd outFd = static_cast<const OutFd&>(recParam);
            fd_ = dup(outFd.fd);
            muxerFormat_->Set<Tag::MEDIA_CREATION_TIME>("now");
            MEDIA_LOG_I(PUBLIC_LOG_S "ConfigureMuxer enter " PUBLIC_LOG_D32, avRecorderTag_.c_str(), fd_);
            break;
        }
        case RecorderPublicParamType::MAX_DURATION: {
            MaxDuration maxDuration = static_cast<const MaxDuration&>(recParam);
            maxDuration_ = maxDuration.duration;
            break;
        }
        case RecorderPublicParamType::MAX_SIZE: {
            MaxFileSize maxFileSize = static_cast<const MaxFileSize&>(recParam);
            maxSize_ = maxFileSize.size;
            break;
        }
        case RecorderPublicParamType::VID_ORIENTATION_HINT: {
            RotationAngle rotationAngle = static_cast<const RotationAngle&>(recParam);
            if (rotationAngle.rotation == Plugins::VideoRotation::VIDEO_ROTATION_0) {
                muxerFormat_->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_0);
            } else if (rotationAngle.rotation == Plugins::VideoRotation::VIDEO_ROTATION_90) {
                muxerFormat_->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_90);
            } else if (rotationAngle.rotation == Plugins::VideoRotation::VIDEO_ROTATION_180) {
                muxerFormat_->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_180);
            } else if (rotationAngle.rotation == Plugins::VideoRotation::VIDEO_ROTATION_270) {
                muxerFormat_->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_270);
            }
            if (muxerFilter_) {
                muxerFilter_->SetParameter(muxerFormat_);
            }
            break;
        }
        case RecorderPublicParamType::GEO_LOCATION: {
            GeoLocation geoLocation = static_cast<const GeoLocation&>(recParam);
            muxerFormat_->Set<Tag::MEDIA_LATITUDE>(geoLocation.latitude);
            muxerFormat_->Set<Tag::MEDIA_LONGITUDE>(geoLocation.longitude);
            break;
        }
        default:
            break;
    }
}

bool HiRecorderImpl::CheckParamType(int32_t sourceId, const RecorderParam &recParam)
{
    FALSE_RETURN_V((SourceIdGenerator::IsAudio(sourceId) && recParam.IsAudioParam() &&
        static_cast<int32_t>(audioSourceId_) == sourceId) ||
        (SourceIdGenerator::IsVideo(sourceId) && recParam.IsVideoParam() &&
        static_cast<int32_t>(videoSourceId_) == sourceId) ||
        ((sourceId == DUMMY_SOURCE_ID) && !(recParam.IsAudioParam() || recParam.IsVideoParam())), false);
    return true;
}

void HiRecorderImpl::OnStateChanged(StateId state)
{
    curState_ = state;
}

/**
 * CapturerChangeInfo to AudioRecorderChangeInfo
*/
AudioRecorderChangeInfo HiRecorderImpl::ConvertCapturerChangeInfo(
    const AudioStandard::AudioCapturerChangeInfo &capturerChangeInfo)
{
    AudioRecorderChangeInfo audioRecorderChangeInfo;
    audioRecorderChangeInfo.createrUID = capturerChangeInfo.createrUID;
    audioRecorderChangeInfo.clientUID = capturerChangeInfo.clientUID;
    audioRecorderChangeInfo.clientPid = capturerChangeInfo.clientPid;
    audioRecorderChangeInfo.sessionId = capturerChangeInfo.sessionId;
    audioRecorderChangeInfo.capturerState = capturerChangeInfo.capturerState;

    audioRecorderChangeInfo.capturerInfo.sourceType = capturerChangeInfo.capturerInfo.sourceType;
    audioRecorderChangeInfo.capturerInfo.capturerFlags = capturerChangeInfo.capturerInfo.capturerFlags;

    audioRecorderChangeInfo.inputDeviceInfo.deviceName = capturerChangeInfo.inputDeviceInfo.deviceName;
    audioRecorderChangeInfo.inputDeviceInfo.deviceId = capturerChangeInfo.inputDeviceInfo.deviceId;
    audioRecorderChangeInfo.inputDeviceInfo.channelMasks = capturerChangeInfo.inputDeviceInfo.channelMasks;
    audioRecorderChangeInfo.inputDeviceInfo.deviceRole = capturerChangeInfo.inputDeviceInfo.deviceRole;
    audioRecorderChangeInfo.inputDeviceInfo.deviceType = capturerChangeInfo.inputDeviceInfo.deviceType;
    audioRecorderChangeInfo.inputDeviceInfo.displayName = capturerChangeInfo.inputDeviceInfo.displayName;
    audioRecorderChangeInfo.inputDeviceInfo.interruptGroupId =
        capturerChangeInfo.inputDeviceInfo.interruptGroupId;
    audioRecorderChangeInfo.inputDeviceInfo.isLowLatencyDevice =
        capturerChangeInfo.inputDeviceInfo.isLowLatencyDevice;
    audioRecorderChangeInfo.inputDeviceInfo.macAddress = capturerChangeInfo.inputDeviceInfo.macAddress;
    audioRecorderChangeInfo.inputDeviceInfo.channelIndexMasks =
        capturerChangeInfo.inputDeviceInfo.channelIndexMasks;
    for (auto item : capturerChangeInfo.inputDeviceInfo.audioStreamInfo.channels) {
        audioRecorderChangeInfo.inputDeviceInfo.audioStreamInfo.channels.insert(static_cast<int32_t>(item));
    }
    for (auto item : capturerChangeInfo.inputDeviceInfo.audioStreamInfo.samplingRate) {
        audioRecorderChangeInfo.inputDeviceInfo.audioStreamInfo.samplingRate.insert(static_cast<int32_t>(item));
    }
    audioRecorderChangeInfo.inputDeviceInfo.audioStreamInfo.encoding =
        capturerChangeInfo.inputDeviceInfo.audioStreamInfo.encoding;
    audioRecorderChangeInfo.inputDeviceInfo.audioStreamInfo.format =
        capturerChangeInfo.inputDeviceInfo.audioStreamInfo.format;
    return audioRecorderChangeInfo;
}

EncoderCapabilityData HiRecorderImpl::ConvertVideoEncoderInfo(MediaAVCodec::CapabilityData *capabilityData)
{
    EncoderCapabilityData encoderInfo;
    encoderInfo.mimeType = capabilityData->mimeType;
    encoderInfo.type = VIDEO_TYPE;
    encoderInfo.bitrate = CodecRange(capabilityData->bitrate.minVal, capabilityData->bitrate.maxVal);
    encoderInfo.frameRate = CodecRange(capabilityData->frameRate.minVal, capabilityData->frameRate.maxVal);
    encoderInfo.width = CodecRange(capabilityData->width.minVal, capabilityData->width.maxVal);
    encoderInfo.height = CodecRange(capabilityData->height.minVal, capabilityData->height.maxVal);
    encoderInfo.channels = CodecRange();
    encoderInfo.sampleRate = std::vector<int32_t>();
    return encoderInfo;
}

EncoderCapabilityData HiRecorderImpl::ConvertAudioEncoderInfo(MediaAVCodec::CapabilityData *capabilityData)
{
    EncoderCapabilityData encoderInfo;
    encoderInfo.mimeType = capabilityData->mimeType;
    encoderInfo.type = AUIDO_TYPE;
    encoderInfo.bitrate = CodecRange(capabilityData->bitrate.minVal, capabilityData->bitrate.maxVal);
    encoderInfo.frameRate = CodecRange();
    encoderInfo.width = CodecRange();
    encoderInfo.height = CodecRange();
    encoderInfo.channels = CodecRange(capabilityData->channels.minVal, capabilityData->channels.maxVal);
    encoderInfo.sampleRate = capabilityData->sampleRate;
    return encoderInfo;
}

std::vector<EncoderCapabilityData> HiRecorderImpl::ConvertEncoderInfo(
    std::vector<MediaAVCodec::CapabilityData*> &capData)
{
    std::vector<EncoderCapabilityData> encoderInfoVector;
    for (int32_t i = 0; i < (int32_t)capData.size(); i++) {
        EncoderCapabilityData encoderInfo;
        if (capData[i]->codecType == MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER) {
            encoderInfo = ConvertVideoEncoderInfo(capData[i]);
            encoderInfoVector.push_back(encoderInfo);
        } else if (capData[i]->codecType == MediaAVCodec::AVCodecType::AVCODEC_TYPE_AUDIO_ENCODER) {
            encoderInfo = ConvertAudioEncoderInfo(capData[i]);
            encoderInfoVector.push_back(encoderInfo);
        } else {
            MEDIA_LOG_W("codecType is not encoder, skip convert");
        }
    }
    return encoderInfoVector;
}
} // namespace MEDIA
} // namespace OHOS
