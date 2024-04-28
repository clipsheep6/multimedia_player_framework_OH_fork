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

#include "common/media_source.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_description.h"
#include "meta_utils.h"
#include "uri_helper.h"
#include "osal/task/pipeline_threadpool.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN, "AVMetadataHelperImpl" };
}

namespace OHOS {
namespace Media {
void AVMetadataHelperImpl::OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
{
    MEDIA_LOGE("OnError errorType:%{public}d, errorCode:%{public}d", static_cast<int32_t>(errorType), errorCode);
    stopProcessing_ = true;
}

AVMetadataHelperImpl::AVMetadataHelperImpl()
{
    MEDIA_LOGD("Constructor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    groupId_ = std::string("AVMeta_") + std::to_string(OHOS::Media::Pipeline::Pipeline::GetNextPipelineId());
}

AVMetadataHelperImpl::~AVMetadataHelperImpl()
{
    MEDIA_LOGD("Destructor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Destroy();
}

int32_t AVMetadataHelperImpl::SetSource(const std::string &uri, int32_t /* usage */)
{
    UriHelper uriHelper(uri);
    if (uriHelper.UriType() != UriHelper::URI_TYPE_FILE && uriHelper.UriType() != UriHelper::URI_TYPE_FD) {
        MEDIA_LOGE("Unsupported uri type : %{private}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    MEDIA_LOGD("0x%{public}06" PRIXPTR " SetSource uri: %{private}s, type:%{public}d", FAKE_POINTER(this), uri.c_str(),
        uriHelper.UriType());

    auto ret = SetSourceInternel(uri);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, MSERR_INVALID_VAL,
        "0x%{public}06" PRIXPTR " Failed to call SetSourceInternel", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(MetaUtils::CheckFileType(mediaDemuxer_->GetGlobalMetaInfo()),
        MSERR_UNSUPPORT, "0x%{public}06" PRIXPTR "SetSource unsupport", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AVMetadataHelperImpl::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR "SetSource dataSrc", FAKE_POINTER(this));
    Status ret = SetSourceInternel(dataSrc);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, MSERR_INVALID_VAL, "Failed to call SetSourceInternel");

    CHECK_AND_RETURN_RET_LOG(MetaUtils::CheckFileType(mediaDemuxer_->GetGlobalMetaInfo()),
        MSERR_UNSUPPORT, "0x%{public}06" PRIXPTR "SetSource unsupport", FAKE_POINTER(this));
    MEDIA_LOGI("0x%{public}06" PRIXPTR "set source success", FAKE_POINTER(this));
    return MSERR_OK;
}

Status AVMetadataHelperImpl::SetSourceInternel(const std::string &uri)
{
    Reset();
    mediaDemuxer_ = std::make_shared<MediaDemuxer>();
    mediaDemuxer_->SetPlayerId(groupId_);
    CHECK_AND_RETURN_RET_LOG(
        mediaDemuxer_ != nullptr, Status::ERROR_INVALID_DATA, "SetSourceInternel demuxer is nullptr");
    Status ret = mediaDemuxer_->SetDataSource(std::make_shared<MediaSource>(uri));
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret,
        "0x%{public}06" PRIXPTR " SetSourceInternel demuxer failed to call SetDataSource", FAKE_POINTER(this));
    return Status::OK;
}

Status AVMetadataHelperImpl::SetSourceInternel(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    Reset();
    mediaDemuxer_ = std::make_shared<MediaDemuxer>();
    mediaDemuxer_->SetPlayerId(groupId_);
    CHECK_AND_RETURN_RET_LOG(
        mediaDemuxer_ != nullptr, Status::ERROR_INVALID_DATA, "SetSourceInternel demuxer is nullptr");
    Status ret = mediaDemuxer_->SetDataSource(std::make_shared<MediaSource>(dataSrc));
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "Failed to call SetDataSource");
    return Status::OK;
}

std::string AVMetadataHelperImpl::ResolveMetadata(int32_t key)
{
    MEDIA_LOGI("enter ResolveMetadata with key: %{public}d", key);
    auto res = InitMetadataCollector();
    CHECK_AND_RETURN_RET(res == Status::OK, "");
    return metadataCollector_->ExtractMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperImpl::ResolveMetadata()
{
    MEDIA_LOGD("enter ResolveMetadata");
    auto res = InitMetadataCollector();
    CHECK_AND_RETURN_RET(res == Status::OK, {});
    return metadataCollector_->ExtractMetadata();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchArtPicture()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " enter FetchArtPicture", FAKE_POINTER(this));
    auto res = InitMetadataCollector();
    CHECK_AND_RETURN_RET(res == Status::OK, nullptr);
    return metadataCollector_->GetArtPicture();
}

std::shared_ptr<Meta> AVMetadataHelperImpl::GetAVMetadata()
{
    MEDIA_LOGE("enter GetAVMetadata");
    auto res = InitMetadataCollector();
    CHECK_AND_RETURN_RET(res == Status::OK, nullptr);
    return metadataCollector_->GetAVMetadata();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    MEDIA_LOGD("enter FetchFrameAtTime");
    auto res = InitThumbnailGenerator();
    CHECK_AND_RETURN_RET(res == Status::OK, nullptr);
    return thumbnailGenerator_->FetchFrameAtTime(timeUs, option, param);
}

void AVMetadataHelperImpl::Reset()
{
    if (metadataCollector_ != nullptr) {
        metadataCollector_->Reset();
    }

    if (thumbnailGenerator_ != nullptr) {
        thumbnailGenerator_->Reset();
    }

    if (mediaDemuxer_ != nullptr) {
        mediaDemuxer_->Reset();
    }
}

void AVMetadataHelperImpl::Destroy()
{
    if (metadataCollector_ != nullptr) {
        metadataCollector_->Destroy();
    }

    if (thumbnailGenerator_ != nullptr) {
        thumbnailGenerator_->Destroy();
    }

    metadataCollector_ = nullptr;
    thumbnailGenerator_ = nullptr;
    PipeLineThreadPool::GetInstance().DestroyThread(groupId_);
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Finish Destroy.", FAKE_POINTER(this));
}

Status AVMetadataHelperImpl::InitMetadataCollector()
{
    if (metadataCollector_ == nullptr) {
        metadataCollector_ = std::make_shared<AVMetaDataCollector>(mediaDemuxer_);
    }
    CHECK_AND_RETURN_RET_LOG(
        metadataCollector_ != nullptr, Status::ERROR_INVALID_STATE, "Init metadata collector failed.");
    return Status::OK;
}

Status AVMetadataHelperImpl::InitThumbnailGenerator()
{
    if (thumbnailGenerator_ == nullptr) {
        thumbnailGenerator_ = std::make_shared<AVThumbnailGenerator>(mediaDemuxer_);
    }
    CHECK_AND_RETURN_RET_LOG(
        thumbnailGenerator_ != nullptr, Status::ERROR_INVALID_STATE, "Init thumbnail generator failed.");
    return Status::OK;
}
}  // namespace Media
}  // namespace OHOS