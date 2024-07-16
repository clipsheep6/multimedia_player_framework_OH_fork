/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "hitranscoder_impl.h"
#include "sync_fence.h"
#include <sys/syscall.h>
#include "directory_ex.h"
#include "osal/task/jobutils.h"
#include "media_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "HiTransCoder" };
}

namespace OHOS {
namespace Media {
constexpr int32_t REPORT_PROGRESS_INTERVAL = 100;
constexpr int32_t TRANSCODER_COMPLETE_PROGRESS = 100;
class TransCoderEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TransCoderEventReceiver(HiTransCoderImpl *hiTransCoderImpl)
    {
        hiTransCoderImpl_ = hiTransCoderImpl;
    }

    void OnEvent(const Event &event)
    {
        hiTransCoderImpl_->OnEvent(event);
    }

private:
    HiTransCoderImpl *hiTransCoderImpl_;
};

class TransCoderFilterCallback : public Pipeline::FilterCallback {
public:
    explicit TransCoderFilterCallback(HiTransCoderImpl *hiTransCoderImpl)
    {
        hiTransCoderImpl_ = hiTransCoderImpl;
    }

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType)
    {
        hiTransCoderImpl_->OnCallback(filter, cmd, outType);
        return Status::OK;
    }

private:
    HiTransCoderImpl *hiTransCoderImpl_;
};

HiTransCoderImpl::HiTransCoderImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
    : appUid_(appUid), appPid_(appPid), appTokenId_(appTokenId), appFullTokenId_(appFullTokenId)
{
    MEDIA_LOG_I("HiTransCoderImpl");
    pipeline_ = std::make_shared<Pipeline::Pipeline>();
    transCoderId_ = std::string("Trans_") + std::to_string(OHOS::Media::Pipeline::Pipeline::GetNextPipelineId());
}

HiTransCoderImpl::~HiTransCoderImpl()
{
    MEDIA_LOG_I("~HiTransCoderImpl");
}

int32_t HiTransCoderImpl::Init()
{
    MEDIA_LOG_I("HiTransCoderImpl::Init()");
    transCoderEventReceiver_ = std::make_shared<TransCoderEventReceiver>(this);
    transCoderFilterCallback_ = std::make_shared<TransCoderFilterCallback>(this);
    pipeline_->Init(transCoderEventReceiver_, transCoderFilterCallback_, transCoderId_);
    callbackLooper_ = std::make_shared<HiTransCoderCallbackLooper>();
    callbackLooper_->SetTransCoderEngine(this, transCoderId_);
    return static_cast<int32_t>(Status::OK);
}

int32_t HiTransCoderImpl::GetRealPath(const std::string &url, std::string &realUrlPath) const
{
    std::string fileHead = "file://";
    std::string tempUrlPath;

    if (url.find(fileHead) == 0 && url.size() > fileHead.size()) {
        tempUrlPath = url.substr(fileHead.size());
    } else {
        tempUrlPath = url;
    }
    if (tempUrlPath.find("..") != std::string::npos) {
        MEDIA_LOG_E("invalid url. The Url (%{private}s) path may be invalid.", tempUrlPath.c_str());
        return MSERR_FILE_ACCESS_FAILED;
    }
    bool ret = PathToRealPath(tempUrlPath, realUrlPath);
    if (!ret) {
        MEDIA_LOG_E("invalid url. The Url (%{private}s) path may be invalid.", url.c_str());
        return MSERR_OPEN_FILE_FAILED;
    }
    if (access(realUrlPath.c_str(), R_OK) != 0) {
        return MSERR_FILE_ACCESS_FAILED;
    }
    return MSERR_OK;
}

int32_t HiTransCoderImpl::SetInputFile(const std::string &url)
{
    inputFile_ = url;
    if (url.find("://") == std::string::npos || url.find("file://") == 0) {
        std::string realUriPath;
        int32_t result = GetRealPath(url, realUriPath);
        if (result != MSERR_OK) {
            MEDIA_LOG_E("SetInputFile error: GetRealPath error");
            return result;
        }
        inputFile_ = "file://" + realUriPath;
    }
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(inputFile_);
    demuxerFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::DemuxerFilter>("builtin.player.demuxer",
        Pipeline::FilterType::FILTERTYPE_DEMUXER);
    demuxerFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
    Status ret = demuxerFilter_->SetDataSource(mediaSource);
    if (ret != Status::OK) {
        MEDIA_LOG_E("SetInputFile error: demuxerFilter_->SetDataSource error");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_UNSUPPORT_SOURCE});
        return static_cast<int32_t>(ret);
    }
    int64_t duration = 0;
    if (demuxerFilter_->GetDuration(duration)) {
        durationMs_ = Plugins::HstTime2Us(duration);
    } else {
        MEDIA_LOG_E("Get media duration failed");
    }
    ret = ConfigureVideoAudioMetaData();
    if (ret != Status::OK) {
        return static_cast<int32_t>(ret);
    }
    pipeline_->AddHeadFilters({demuxerFilter_});
    return static_cast<int32_t>(ret);
}

Status HiTransCoderImpl::ConfigureVideoAudioMetaData()
{
    if (demuxerFilter_ == nullptr) {
        MEDIA_LOG_E("demuxerFilter_ is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxerFilter_->GetStreamMetaInfo();
    size_t trackCount = trackInfos.size();
    MEDIA_LOG_I("trackCount: %{public}d", trackCount);
    if (trackCount == 0) {
        MEDIA_LOG_E("No track found in the source");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_DEMUXER_FAILED});
        return Status::ERROR_INVALID_PARAMETER;
    }
    for (size_t index = 0; index < trackCount; index++) {
        std::string trackMime;
        if (!trackInfos[index]->GetData(Tag::MIME_TYPE, trackMime)) {
            MEDIA_LOG_E("trackInfos index: %{public}d, get trackMime failed", index);
            OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_UNKNOWN});
            return Status::ERROR_UNKNOWN;
        }
        if (trackMime.find("video/") == 0) {
            MEDIA_LOG_I("SetInputFile contain video");
            if (trackInfos[index]->GetData(Tag::VIDEO_WIDTH, inputVideoWidth_) &&
                trackInfos[index]->GetData(Tag::VIDEO_HEIGHT, inputVideoHeight_)) {
                MEDIA_LOG_D("inputVideoWidth_: %{public}d, inputVideoHeight_: %{public}d",
                    inputVideoWidth_, inputVideoHeight_);
            } else {
                MEDIA_LOG_W("Get input video width or height failed");
            }
        } else if (trackMime.find("audio/") == 0) {
            MEDIA_LOG_I("SetInputFile contain audio");
            int32_t channels;
            if (trackInfos[index]->GetData(Tag::AUDIO_CHANNEL_COUNT, channels)) {
                MEDIA_LOG_D("Audio channel count: %{public}d", channels);
            } else {
                MEDIA_LOG_W("Get audio channel count failed");
            }
            audioEncFormat_->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
            audioEncFormat_->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
            int32_t sampleRate;
            if (trackInfos[index]->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate)) {
                MEDIA_LOG_D("Audio sampleRate: %{public}d", sampleRate);
            } else {
                MEDIA_LOG_W("Get audio channel count failed");
            }
            audioEncFormat_->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
        }
    }
    return Status::OK;
}

int32_t HiTransCoderImpl::SetOutputFile(const int32_t fd)
{
    MEDIA_LOG_I("HiTransCoderImpl::SetOutputFile()");
    fd_ = dup(fd);
    return static_cast<int32_t>(Status::OK);
}

int32_t HiTransCoderImpl::SetOutputFormat(OutputFormatType format)
{
    MEDIA_LOG_I("HiTransCoderImpl::SetOutputFormat()");
    outputFormatType_ = format;
    return static_cast<int32_t>(Status::OK);
}

int32_t HiTransCoderImpl::SetObs(const std::weak_ptr<ITransCoderEngineObs> &obs)
{
    MEDIA_LOG_I("HiTransCoderImpl::SetObs()");
    obs_ = obs;
    callbackLooper_->StartWithTransCoderEngineObs(obs);
    return static_cast<int32_t>(Status::OK);
}

Status HiTransCoderImpl::ConfigureVideoEncoderFormat(const TransCoderParam &transCoderParam)
{
    VideoEnc videoEnc = static_cast<const VideoEnc&>(transCoderParam);
    MEDIA_LOG_I("HiTransCoderImpl::Configure videoEnc %{public}d", videoEnc.encFmt);
    switch (videoEnc.encFmt) {
        case OHOS::Media::VideoCodecFormat::H264:
            videoEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
            videoEncFormat_->Set<Tag::VIDEO_H264_PROFILE>(Plugins::VideoH264Profile::BASELINE);
            videoEncFormat_->Set<Tag::VIDEO_H264_LEVEL>(32); // 32: LEVEL 3.2
            break;
        case OHOS::Media::VideoCodecFormat::MPEG4:
            videoEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
            break;
        case OHOS::Media::VideoCodecFormat::H265:
            videoEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_HEVC);
            break;
        default:
            MEDIA_LOG_I("ConfigureVideo %{public}d not supported", videoEnc.encFmt);
            OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
            return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

Status HiTransCoderImpl::ConfigureVideoWidthHeight(const TransCoderParam &transCoderParam)
{
    VideoRectangle videoRectangle = static_cast<const VideoRectangle&>(transCoderParam);
    if (videoRectangle.width <= 0 || videoRectangle.height <= 0) {
        MEDIA_LOG_E("Invalid videoRectangle.width %{public}d, videoRectangle.height %{public}d",
            videoRectangle.width, videoRectangle.height);
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
        return Status::ERROR_INVALID_PARAMETER;
    }
    MEDIA_LOG_I("videoRectangle.width %{public}d, videoRectangle.height %{public}d",
        videoRectangle.width, videoRectangle.height);
    videoEncFormat_->Set<Tag::VIDEO_WIDTH>(videoRectangle.width);
    videoEncFormat_->Set<Tag::VIDEO_HEIGHT>(videoRectangle.height);
    return Status::OK;
}

int32_t HiTransCoderImpl::Configure(const TransCoderParam &transCoderParam)
{
    MEDIA_LOG_I("HiTransCoderImpl::Configure()");
    Status ret = Status::OK;
    switch (transCoderParam.type) {
        case TransCoderPublicParamType::VIDEO_ENC_FMT: {
            ret = ConfigureVideoEncoderFormat(transCoderParam);
            break;
        }
        case TransCoderPublicParamType::VIDEO_RECTANGLE: {
            ret = ConfigureVideoWidthHeight(transCoderParam);
            break;
        }
        case TransCoderPublicParamType::VIDEO_BITRATE: {
            VideoBitRate videoBitrate = static_cast<const VideoBitRate&>(transCoderParam);
            if (videoBitrate.bitRate <= 0) {
                MEDIA_LOG_E("Invalid videoBitrate.bitRate %{public}d", videoBitrate.bitRate);
                OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
                return static_cast<int32_t>(Status::ERROR_INVALID_PARAMETER);
            }
            MEDIA_LOG_I("HiTransCoderImpl::Configure videoBitRate %{public}d", videoBitrate.bitRate);
            videoEncFormat_->Set<Tag::MEDIA_BITRATE>(videoBitrate.bitRate);
            break;
        }
        case TransCoderPublicParamType::AUDIO_ENC_FMT: {
            AudioEnc audioEnc = static_cast<const AudioEnc&>(transCoderParam);
            MEDIA_LOG_I("HiTransCoderImpl::Configure audioEnc %{public}d", audioEnc.encFmt);
            audioEncFormat_->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
            break;
        }
        case TransCoderPublicParamType::AUDIO_BITRATE: {
            AudioBitRate audioBitrate = static_cast<const AudioBitRate&>(transCoderParam);
            if (audioBitrate.bitRate <= 0) {
                MEDIA_LOG_E("Invalid audioBitrate.bitRate %{public}d", audioBitrate.bitRate);
                OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
                return static_cast<int32_t>(Status::ERROR_INVALID_PARAMETER);
            }
            MEDIA_LOG_I("HiTransCoderImpl::Configure audioBitrate %{public}d", audioBitrate.bitRate);
            audioEncFormat_->Set<Tag::MEDIA_BITRATE>(audioBitrate.bitRate);
            break;
        }
        default:
            break;
    }
    return static_cast<int32_t>(ret);
}

int32_t HiTransCoderImpl::Prepare()
{
    MEDIA_LOG_I("HiTransCoderImpl::Prepare()");
    int32_t width = 0;
    int32_t height = 0;
    if (videoEncFormat_->GetData(Tag::VIDEO_WIDTH, width) &&
        videoEncFormat_->GetData(Tag::VIDEO_HEIGHT, height)) {
        MEDIA_LOG_D("set output video width: %{public}d, height: %{public}d", width, height);
    } else {
        MEDIA_LOG_E("Output video width or height not set");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
        return static_cast<int32_t>(Status::ERROR_INVALID_PARAMETER);
    }
    isNeedVideoResizeFilter_ = width != inputVideoWidth_ || height != inputVideoHeight_;
    Status ret = pipeline_->Prepare();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Prepare failed with error " PUBLIC_LOG_D32, ret);
        auto errCode = TransStatus(ret);
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, errCode});
        return static_cast<int32_t>(errCode);
    }
    if ((videoEncoderFilter_ != nullptr) && (videoDecoderFilter_ != nullptr)) {
        if (isNeedVideoResizeFilter_ && (videoResizeFilter_ != nullptr)) {
            sptr<Surface> resizeFilterSurface = videoResizeFilter_->GetInputSurface();
            FALSE_RETURN_V_MSG_E(resizeFilterSurface != nullptr,
                static_cast<int32_t>(Status::ERROR_NULL_POINTER), "resizeFilterSurface is nullptr");
            videoDecoderFilter_->SetOutputSurface(resizeFilterSurface);
            sptr<Surface> encoderFilterSurface = videoEncoderFilter_->GetInputSurface();
            FALSE_RETURN_V_MSG_E(encoderFilterSurface != nullptr,
                static_cast<int32_t>(Status::ERROR_NULL_POINTER), "encoderFilterSurface is nullptr");
            videoResizeFilter_->SetOutputSurface(encoderFilterSurface, width, height);
        } else {
            sptr<Surface> encoderFilterSurface = videoEncoderFilter_->GetInputSurface();
            FALSE_RETURN_V_MSG_E(encoderFilterSurface != nullptr,
                static_cast<int32_t>(Status::ERROR_NULL_POINTER), "encoderFilterSurface is nullptr");
            videoDecoderFilter_->SetOutputSurface(encoderFilterSurface);
        }
    }
    return static_cast<int32_t>(ret);
}

int32_t HiTransCoderImpl::Start()
{
    MEDIA_LOG_I("HiTransCoderImpl::Start()");
    int32_t ret = TransStatus(pipeline_->Start());
    if (ret != MSERR_OK) {
        MEDIA_LOG_E("Start pipeline failed");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, ret});
        return ret;
    }
    callbackLooper_->StartReportMediaProgress(REPORT_PROGRESS_INTERVAL);
    return ret;
}

int32_t HiTransCoderImpl::Pause()
{
    MEDIA_LOG_I("HiTransCoderImpl::Pause()");
    callbackLooper_->StopReportMediaProgress();
    Status ret = pipeline_->Pause();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Pause pipeline failed");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_UNKNOWN});
    }
    return static_cast<int32_t>(ret);
}

int32_t HiTransCoderImpl::Resume()
{
    MEDIA_LOG_I("HiTransCoderImpl::Resume()");
    Status ret = pipeline_->Resume();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Resume pipeline failed");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        return static_cast<int32_t>(ret);
    }
    callbackLooper_->StartReportMediaProgress(REPORT_PROGRESS_INTERVAL);
    return static_cast<int32_t>(ret);
}

int32_t HiTransCoderImpl::Cancel()
{
    MEDIA_LOG_I("HiTransCoderImpl::Cancel enter");
    callbackLooper_->StopReportMediaProgress();
    Status ret = pipeline_->Stop();
    callbackLooper_->Stop();
    if (ret != Status::OK) {
        MEDIA_LOG_E("Stop pipeline failed");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_UNKNOWN});
        return static_cast<int32_t>(ret);
    }
    MEDIA_LOG_I("HiTransCoderImpl::Cancel done");
    return static_cast<int32_t>(ret);
}

void HiTransCoderImpl::OnEvent(const Event &event)
{
    switch (event.type) {
        case EventType::EVENT_ERROR: {
            HandleErrorEvent(AnyCast<int32_t>(event.param));
            break;
        }
        case EventType::EVENT_COMPLETE: {
            MEDIA_LOG_I("HiTransCoderImpl EVENT_COMPLETE");
            cancelTask_ = std::make_shared<Task>("CancelTransCoder", "",
                TaskType::SINGLETON, TaskPriority::NORMAL, false);
            cancelTask_->SubmitJobOnce([this]() {
                Cancel();
                auto ptr = obs_.lock();
                if (ptr != nullptr) {
                    ptr->OnInfo(TransCoderOnInfoType::INFO_TYPE_PROGRESS_UPDATE, TRANSCODER_COMPLETE_PROGRESS);
                    ptr->OnInfo(TransCoderOnInfoType::INFO_TYPE_TRANSCODER_COMPLETED, 0);
                }
            });
        }
        default:
            break;
    }
}

void HiTransCoderImpl::HandleErrorEvent(int32_t errorCode)
{
    callbackLooper_->OnError(TRANSCODER_ERROR_INTERNAL, errorCode);
}

Status HiTransCoderImpl::LinkAudioDecoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter,
    Pipeline::StreamType type)
{
    MEDIA_LOG_I("HiTransCoderImpl::LinkAudioDecoderFilter()");
    audioDecoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::AudioDecoderFilter>(
        "audioDecoderFilter", Pipeline::FilterType::FILTERTYPE_ADEC);
    FALSE_RETURN_V_MSG_E(audioDecoderFilter_ != nullptr, Status::ERROR_NULL_POINTER,
        "audioDecoderFilter is nullptr");
    audioDecoderFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
    FALSE_RETURN_V_MSG_E(pipeline_->LinkFilters(preFilter, {audioDecoderFilter_}, type) == Status::OK,
        Status::ERROR_UNKNOWN, "Add audioDecoderFilter to pipeline fail");
    return Status::OK;
}

Status HiTransCoderImpl::LinkAudioEncoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter,
    Pipeline::StreamType type)
{
    MEDIA_LOG_I("HiTransCoderImpl::LinkAudioEncoderFilter()");
    audioEncoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::AudioEncoderFilter>
        ("audioEncoderFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    FALSE_RETURN_V_MSG_E(audioEncoderFilter_ != nullptr, Status::ERROR_NULL_POINTER,
        "audioEncoderFilter is nullptr");
    audioEncFormat_->Set<Tag::APP_TOKEN_ID>(appTokenId_);
    audioEncFormat_->Set<Tag::APP_UID>(appUid_);
    audioEncFormat_->Set<Tag::APP_PID>(appPid_);
    audioEncFormat_->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
    FALSE_RETURN_V_MSG_E(audioEncoderFilter_->SetCodecFormat(audioEncFormat_) == Status::OK,
        Status::ERROR_UNKNOWN, "audioEncoderFilter SetCodecFormat fail");
    audioEncoderFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
    FALSE_RETURN_V_MSG_E(audioEncoderFilter_->Configure(audioEncFormat_) == Status::OK,
        Status::ERROR_UNKNOWN, "audioEncoderFilter Configure fail");
    FALSE_RETURN_V_MSG_E(pipeline_->LinkFilters(preFilter, {audioEncoderFilter_}, type) == Status::OK,
        Status::ERROR_UNKNOWN, "Add audioEncoderFilter to pipeline fail");
    return Status::OK;
}

Status HiTransCoderImpl::LinkVideoDecoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter,
    Pipeline::StreamType type)
{
    MEDIA_LOG_I("HiTransCoderImpl::LinkVideoDecoderFilter()");
    videoDecoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::SurfaceDecoderFilter>(
        "surfacedecoder", Pipeline::FilterType::FILTERTYPE_VIDEODEC);
    FALSE_RETURN_V_MSG_E(videoDecoderFilter_ != nullptr, Status::ERROR_NULL_POINTER,
        "videoDecoderFilter is nullptr");
    videoDecoderFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
    FALSE_RETURN_V_MSG_E(pipeline_->LinkFilters(preFilter, {videoDecoderFilter_}, type) == Status::OK,
        Status::ERROR_UNKNOWN, "Add videoDecoderFilter_ to pipeline fail");
    return Status::OK;
}

Status HiTransCoderImpl::LinkVideoEncoderFilter(const std::shared_ptr<Pipeline::Filter>& preFilter,
    Pipeline::StreamType type)
{
    MEDIA_LOG_I("HiTransCoderImpl::LinkVideoEncoderFilter()");
    videoEncoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::SurfaceEncoderFilter>
        ("videoEncoderFilter", Pipeline::FilterType::FILTERTYPE_VENC);
    FALSE_RETURN_V_MSG_E(videoEncoderFilter_ != nullptr, Status::ERROR_NULL_POINTER,
        "videoEncoderFilter is nullptr");
    FALSE_RETURN_V_MSG_E(videoEncoderFilter_->SetCodecFormat(videoEncFormat_) == Status::OK,
        Status::ERROR_UNKNOWN, "videoEncoderFilter SetCodecFormat fail");
    videoEncoderFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
    FALSE_RETURN_V_MSG_E(videoEncoderFilter_->Configure(videoEncFormat_) == Status::OK,
        Status::ERROR_UNKNOWN, "videoEncoderFilter Configure fail");
    FALSE_RETURN_V_MSG_E(videoEncoderFilter_->SetTransCoderMode() == Status::OK,
        Status::ERROR_UNKNOWN, "videoEncoderFilter SetTransCoderMode fail");
    FALSE_RETURN_V_MSG_E(pipeline_->LinkFilters(preFilter, {videoEncoderFilter_}, type) == Status::OK,
        Status::ERROR_UNKNOWN, "Add videoEncoderFilter to pipeline fail");
    return Status::OK;
}

Status HiTransCoderImpl::LinkVideoResizeFilter(const std::shared_ptr<Pipeline::Filter>& preFilter,
    Pipeline::StreamType type)
{
    MEDIA_LOG_I("HiTransCoderImpl::LinkVideoResizeFilter()");
    videoResizeFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::VideoResizeFilter>
        ("videoResizeFilter", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    FALSE_RETURN_V_MSG_E(videoResizeFilter_ != nullptr, Status::ERROR_NULL_POINTER,
        "videoResizeFilter_ is nullptr");
    videoResizeFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
    FALSE_RETURN_V_MSG_E(videoResizeFilter_->Configure(videoEncFormat_) == Status::OK,
        Status::ERROR_UNKNOWN, "videoEncoderFilter Configure fail");
    FALSE_RETURN_V_MSG_E(pipeline_->LinkFilters(preFilter, {videoResizeFilter_}, type) == Status::OK,
        Status::ERROR_UNKNOWN, "Add videoResizeFilter to pipeline fail");
    return Status::OK;
}

Status HiTransCoderImpl::LinkMuxerFilter(const std::shared_ptr<Pipeline::Filter>& preFilter,
    Pipeline::StreamType type)
{
    MEDIA_LOG_I("HiTransCoderImpl::LinkMuxerFilter()");
    if (muxerFilter_ == nullptr) {
        muxerFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::MuxerFilter>
            ("muxerFilter", Pipeline::FilterType::FILTERTYPE_MUXER);
        FALSE_RETURN_V_MSG_E(muxerFilter_ != nullptr, Status::ERROR_NULL_POINTER,
            "muxerFilter is nullptr");
        muxerFilter_->Init(transCoderEventReceiver_, transCoderFilterCallback_);
        FALSE_RETURN_V_MSG_E(muxerFilter_->SetOutputParameter(appUid_, appPid_, fd_, outputFormatType_) == Status::OK,
            Status::ERROR_UNKNOWN, "muxerFilter SetOutputParameter fail");
        muxerFilter_->SetParameter(muxerFormat_);
        muxerFilter_->SetTransCoderMode();
        close(fd_);
        fd_ = -1;
    }
    FALSE_RETURN_V_MSG_E(pipeline_->LinkFilters(preFilter, {muxerFilter_}, type) == Status::OK,
        Status::ERROR_UNKNOWN, "Add muxerFilter to pipeline fail");
    return Status::OK;
}

void HiTransCoderImpl::OnCallback(std::shared_ptr<Pipeline::Filter> filter, const Pipeline::FilterCallBackCommand cmd,
    Pipeline::StreamType outType)
{
    if (cmd == Pipeline::FilterCallBackCommand::NEXT_FILTER_NEEDED) {
        switch (outType) {
            case Pipeline::StreamType::STREAMTYPE_RAW_AUDIO:
                LinkAudioEncoderFilter(filter, outType);
                break;
            case Pipeline::StreamType::STREAMTYPE_ENCODED_AUDIO:
                if (audioDecoderFilter_) {
                    LinkMuxerFilter(filter, outType);
                } else {
                    LinkAudioDecoderFilter(filter, outType);
                }
                break;
            case Pipeline::StreamType::STREAMTYPE_RAW_VIDEO:
                if (!isNeedVideoResizeFilter_ || videoResizeFilter_) {
                    LinkVideoEncoderFilter(filter, outType);
                } else {
                    LinkVideoResizeFilter(filter, outType);
                }
                break;
            case Pipeline::StreamType::STREAMTYPE_ENCODED_VIDEO:
                if (videoDecoderFilter_) {
                    LinkMuxerFilter(filter, outType);
                } else {
                    LinkVideoDecoderFilter(filter, outType);
                }
                break;
            default:
                break;
        }
    }
}

int32_t HiTransCoderImpl::GetCurrentTime(int32_t& currentPositionMs)
{
    int64_t currentPts = muxerFilter_->GetCurrentPtsMs();
    currentPositionMs = (int32_t)currentPts;
    return static_cast<int32_t>(Status::OK);
}

int32_t HiTransCoderImpl::GetDuration(int32_t& durationMs)
{
    durationMs = durationMs_.load();
    return static_cast<int32_t>(Status::OK);
}
} // namespace MEDIA
} // namespace OHOS
