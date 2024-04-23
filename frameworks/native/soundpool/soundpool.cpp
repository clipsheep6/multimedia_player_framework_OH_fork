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
#include <unistd.h>
#include "media_errors.h"
#include "media_log.h"
#include "soundpool_manager.h"
#include "soundpool.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SoundPool"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<ISoundPool> SoundPoolFactory::CreateSoundPool(int maxStreams,
    AudioStandard::AudioRendererInfo audioRenderInfo)
{
    std::shared_ptr<SoundPool> impl;
    if (!SoundPool::CheckInitParam(maxStreams, audioRenderInfo)) {
        return nullptr;
    }
    SoundPoolManager::GetInstance().SetSoundPool(getpid(), impl);
    SoundPoolManager::GetInstance().GetSoundPool(getpid(), impl);
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to get SoundPool");

    int32_t ret = impl->Init(maxStreams, audioRenderInfo);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init SoundPool");

    return impl;
}

SoundPool::SoundPool()
{
    MEDIA_LOGI("Construction SoundPool.");
}

SoundPool::~SoundPool()
{
    MEDIA_LOGI("Destruction SoundPool.");
    ReleaseInner();
}

int32_t SoundPool::Init(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo)
{
    // start contruct stream manager
    std::lock_guard lock(soundPoolLock_);
    streamIdManager_ = std::make_shared<StreamIDManager>(maxStreams, audioRenderInfo);
    soundIDManager_ = std::make_shared<SoundIDManager>();
    return MSERR_OK;
}

bool SoundPool::CheckInitParam(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo)
{
    if (maxStreams <= 0) {
        return false;
    }
    if (audioRenderInfo.contentType < AudioStandard::CONTENT_TYPE_UNKNOWN
        || audioRenderInfo.contentType > AudioStandard::CONTENT_TYPE_ULTRASONIC
        || audioRenderInfo.streamUsage < AudioStandard::STREAM_USAGE_UNKNOWN
        || audioRenderInfo.streamUsage > AudioStandard::STREAM_USAGE_VOICE_MODEM_COMMUNICATION
        || audioRenderInfo.rendererFlags < 0 || audioRenderInfo.rendererFlags > 1) {
        return false;
    }
    return true;
}

int32_t SoundPool::Load(const std::string url)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(!url.empty(), -1, "Failed to obtain SoundPool for load");
    CHECK_AND_RETURN_RET_LOG(soundIDManager_ != nullptr, -1, "sound id manager have released.");
    return soundIDManager_->Load(url);
}

int32_t SoundPool::Load(int32_t fd, int64_t offset, int64_t length)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG((fd > 0 && length > 0 && offset >= 0), -1, "Invalid fd param.");
    CHECK_AND_RETURN_RET_LOG(soundIDManager_ != nullptr, -1, "sound id manager have released.");
    return soundIDManager_->Load(fd, offset, length);
}

int32_t SoundPool::Play(int32_t soundID, PlayParams playParameters)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, -1, "sound pool have released.");
    CHECK_AND_RETURN_RET_LOG(soundIDManager_ != nullptr, -1, "sound id manager have released.");
    std::shared_ptr<SoundParser> soundParser = soundIDManager_->FindSoundParser(soundID);

    CHECK_AND_RETURN_RET_LOG(soundParser != nullptr, -1, "Invalid sound.");
    if (!soundParser->IsSoundParserCompleted()) {
        MEDIA_LOGE("sound load no completed. ");
        return -1;
    }
    const int32_t streamID = streamIdManager_->Play(soundParser, playParameters);
    return streamID;
}

int32_t SoundPool::Stop(int32_t streamID)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, MSERR_INVALID_VAL, "sound pool have released.");
    if (std::shared_ptr<CacheBuffer> cacheBuffer = streamIdManager_->FindCacheBuffer(streamID)) {
        return cacheBuffer->Stop(streamID);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t SoundPool::SetLoop(int32_t streamID, int32_t loop)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, MSERR_INVALID_VAL, "sound pool have released.");
    if (std::shared_ptr<CacheBuffer> cacheBuffer = streamIdManager_->FindCacheBuffer(streamID)) {
        return cacheBuffer->SetLoop(streamID, loop);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t SoundPool::SetPriority(int32_t streamID, int32_t priority)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, MSERR_INVALID_VAL, "sound pool have released.");
    if (std::shared_ptr<CacheBuffer> cacheBuffer = streamIdManager_->FindCacheBuffer(streamID)) {
        if (priority < MIN_STREAM_PRIORITY) {
            MEDIA_LOGI("Invalid priority, align priority to min.");
            priority = MIN_STREAM_PRIORITY;
        }
        return cacheBuffer->SetPriority(streamID, priority);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t SoundPool::SetRate(int32_t streamID, AudioStandard::AudioRendererRate renderRate)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, MSERR_INVALID_VAL, "sound pool have released.");
    if (std::shared_ptr<CacheBuffer> cacheBuffer = streamIdManager_->FindCacheBuffer(streamID)) {
        return cacheBuffer->SetRate(streamID, renderRate);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t SoundPool::SetVolume(int32_t streamID, float leftVolume, float rightVolume)
{
    if (!CheckVolumeVaild(&leftVolume, &rightVolume)) {
        return MSERR_INVALID_VAL;
    }

    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, MSERR_INVALID_VAL, "sound pool have released.");
    std::lock_guard lock(soundPoolLock_);
    if (std::shared_ptr<CacheBuffer> cacheBuffer = streamIdManager_->FindCacheBuffer(streamID)) {
        return cacheBuffer->SetVolume(streamID, leftVolume, rightVolume);
    }
    return MSERR_INVALID_OPERATION;
}

int32_t SoundPool::Unload(int32_t soundID)
{
    std::lock_guard lock(soundPoolLock_);
    CHECK_AND_RETURN_RET_LOG(streamIdManager_ != nullptr, -1, "sound pool have released.");
    CHECK_AND_RETURN_RET_LOG(soundIDManager_ != nullptr, -1, "sound id manager have released.");
    if (std::shared_ptr<CacheBuffer> cacheBuffer =
        streamIdManager_->FindCacheBuffer(streamIdManager_->GetStreamIDBySoundID(soundID))) {
        cacheBuffer->Release();
    }
    return soundIDManager_->Unload(soundID);
}

int32_t SoundPool::Release()
{
    return ReleaseInner();
}

int32_t SoundPool::ReleaseInner()
{
    std::lock_guard lock(soundPoolLock_);
    MEDIA_LOGI("Release SoundPool.");
    if (streamIdManager_ != nullptr) {
        streamIdManager_.reset();
    }
    if (soundIDManager_ != nullptr) {
        soundIDManager_.reset();
    }
    if (callback_ != nullptr) {
        callback_.reset();
    }
    if (frameWriteCallback_ != nullptr) {
        frameWriteCallback_.reset();
    }
    SoundPoolManager::GetInstance().Release(getpid());
    return MSERR_OK;
}

int32_t SoundPool::SetSoundPoolCallback(const std::shared_ptr<ISoundPoolCallback> &soundPoolCallback)
{
    MEDIA_LOGI("SoundPool::%{public}s", __func__);
    if (soundIDManager_ != nullptr) soundIDManager_->SetCallback(soundPoolCallback);
    if (streamIdManager_ != nullptr) streamIdManager_->SetCallback(soundPoolCallback);
    callback_ = soundPoolCallback;
    return MSERR_OK;
}

int32_t SoundPool::SetSoundPoolFrameWriteCallback(
    const std::shared_ptr<ISoundPoolFrameWriteCallback> &frameWriteCallback)
{
    MEDIA_LOGI("SoundPool::%{public}s", __func__);
    if (streamIdManager_ != nullptr) streamIdManager_->SetFrameWriteCallback(frameWriteCallback);
    frameWriteCallback_ = frameWriteCallback;
    return MSERR_OK;
}

bool SoundPool::CheckVolumeVaild(float *leftVol, float *rightVol)
{
    if (*leftVol != std::clamp(*leftVol, 0.f, 1.f) ||
        *rightVol != std::clamp(*rightVol, 0.f, 1.f)) {
        MEDIA_LOGI("volume l=%{public}f r=%{public}f out of (0.f, 1.f) bounds, using 1.f", *leftVol, *rightVol);
        *leftVol = *rightVol = 1.f;
    }
    if (*leftVol != *rightVol) {
        MEDIA_LOGI("left volume %{public}f set not eq the right volume %{public}f ,use the left volume",
            *leftVol, *rightVol);
        *rightVol = *leftVol;
    }
    return true;
}
} // namespace Media
} // namespace OHOS
