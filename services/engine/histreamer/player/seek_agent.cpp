/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "SeekAgent"

#include "seek_agent.h"
#include "media_log.h"
#include "meta/media_types.h"
#include "meta/meta.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN, "SeekAgent" };
}

namespace OHOS {
namespace Media {

AudioBufferFilledListener::AudioBufferFilledListener(std::shared_ptr<SeekAgent> seekAgent,
    sptr<AVBufferQueueProducer> producer, int32_t trackId)
    : seekAgent_(seekAgent), producer_(producer), trackId_(trackId)
{
    MEDIA_LOGI("AudioBufferFilledListener ctor called.");
}

void AudioBufferFilledListener::OnBufferFilled(std::shared_ptr<AVBuffer>& buffer)
{
    if (auto agent = seekAgent_.lock()) {
        agent->OnAudioBufferFilled(buffer, producer_, trackId_);
    } else {
        MEDIA_LOGE("Invalid agent instance.");
    }
}

VideoBufferFilledListener::VideoBufferFilledListener(std::shared_ptr<SeekAgent> seekAgent,
    sptr<AVBufferQueueProducer> producer, int32_t trackId)
    : seekAgent_(seekAgent), producer_(producer), trackId_(trackId)
{
    MEDIA_LOGI("VideoBufferFilledListener ctor called.");
}

void VideoBufferFilledListener::OnBufferFilled(std::shared_ptr<AVBuffer>& buffer)
{
    if (auto agent = seekAgent_.lock()) {
        agent->OnVideoBufferFilled(buffer, producer_, trackId_);
    } else {
        MEDIA_LOGE("Invalid agent instance.");
    }
}

SeekAgent::SeekAgent(std::shared_ptr<Pipeline::DemuxerFilter> demuxer)
    : demuxer_(demuxer), isAudioTargetArrived_(true), isVideoTargetArrived_(true),
    seekTargetPos_(-1), isSeeking_(false)
{
    MEDIA_LOGI("SeekAgent ctor called.");
}

SeekAgent::~SeekAgent()
{
    MEDIA_LOGI("~SeekAgent dtor called.");
}

Status SeekAgent::Seek(int64_t seekPos)
{
    MEDIA_LOGD("Seek start, seekPos: %{public}" PRId64, seekPos);
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, Status::ERROR_INVALID_PARAMETER, "Invalid demuxer filter instance.");
    seekTargetPos_ = seekPos;
    int64_t realSeekTime = seekPos;
    auto st = demuxer_->SeekTo(seekPos, Plugins::SeekMode::SEEK_CLOSEST_INNER, realSeekTime);
    CHECK_AND_RETURN_RET_LOG(st == Status::OK, Status::ERROR_INVALID_OPERATION, "Seekto error.");
 
    isSeeking_ = true;
    st = SetBufferFilledListener();
    CHECK_AND_RETURN_RET_LOG(st == Status::OK, Status::ERROR_INVALID_OPERATION, "SetBufferFilledListener failed.");

    MEDIA_LOGI("demuxer_ realSeekTime: %{public}" PRId64 "ns", realSeekTime);
    demuxer_->PrepareBeforeStart();
    MEDIA_LOGI("ResumeForSeek end");
    {
        AutoLock lock(targetArrivedLock_);
        demuxer_->ResumeForSeek();
        targetArrivedCond_.WaitFor(lock, WAIT_MAX_MS, [this] {return isAudioTargetArrived_ && isVideoTargetArrived_;});
        MEDIA_LOGI("Wait end");
    }
    MEDIA_LOGI("PauseForSeek start");
    demuxer_->PauseForSeek();
    st = RemoveBufferFilledListener();
    return st;
}

Status SeekAgent::GetAllTrackInfo(uint32_t &videoTrackId, std::vector<uint32_t> &audioTrackIds)
{
    auto trackInfo = demuxer_->GetStreamMetaInfo();
    for (uint32_t index = 0; index < trackInfo.size(); index++) {
        auto trackMeta = trackInfo[index];
        std::string mimeType;
        if (trackMeta->Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("video") == 0) {
            MEDIA_LOGI("Find video trackId: %{public}u, mimeType: %{public}s", index, mimeType.c_str());
            videoTrackId = index;
            continue;
        }
        if (trackMeta->Get<Tag::MIME_TYPE>(mimeType) && mimeType.find("audio") == 0) {
            MEDIA_LOGI("Find audio trackId: %{public}u, mimeType: %{public}s", index, mimeType.c_str());
            audioTrackIds.push_back(index);
        }
    }
    return Status::OK;
}

Status SeekAgent::SetBufferFilledListener()
{
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, Status::ERROR_INVALID_PARAMETER, "Invalid demuxer filter instance.");
    producerMap_ = demuxer_->GetBufferQueueProducerMap();
    CHECK_AND_RETURN_RET_LOG(!producerMap_.empty(), Status::ERROR_INVALID_PARAMETER, "producerMap is empty.");

    uint32_t videoTrackId = -1;
    std::vector<uint32_t> audioTrackIds;
    GetAllTrackInfo(videoTrackId, audioTrackIds);

    auto it = producerMap_.begin();
    while (it != producerMap_.end()) {
        if (it->second == nullptr) {
            it++;
            continue;
        }
        if (std::find(audioTrackIds.begin(), audioTrackIds.end(), it->first) != audioTrackIds.end()) {
            sptr<IBrokerListener> audioListener
                = new AudioBufferFilledListener(shared_from_this(), it->second, it->first);
            {
                AutoLock lock(targetArrivedLock_);
                isAudioTargetArrived_ = false;
            }
            MEDIA_LOGI("Add Listener audio id : %{public}d", it->first);
            it->second->SetBufferFilledListener(audioListener);
            listenerMap_.insert({it->first, audioListener});
            it++;
            continue;
        }
        if (it->first == videoTrackId) {
            sptr<IBrokerListener> videoListener
                = new VideoBufferFilledListener(shared_from_this(), it->second, it->first);
            {
                AutoLock lock(targetArrivedLock_);
                isVideoTargetArrived_ = false;
            }
            MEDIA_LOGI("Add Listener video id : %{public}d", it->first);
            it->second->SetBufferFilledListener(videoListener);
            listenerMap_.insert({it->first, videoListener});
        }
        it++;
    }
    return Status::OK;
}

Status SeekAgent::RemoveBufferFilledListener()
{
    auto it = listenerMap_.begin();
    while (it != listenerMap_.end()) {
        auto iterator = producerMap_.find(it->first);
        if (iterator == producerMap_.end()) {
            it++;
            continue;
        }
        auto producer = iterator->second;
        if (producer == nullptr) {
            it++;
            continue;
        }
        if (it->second == nullptr) {
            it++;
            continue;
        }
        producer->RemoveBufferFilledListener(it->second);
        it++;
    }
    return Status::OK;
}

Status SeekAgent::OnAudioBufferFilled(std::shared_ptr<AVBuffer>& buffer,
    sptr<AVBufferQueueProducer> producer, int32_t trackId)
{
    MEDIA_LOGD("OnAudioBufferFilled, pts: %{public}" PRId64, buffer->pts_);
    if (buffer->pts_ >= seekTargetPos_ * MS_TO_US || (buffer->flag_ & (uint32_t)(AVBufferFlag::EOS))) {
        {
            AutoLock lock(targetArrivedLock_);
            isAudioTargetArrived_ = true;
        }
        MEDIA_LOGI("audio arrive target.");
        demuxer_->PauseTaskByTrackId(trackId);
        targetArrivedCond_.NotifyAll();

        producer->ReturnBuffer(buffer, false);
        return Status::OK;
    }
    MEDIA_LOGD("OnAudioBufferFilled, ReturnBuffer");
    producer->ReturnBuffer(buffer, false);
    return Status::OK;
}

Status SeekAgent::OnVideoBufferFilled(std::shared_ptr<AVBuffer>& buffer,
    sptr<AVBufferQueueProducer> producer, int32_t trackId)
{
    MEDIA_LOGI("OnVideoBufferFilled, pts: %{public}" PRId64, buffer->pts_);
    if (buffer->pts_ >= seekTargetPos_ * MS_TO_US || (buffer->flag_ & (uint32_t)(AVBufferFlag::EOS))) {
        {
            AutoLock lock(targetArrivedLock_);
            isVideoTargetArrived_ = true;
        }
        MEDIA_LOGI("video arrive target");
        demuxer_->PauseTaskByTrackId(trackId);
        targetArrivedCond_.NotifyAll();
        producer->ReturnBuffer(buffer, true);
        return Status::OK;
    }
    bool canDrop = false;
    buffer->meta_->GetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, canDrop);
    MEDIA_LOGD("ReturnBuffer, pts: %{public}" PRId64 ", isPushBuffer: %{public}i", buffer->pts_, !canDrop);
    producer->ReturnBuffer(buffer, !canDrop);
    return Status::OK;
}
}  // namespace Media
}  // namespace OHOS
