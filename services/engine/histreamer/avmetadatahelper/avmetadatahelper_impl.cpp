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

#include "avmetadatahelper_impl.h"
#include "media_errors.h"
#include "common/log.h"
#include "common/media_source.h"
#include "uri_helper.h"
#include "meta/meta.h"
#include "meta/meta_key.h"

namespace OHOS {
namespace Media {
static const std::set<PixelFormat> SUPPORTED_PIXELFORMAT = {
    PixelFormat::RGB_565, PixelFormat::RGB_888, PixelFormat::RGBA_8888
};

class HelperEventReceiver : public Pipeline::EventReceiver {
public:
    explicit HelperEventReceiver(AVMetadataHelperImpl *helperImpl) : helperImpl_(helperImpl)
    {
    }

    void OnEvent(const Event &event)
    {
        helperImpl_->OnEvent(event);
    }

private:
    AVMetadataHelperImpl* helperImpl_;
};

class HelperFilterCallback : public Pipeline::FilterCallback {
public:
    explicit HelperFilterCallback(AVMetadataHelperImpl* helperImpl) : helperImpl_(helperImpl)
    {
    }

    void OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType)
    {
        helperImpl_->OnCallback(filter, cmd, outType);
    }

private:
    AVMetadataHelperImpl* helperImpl_;
};

void AVMetadataHelperImpl::OnEvent(const Event &event)
{
}

void AVMetadataHelperImpl::OnCallback(std::shared_ptr<Pipeline::Filter> filter,
    const Pipeline::FilterCallBackCommand cmd, Pipeline::StreamType outType)
{
}

AVMetadataHelperImpl::AVMetadataHelperImpl()
{
    MEDIA_LOG_I("enter AVMetadataHelperImpl, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    metaCollector_ = std::make_shared<AVMetaDataCollector>();
}

AVMetadataHelperImpl::~AVMetadataHelperImpl()
{
    MEDIA_LOG_I("enter ~AVMetadataHelperImpl, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
}

int32_t AVMetadataHelperImpl::SetSource(const std::string &uri, int32_t usage)
{
    if ((usage != AVMetadataUsage::AV_META_USAGE_META_ONLY) &&
        (usage != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP)) {
        MEDIA_LOG_E("Invalid avmetadatahelper usage: %{public}d", usage);
        return MSERR_INVALID_VAL;
    }
    UriHelper uriHelper(uri);
    if (uriHelper.UriType() != UriHelper::URI_TYPE_FILE && uriHelper.UriType() != UriHelper::URI_TYPE_FD) {
        MEDIA_LOG_E("Unsupported uri type : %{public}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    usage_ = usage;
    MEDIA_LOG_I("uri: %{public}s, usage: %{public}d", uri.c_str(), usage);

    if (usage == AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        pipeline_ = std::make_shared<Pipeline::Pipeline>();
        demuxerFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::DemuxerFilter>(
            "builtin.player.demuxer", Pipeline::FilterType::FILTERTYPE_DEMUXER);
        FALSE_RETURN_V(demuxerFilter_ != nullptr, MSERR_INVALID_VAL);

        auto eventReceiver = std::make_shared<HelperEventReceiver>(this);
        auto filterCallback = std::make_shared<HelperFilterCallback>(this);
        pipeline_->Init(eventReceiver, filterCallback);
        demuxerFilter_->Init(eventReceiver, filterCallback);
        videoDecoderFilter_ = Pipeline::FilterFactory::Instance().CreateFilter<Pipeline::CodecFilter>(
            "builtin.player.videodecoder", Pipeline::FilterType::FILTERTYPE_VDEC);
        FALSE_RETURN_V(videoDecoderFilter_ != nullptr, MSERR_INVALID_VAL);
    } else {
        mediaDemuxer_ = std::make_shared<MediaDemuxer>();
    }

    Status ret = SetSourceInternel(uri, usage);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, MSERR_INVALID_VAL, "Failed to call SetSourceInternel");

    MEDIA_LOG_I("set source success");
    return MSERR_OK;
}

int32_t AVMetadataHelperImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    MEDIA_LOG_I("SetSource");
    return MSERR_OK;
}

std::string AVMetadataHelperImpl::ResolveMetadata(int32_t key)
{
    MEDIA_LOG_I("enter ResolveMetadata with key: %{public}d", key);
    std::string result;

    int32_t ret = ExtractMetadata();
    FALSE_RETURN_V_MSG_E(ret == MSERR_OK, result, "Failed to call ExtractMetadata");

    auto it = collectedMeta_.find(key);
    if (it == collectedMeta_.end() || it->second.empty()) {
        MEDIA_LOG_E("The specified metadata %{public}d cannot be obtained from the specified stream.", key);
        return result;
    }

    MEDIA_LOG_I("exit ResolveMetadata with key");
    result = collectedMeta_[key];
    return result;
}

std::unordered_map<int32_t, std::string> AVMetadataHelperImpl::ResolveMetadata()
{
    MEDIA_LOG_I("enter ResolveMetadata");

    int32_t ret = ExtractMetadata();
    FALSE_RETURN_V_MSG_E(ret == MSERR_OK, {}, "Failed to call ExtractMetadata");

    MEDIA_LOG_I("exit ResolveMetadata");
    return collectedMeta_;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchArtPicture()
{
    MEDIA_LOG_I("enter FetchArtPicture");
    if (collectedArtPicture_ != nullptr) {
        MEDIA_LOG_I("Repeated ExtractArtPicture");
        return collectedArtPicture_;
    }
    if (mediaDemuxer_ == nullptr) {
        MEDIA_LOG_E("ExtractArtPicutre Failed : mediaDemuxer_ is nullptr");
        return nullptr;
    }
    const std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
    collectedArtPicture_ = metaCollector_->GetArtPicture(trackInfos);
    if (collectedArtPicture_ == nullptr) {
        MEDIA_LOG_E("ExtractArtPicutre Failed");
    }
    return collectedArtPicture_;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    MEDIA_LOG_I("enter FetchFrameAtTime");
    return nullptr;
}

Status AVMetadataHelperImpl::SetSourceInternel(const std::string &uri, int32_t usage)
{
    Reset();

    Status ret;
    if (usage_ == AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        ret = demuxerFilter_->SetDataSource(std::make_shared<MediaSource>(uri));
    } else {
        ret = mediaDemuxer_->SetDataSource(std::make_shared<MediaSource>(uri));
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to call SetDataSource");

    ret = PrepareInternel();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Failed to call PrepareInternel");

    return Status::OK;
}

Status AVMetadataHelperImpl::PrepareInternel()
{
    Status ret = Status::OK;
    if (usage_ == AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        FALSE_RETURN_V_MSG_E(demuxerFilter_ != nullptr, Status::ERROR_INVALID_OPERATION, "set source firstly");
        ret = demuxerFilter_->Prepare();
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_INVALID_DATA, "prepare failed");
    return Status::OK;
}

int32_t AVMetadataHelperImpl::ExtractMetadata()
{
    FALSE_RETURN_V_MSG_E(usage_ != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP, MSERR_INVALID_OPERATION, "usage error");
    FALSE_RETURN_V_MSG_E(mediaDemuxer_ != nullptr, MSERR_INVALID_OPERATION, "mediaDemuxer_ is nullptr");

    if (!hasCollectMeta_) {
        const std::shared_ptr<Meta> globalInfo = mediaDemuxer_->GetGlobalMetaInfo();
        const std::vector<std::shared_ptr<Meta>> trackInfos = mediaDemuxer_->GetStreamMetaInfo();
        collectedMeta_ = metaCollector_->GetMetadata(globalInfo, trackInfos);
        hasCollectMeta_ = true;
    }
    return MSERR_OK;
}

void AVMetadataHelperImpl::Reset()
{
    if (demuxerFilter_ != nullptr) {
        demuxerFilter_->Stop();
        hasCollectMeta_ = false;
        collectedArtPicture_ = nullptr;
    }

    if (demuxerFilter_ != nullptr) {
        demuxerFilter_->Reset();
    }

    if (mediaDemuxer_ != nullptr) {
        mediaDemuxer_->Reset();
    }

    errHappened_ = false;
    firstFetch_ = true;
}
} // namespace Media
} // namespace OHOS