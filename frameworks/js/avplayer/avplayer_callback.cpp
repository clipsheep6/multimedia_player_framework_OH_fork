/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "avplayer_callback.h"
#include <uv.h>
#include "media_errors.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVPlayerCallback"};
}

namespace OHOS {
namespace Media {
AVPlayerCallback::AVPlayerCallback(napi_env env, )
    : env_(env)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVPlayerCallback::~AVPlayerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVPlayerCallback::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    MediaServiceExtErrCodeAPI9 err = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errorCode));
    std::string msg = "avplayer error:";
    return AVPlayerCallback::OnErrorCb(err, msg);
}

void AVPlayerCallback::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    MEDIA_LOGE("OnErrorCb:errorCode %{public}d, errorMsg %{public}s", errorCode, errorMsg.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVPlayerEvent::EVENT_ERROR) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    MediaNapiCall::EventError *event = new(std::nothrow) MediaNapiCall::EventError();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventError");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_ERROR);
    event->callbackName = AVPlayerEvent::EVENT_ERROR;
    event->errorCode = errorCode;
    event->errorMsg = errorMsg;
    return MediaNapiCall::CallError(env_, event);
}

void AVPlayerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("OnInfo is called, PlayerOnInfoType: %{public}d", type);
    switch (type) {
        case INFO_TYPE_STATE_CHANGE:
            AVPlayerCallback::OnStateChangeCb(static_cast<PlayerStates>(extra));
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            AVPlayerCallback::OnVolumeChangeCb(extra);
            break;
        case INFO_TYPE_SEEKDONE:
            AVPlayerCallback::OnSeekDoneCb(extra);
            break;
        case INFO_TYPE_SPEEDDONE:
            AVPlayerCallback::OnSpeedDoneCb(extra);
            break;
        case INFO_TYPE_BITRATEDONE:
            AVPlayerCallback::OnBitRateDoneCb(extra);
            break;
        case INFO_TYPE_POSITION_UPDATE:
            AVPlayerCallback::OnPositionUpdateCb(extra);
            break;
        case INFO_TYPE_BUFFERING_UPDATE:
            AVPlayerCallback::OnBufferingUpdateCb(infoBody);
            break;
        case INFO_TYPE_MESSAGE:
            AVPlayerCallback::OnMessageCb(extra, infoBody);
            break;
        case INFO_TYPE_RESOLUTION_CHANGE:
            AVPlayerCallback::OnVideoSizeChangedCb(infoBody);
            break;
        case INFO_TYPE_INTERRUPT_EVENT:
            AVPlayerCallback::OnAudioInterruptCb(infoBody);
            break;
        case INFO_TYPE_BITRATE_COLLECT:
            AVPlayerCallback::OnBitRateCollectedCb(infoBody);
            break;
        case INFO_TYPE_EOS:
            AVPlayerCallback::OnEosCb(extra);
            break;
        default:
            break;
    }
}

void AVPlayerCallback::OnStateChangeCb(PlayerStates state)
{
    MEDIA_LOGD("OnStateChanged is called, current state: %{public}d", state);
    state_ = state;

    static std::map<enum PlayerStates, std::string> stateMap = {
        { PLAYER_PREPARED, AVPlayerState::STATE_PREPARED },
        { PLAYER_STARTED, AVPlayerState::STATE_PLAYING },
        { PLAYER_PAUSED, AVPlayerState::STATE_PAUSED },
        { PLAYER_STOPPED, AVPlayerState::STATE_STOPPED },
        { PLAYER_IDLE, AVPlayerState::STATE_IDLE },
        { PLAYER_STATE_ERROR, AVPlayerState::STATE_ERROR },
    };

    auto it = stateMap.find(state);
    if (it != stateMap.end()) {
        std::string stateName = stateMap.at(state);
        // int32_t reason = StateChangeReason::USER;
        if (refMap_.find(AVPlayerEvent::EVENT_STATE_CHANGE) == refMap_.end()) {
            MEDIA_LOGW("can not find state change callback!");
            return;
        }
        // 发状态码
    } else if (state == PLAYER_PLAYBACK_COMPLETE) {
        if (refMap_.find(AVPlayerEvent::EVENT_PLAY_COMPLETE) == refMap_.end()) {
            MEDIA_LOGW("can not find play complete callback!");
            return;
        }

        MediaNapiCall::EventBase *event = new(std::nothrow) MediaNapiCall::EventBase();
        CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventBase");

        event->callback = refMap_.at(AVPlayerEvent::EVENT_PLAY_COMPLETE);
        event->callbackName = AVPlayerEvent::EVENT_PLAY_COMPLETE;
        MediaNapiCall::CallSignle(env_, event);
    }
}

void AVPlayerCallback::OnVolumeChangeCb(double volumeLevel)
{
    MEDIA_LOGD("OnVolumeChangeCb in");
    if (refMap_.find(AVPlayerEvent::EVENT_VOLUME_CHANGE) == refMap_.end()) {
        MEDIA_LOGW("can not find vol change callback!");
        return;
    }

    MediaNapiCall::EventDoubleVec *event = new(std::nothrow) MediaNapiCall::EventDoubleVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventDoubleVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_VOLUME_CHANGE);
    event->callbackName = AVPlayerEvent::EVENT_VOLUME_CHANGE;
    event->valueVec.push_back(volumeLevel);
    return MediaNapiCall::CallDoubleVec(env_, event);
}

void AVPlayerCallback::OnSeekDoneCb(int32_t currentPositon) const
{
    MEDIA_LOGD("OnSeekDone is called, currentPositon: %{public}d", currentPositon);
    if (refMap_.find(AVPlayerEvent::EVENT_SEEK_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find seekdone callback!");
        return;
    }

    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_SEEK_DONE);
    event->callbackName = AVPlayerEvent::EVENT_SEEK_DONE;
    event->valueVec.push_back(currentPositon);
    return MediaNapiCall::CallIntVec(env_, event);
}

void AVPlayerCallback::OnSpeedDoneCb(int32_t speedMode) const
{
    MEDIA_LOGD("OnSpeedDoneCb is called, speedMode: %{public}d", speedMode);
    if (refMap_.find(AVPlayerEvent::EVENT_SPEED_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find speeddone callback!");
        return;
    }

    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_SPEED_DONE);
    event->callbackName = AVPlayerEvent::EVENT_SPEED_DONE;
    event->valueVec.push_back(speedMode);
    return MediaNapiCall::CallIntVec(env_, event);
}

void AVPlayerCallback::OnBitRateDoneCb(int32_t bitRate) const
{
    MEDIA_LOGD("OnBitRateDoneCb is called, bitRate: %{public}d", bitRate);
    if (refMap_.find(AVPlayerEvent::EVENT_BITRATE_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find bitrate callback!");
        return;
    }

    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_BITRATE_DONE);
    event->callbackName = AVPlayerEvent::EVENT_BITRATE_DONE;
    event->valueVec.push_back(bitRate);
    return MediaNapiCall::CallIntVec(env_, event);
}

void AVPlayerCallback::OnPositionUpdateCb(int32_t position) const
{
    MEDIA_LOGD("OnPositionUpdateCb is called, position: %{public}d", position);
    if (refMap_.find(AVPlayerEvent::EVENT_TIME_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find timeupdate callback!");
        return;
    }

    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_TIME_UPDATE);
    event->callbackName = AVPlayerEvent::EVENT_TIME_UPDATE;
    event->valueVec.push_back(position);
    return MediaNapiCall::CallIntVec(env_, event);
}

void AVPlayerCallback::OnBufferingUpdateCb(const Format &infoBody) const
{
    MEDIA_LOGD("OnBufferingUpdateCb is called");
    if (refMap_.find(AVPlayerEvent::EVENT_BUFFERING_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find buffering update callback!");
        return;
    }

    int32_t value = 0;
    int32_t bufferingType = -1;
    if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_START))) {
        bufferingType = BUFFERING_START;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), value);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_END))) {
        bufferingType = BUFFERING_END;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), value);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT))) {
        bufferingType = BUFFERING_PERCENT;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT), value);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_CACHED_DURATION))) {
        bufferingType = CACHED_DURATION;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION), value);
    } else {
        return;
    }

    MEDIA_LOGD("OnBufferingUpdateCb is called, buffering type: %{public}d value: %{public}d", bufferingType, value);

    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_BUFFERING_UPDATE);
    event->callbackName = AVPlayerEvent::EVENT_BUFFERING_UPDATE;
    event->valueVec.push_back(bufferingType);
    event->valueVec.push_back(value);
    return MediaNapiCall::CallIntVec(env_, event);
}

void AVPlayerCallback::OnMessageCb(int32_t extra, const Format &infoBody) const
{
    MEDIA_LOGD("OnMessageCb is called, extra: %{public}d", extra);
    if (extra == PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START) {
        AVPlayerCallback::OnStartRenderFrameCb();
    }
}

void AVPlayerCallback::OnStartRenderFrameCb() const
{
    MEDIA_LOGD("OnStartRenderFrameCb is called");
    if (refMap_.find(AVPlayerEvent::EVENT_START_RENDER_FRAME) == refMap_.end()) {
        MEDIA_LOGW("can not find start render callback!");
        return;
    }

    MediaNapiCall::EventBase *event = new(std::nothrow) MediaNapiCall::EventBase();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventBase");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_START_RENDER_FRAME);
    event->callbackName = AVPlayerEvent::EVENT_START_RENDER_FRAME;
    MediaNapiCall::CallSignle(env_, event);
}

void AVPlayerCallback::OnVideoSizeChangedCb(const Format &infoBody)
{
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_WIDTH, width_);
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_HEIGHT, height_);
    MEDIA_LOGD("OnVideoSizeChangedCb is called, width = %{public}d, height = %{public}d", width_, height_);
    if (refMap_.find(AVPlayerEvent::EVENT_VIDEO_SIZE_CHANGE) == refMap_.end()) {
        MEDIA_LOGW("can not find video size changed callback!");
        return;
    }
    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_VIDEO_SIZE_CHANGE);
    event->callbackName = AVPlayerEvent::EVENT_VIDEO_SIZE_CHANGE;
    event->valueVec.push_back(width_);
    event->valueVec.push_back(height_);
    return MediaNapiCall::CallIntVec(env_, event);
}

void AVPlayerCallback::OnAudioInterruptCb(const Format &infoBody) const
{
    if (refMap_.find(AVPlayerEvent::EVENT_AUDIO_INTERRUPT) == refMap_.end()) {
        MEDIA_LOGW("can not find audio interrupt callback!");
        return;
    }

    MediaNapiCall::EventPropertyInt *event = new(std::nothrow) MediaNapiCall::EventPropertyInt();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventPropertyInt");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_AUDIO_INTERRUPT);
    event->callbackName = AVPlayerEvent::EVENT_AUDIO_INTERRUPT;
    int32_t eventType = 0;
    int32_t forceType = 0;
    int32_t hintType = 0;
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventType);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceType);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintType);
    MEDIA_LOGD("OnAudioInterruptCb is called, eventType = %{public}d, forceType = %{public}d, hintType = %{public}d",
        eventType, forceType, hintType);
    // ohos.multimedia.audio.d.ts interface InterruptEvent
    event->valueVec["eventType"] = eventType;
    event->valueVec["forceType"] = forceType;
    event->valueVec["hintType"] = hintType;
    return MediaNapiCall::CallPropertyInt(env_, event);
}

void AVPlayerCallback::OnBitRateCollectedCb(const Format &infoBody) const
{
    if (refMap_.find(AVPlayerEvent::EVENT_AVAILABLE_BITRATES) == refMap_.end()) {
        MEDIA_LOGW("can not find bitrate collected callback!");
        return;
    }

    std::vector<int32_t> bitrateVec;
    if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BITRATE))) {
        uint8_t *addr = nullptr;
        size_t size  = 0;
        uint32_t bitrate = 0;
        infoBody.GetBuffer(std::string(PlayerKeys::PLAYER_BITRATE), &addr, size);
        CHECK_AND_RETURN_LOG(addr != nullptr, "bitrate addr is nullptr");

        MEDIA_LOGD("bitrate size = %{public}zu", size / sizeof(uint32_t));
        while (size > 0) {
            if ((size - sizeof(uint32_t)) < 0) {
                break;
            }

            bitrate = *(static_cast<uint32_t *>(static_cast<void *>(addr)));
            MEDIA_LOGD("bitrate = %{public}u", bitrate);
            addr += sizeof(uint32_t);
            size -= sizeof(uint32_t);
            bitrateVec.push_back(static_cast<int32_t>(bitrate));
        }
    }

    MediaNapiCall::EventIntVec *event = new(std::nothrow) MediaNapiCall::EventIntVec();
    CHECK_AND_RETURN_LOG(event != nullptr, "failed to new EventIntVec");

    event->callback = refMap_.at(AVPlayerEvent::EVENT_AVAILABLE_BITRATES);
    event->callbackName = AVPlayerEvent::EVENT_AVAILABLE_BITRATES;
    event->valueVec = bitrateVec;
    return MediaNapiCall::CallIntArray(env_, event);
}

void AVPlayerCallback::OnEosCb(int32_t isLooping) const
{
    MEDIA_LOGD("OnEndOfStream is called, isloop: %{public}d", isLooping);
}

void AVPlayerCallback::SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_[name] = ref;
}

void AVPlayerCallback::ClearCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.clear();
}

void AVPlayerCallback::SaveAsyncContext(const std::string &name, std::shared_ptr<MediaAsyncContext> ctx)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (ctxMap_.find(name) == ctxMap_.end())  {
        std::queue<std::shared_ptr<MediaAsyncContext>> contextQue;
        contextQue.push(ctx);
        ctxMap_[name] = contextQue;
    } else {
        ctxMap_.at(name).push(ctx);
    }
}

void AVPlayerCallback::ClearAsyncContext()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = ctxMap_.begin(); it != ctxMap_.end(); it++) {
        auto &contextQue = it->second;
        while (!contextQue.empty()) {
            std::shared_ptr<MediaAsyncContext> context = contextQue.front();
            contextQue.pop();
            // 
        }
    }
    ctxMap_.clear();
}

PlayerStates AVPlayerCallback::GetCurrentState() const
{
    return state_;
}

int32_t AVPlayerCallback::GetVideoWidth() const
{
    return width_;
}

int32_t AVPlayerCallback::GetVideoHeight() const
{
    return height_;
}
} // namespace Media
} // namespace OHOS