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
#include "meta/video_types.h"
#include "meta/any.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER, "HiTransCoder" };
}

namespace OHOS {
namespace Media {
constexpr int32_t REPORT_PROGRESS_INTERVAL = 100;
constexpr int32_t TRANSCODER_COMPLETE_PROGRESS = 100;
constexpr int8_t VIDEO_HDR_TYPE_NONE = 0; // This option is used to mark none HDR type.
constexpr int8_t VIDEO_HDR_TYPE_VIVID = 1; // This option is used to mark HDR Vivid type.
constexpr int32_t MINIMUM_WIDTH_HEIGHT = 240;

static const std::unordered_set<std::string> AVMETA_KEY = {
    { Tag::MEDIA_ALBUM },
    { Tag::MEDIA_ALBUM_ARTIST },
    { Tag::MEDIA_ARTIST },
    { Tag::MEDIA_AUTHOR },
    { Tag::MEDIA_COMPOSER },
    { Tag::MEDIA_DATE },
    { Tag::MEDIA_CREATION_TIME },
    { Tag::MEDIA_DURATION },
    { Tag::MEDIA_GENRE },
    { Tag::MIME_TYPE },
    { Tag::AUDIO_SAMPLE_RATE },
    { Tag::MEDIA_TITLE },
    { Tag::VIDEO_HEIGHT },
    { Tag::VIDEO_WIDTH },
    { Tag::VIDEO_ROTATION },
    { Tag::VIDEO_IS_HDR_VIVID },
    { Tag::MEDIA_LONGITUDE },
    { Tag::MEDIA_LATITUDE },
    { "customInfo" },
};

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

void HiTransCoderImpl::ConfigureMetaDataToTrackFormat(const std::shared_ptr<Meta> &globalInfo,
    const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    FALSE_RETURN_MSG(
        globalInfo != nullptr && trackInfos.size() != 0, "globalInfo or trackInfos are invalid.");
    
    (void)SetValueByType(globalInfo, muxerFormat_);

    for (size_t index = 0; index < trackInfos.size(); index++) {
        MEDIA_LOG_I("trackInfos index: %{public}zu", index);
        std::shared_ptr<Meta> meta = trackInfos[index];
        FALSE_RETURN_MSG(meta != nullptr, "meta is invalid, index: %zu", index);
        Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_W("mediaType not found, index: %zu", index);
            continue;
        }
        (void)SetValueByType(meta, muxerFormat_);
        if (mediaType == Plugins::MediaType::VIDEO) {
            (void)SetValueByType(meta, videoEncFormat_);
        } else if (mediaType == Plugins::MediaType::AUDIO) {
            (void)SetValueByType(meta, audioEncFormat_);
        }
    }
}

bool HiTransCoderImpl::SetValueByType(const std::shared_ptr<Meta> &innerMeta, std::shared_ptr<Meta> &outputMeta)
{
    if (innerMeta == nullptr || outputMeta == nullptr) {
        return false;
    }
    for (const auto &metaKey : AVMETA_KEY) {
        bool isSetData = false;
        Any type = OHOS::Media::GetDefaultAnyValue(metaKey);
        if (Any::IsSameTypeWith<int32_t>(type)) {
            int32_t intVal;
            isSetData = !outputMeta->GetData(metaKey, intVal) && innerMeta->GetData(metaKey, intVal);
            if (isSetData) {
                outputMeta->SetData(metaKey, intVal);
            }
        } else if (Any::IsSameTypeWith<std::string>(type)) {
            std::string strVal;
            isSetData = !outputMeta->GetData(metaKey, strVal) && innerMeta->GetData(metaKey, strVal);
            if (isSetData) {
                outputMeta->SetData(metaKey, strVal);
            }
        } else if (Any::IsSameTypeWith<Plugins::VideoRotation>(type)) {
            Plugins::VideoRotation rotation;
            isSetData = !outputMeta->GetData(metaKey, rotation) && innerMeta->GetData(metaKey, rotation);
            if (isSetData) {
                outputMeta->SetData(metaKey, rotation);
            }
        } else if (Any::IsSameTypeWith<int64_t>(type)) {
            int64_t duration;
            isSetData = !outputMeta->GetData(metaKey, duration) && innerMeta->GetData(metaKey, duration);
            if (isSetData) {
                outputMeta->SetData(metaKey, duration);
            }
        } else if (Any::IsSameTypeWith<bool>(type)) {
            bool isTrue;
            isSetData = !outputMeta->GetData(metaKey, isTrue) && innerMeta->GetData(metaKey, isTrue);
            if (isSetData) {
                outputMeta->SetData(metaKey, isTrue);
            }
        } else if (Any::IsSameTypeWith<float>(type)) {
            float value;
            isSetData = !outputMeta->GetData(metaKey, value) && innerMeta->GetData(metaKey, value);
            if (isSetData) {
                outputMeta->SetData(metaKey, value);
            }
        }
    }
    return true;
}

Status HiTransCoderImpl::ConfigureVideoAudioMetaData()
{
    if (demuxerFilter_ == nullptr) {
        MEDIA_LOG_E("demuxerFilter_ is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    std::shared_ptr<Meta> globalInfo = demuxerFilter_->GetGlobalMetaInfo();
    std::vector<std::shared_ptr<Meta>> trackInfos = demuxerFilter_->GetStreamMetaInfo();
    size_t trackCount = trackInfos.size();
    MEDIA_LOG_I("trackCount: %{public}d", trackCount);
    if (trackCount == 0) {
        MEDIA_LOG_E("No track found in the source");
        OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_DEMUXER_FAILED});
        return Status::ERROR_INVALID_PARAMETER;
    }
    ConfigureMetaDataToTrackFormat(globalInfo, trackInfos);
    (void)ConfigureMetaData(trackInfos);
    (void)SetTrackMime(trackInfos);
    return Status::OK;
}

Status HiTransCoderImpl::ConfigureInputVideoMetaData(const std::vector<std::shared_ptr<Meta>> &trackInfos,
    const size_t &index)
{
    MEDIA_LOG_I("InputVideo contains videoTrack");
    isExistVideoTrack_ = true;
    Plugins::VideoRotation rotation = Plugins::VideoRotation::VIDEO_ROTATION_0;
    if (muxerFormat_ && trackInfos[index]->Get<Tag::VIDEO_ROTATION>(rotation)) {
        muxerFormat_->Set<Tag::VIDEO_ROTATION>(rotation);
    }
    if (trackInfos[index]->GetData(Tag::VIDEO_WIDTH, inputVideoWidth_) &&
        trackInfos[index]->GetData(Tag::VIDEO_HEIGHT, inputVideoHeight_)) {
        MEDIA_LOG_D("inputVideoWidth_: %{public}d, inputVideoHeight_: %{public}d",
            inputVideoWidth_, inputVideoHeight_);
        videoEncFormat_->Set<Tag::VIDEO_WIDTH>(inputVideoWidth_);
        videoEncFormat_->Set<Tag::VIDEO_HEIGHT>(inputVideoHeight_);
    } else {
        MEDIA_LOG_W("Get input video width or height failed");
    }
    bool isHdr = false;
    trackInfos[index]->GetData(Tag::VIDEO_IS_HDR_VIVID, isHdr);
    if (isHdr) {
        videoEncFormat_->SetData(Tag::VIDEO_IS_HDR_VIVID, VIDEO_HDR_TYPE_VIVID);
    } else {
        videoEncFormat_->SetData(Tag::VIDEO_IS_HDR_VIVID, VIDEO_HDR_TYPE_NONE);
    }
    return Status::OK;
}

Status HiTransCoderImpl::ConfigureMetaData(const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    for (size_t index = 0; index < trackInfos.size(); index++) {
        std::shared_ptr<Meta> meta = trackInfos[index];
        FALSE_RETURN_V_MSG_E(meta != nullptr, Status::ERROR_INVALID_PARAMETER,
            "meta is invalid, index: %zu", index);
        Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
        if (!meta->GetData(Tag::MEDIA_TYPE, mediaType)) {
            MEDIA_LOG_W("mediaType not found, index: %zu", index);
            continue;
        }
        if (mediaType == Plugins::MediaType::VIDEO) {
            (void)ConfigureInputVideoMetaData(trackInfos, index);
        } else if (mediaType == Plugins::MediaType::AUDIO) {
            int32_t channels = 0;
            if (trackInfos[index]->GetData(Tag::AUDIO_CHANNEL_COUNT, channels)) {
                MEDIA_LOG_D("Audio channel count: %{public}d", channels);
            } else {
                MEDIA_LOG_W("Get audio channel count failed");
            }
            audioEncFormat_->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
            audioEncFormat_->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
            int32_t sampleRate = 0;
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

Status HiTransCoderImpl::SetTrackMime(const std::vector<std::shared_ptr<Meta>> &trackInfos)
{
    for (size_t index = 0; index < trackInfos.size(); index++) {
        std::string trackMime;
        if (!trackInfos[index]->GetData(Tag::MIME_TYPE, trackMime)) {
            continue;
        }
        if (trackMime.find("video/") == 0) {
            videoEncFormat_->Set<Tag::MIME_TYPE>(trackMime);
        } else if (trackMime.find("audio/" == 0)) {
            audioEncFormat_->Set<Tag::MIME_TYPE>(trackMime);
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
            break;
    }
    return Status::OK;
}

Status HiTransCoderImpl::ConfigureVideoWidthHeight(const TransCoderParam &transCoderParam)
{
    VideoRectangle videoRectangle = static_cast<const VideoRectangle&>(transCoderParam);
    if (videoRectangle.width != -1) {
        videoEncFormat_->Set<Tag::VIDEO_WIDTH>(videoRectangle.width);
        }
    if (videoRectangle.height != -1) {
        videoEncFormat_->Set<Tag::VIDEO_HEIGHT>(videoRectangle.height);
        }
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
    if (isExistVideoTrack_) {
        if (videoEncFormat_->GetData(Tag::VIDEO_WIDTH, width) &&
            videoEncFormat_->GetData(Tag::VIDEO_HEIGHT, height)) {
            MEDIA_LOG_D("set output video width: %{public}d, height: %{public}d", width, height);
        } else {
            MEDIA_LOG_E("Output video width or height not set");
            OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
            return static_cast<int32_t>(Status::ERROR_INVALID_PARAMETER);
        }
        if (width > inputVideoWidth_ || height > inputVideoHeight_ || std::min(width, height) < MINIMUM_WIDTH_HEIGHT) {
            MEDIA_LOG_E("Output video width or height is invalid");
            OnEvent({"TranscoderEngine", EventType::EVENT_ERROR, MSERR_INVALID_VAL});
            return static_cast<int32_t>(Status::ERROR_INVALID_PARAMETER);
        }
        isNeedVideoResizeFilter_ = width != inputVideoWidth_ || height != inputVideoHeight_;
    }
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
            pauseTask_ = std::make_shared<Task>("PauseTransCoder", "",
                TaskType::SINGLETON, TaskPriority::NORMAL, false);
            pauseTask_->SubmitJobOnce([this]() {
                Pause();
            });
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
    FALSE_RETURN_V_MSG_E(videoEncFormat_ != nullptr, Status::ERROR_NULL_POINTER,
        "videoEncFormat is nullptr");
    videoEncFormat_->Set<Tag::VIDEO_ENCODE_BITRATE_MODE>(Plugins::VideoEncodeBitrateMode::VBR);
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
