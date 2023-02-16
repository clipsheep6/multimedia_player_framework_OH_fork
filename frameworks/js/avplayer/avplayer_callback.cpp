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
#include "avplayer_napi.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVPlayerCallback"};
}

namespace OHOS {
namespace Media {
class NapiCallback {
public:
    struct Base {
        std::weak_ptr<AutoRef> callback;
        std::string callbackName = "unknown";
        Base() = default;
        virtual ~Base() = default;
        virtual void UvWork()
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            // Call back function
            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 0, nullptr, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct Error : public Base {
        std::string errorMsg = "unknown";
        MediaServiceExtErrCodeAPI9 errorCode = MSERR_EXT_API9_UNSUPPORT_FORMAT;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            (void)CommonNapi::CreateError(ref->env_, errorCode, errorMsg, args[0]);

            // Call back function
            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct Int : public Base {
        int32_t value = 0;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr}; // callback: (int)
            (void)napi_create_int32(ref->env_, value, &args[0]);

            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct IntVec : public Base {
        std::vector<int32_t> valueVec;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[2] = {nullptr}; // callback: (int, int)
            (void)napi_create_int32(ref->env_, valueVec[0], &args[0]);
            (void)napi_create_int32(ref->env_, valueVec[1], &args[1]);

            const int32_t argCount = static_cast<int32_t>(valueVec.size());
            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct IntArray : public Base {
        std::vector<int32_t> valueVec;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value array = nullptr;
            (void)napi_create_array_with_length(ref->env_, valueVec.size(), &array);

            for (uint32_t i = 0; i < valueVec.size(); i++) {
                napi_value number = nullptr;
                (void)napi_create_int32(ref->env_, valueVec.at(i), &number);
                (void)napi_set_element(ref->env_, array, i, number);
            }

            napi_value result = nullptr;
            napi_value args[1] = {array};
            status = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct Double : public Base {
        double value = 0.0;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            (void)napi_create_double(ref->env_, value, &args[0]);

            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct PropertyInt : public Base {
        std::map<std::string, int32_t> valueMap;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            napi_create_object(ref->env_, &args[0]);
            for (auto &it : valueMap) {
                CommonNapi::SetPropertyInt32(ref->env_, args[0], it.first, it.second);
            }

            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    struct StateChange : public Base {
        std::string state = "";
        int32_t reason = 0;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> ref = callback.lock();
            CHECK_AND_RETURN_LOG(ref != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(ref->env_, ref->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            const int32_t argCount = 2;
            // callback: (state: AVPlayerState, reason: StateChangeReason)
            napi_value args[argCount] = {nullptr};
            (void)napi_create_string_utf8(ref->env_, state.c_str(), NAPI_AUTO_LENGTH, &args[0]);
            (void)napi_create_int32(ref->env_, reason, &args[1]);

            napi_value result = nullptr;
            status = napi_call_function(ref->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    static void CompleteCallback(napi_env env, NapiCallback::Base *jsCb)
    {
        ON_SCOPE_EXIT(0) { delete jsCb; };

        uv_loop_s *loop = nullptr;
        napi_get_uv_event_loop(env, &loop);
        CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to napi_get_uv_event_loop");

        uv_work_t *work = new(std::nothrow) uv_work_t;
        CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

        work->data = reinterpret_cast<void *>(jsCb);
        // async callback, jsWork and jsWork->data should be heap object.
        int ret = uv_queue_work(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
            CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
            (void)status;
            NapiCallback::Base *cb = reinterpret_cast<NapiCallback::Base *>(work->data);
            if (cb != nullptr) {
                MEDIA_LOGI("JsCallBack %{public}s, uv_queue_work start", cb->callbackName.c_str());
                cb->UvWork();
                delete cb;
            }
            delete work;
        });
        if (ret != 0) {
            MEDIA_LOGE("Failed to execute libuv work queue");
            delete jsCb;
            delete work;
        }
        CANCEL_SCOPE_EXIT_GUARD(0);
    }
};

AVPlayerCallback::AVPlayerCallback(napi_env env, AVPlayerNotify *listener)
    : env_(env), listener_(listener)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVPlayerCallback::~AVPlayerCallback()
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVPlayerCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    MediaServiceExtErrCodeAPI9 errorCodeApi9 = MSErrorToExtErrorAPI9(static_cast<MediaServiceErrCode>(errorCode));
    if (errorCodeApi9 == MSERR_EXT_API9_NO_PERMISSION ||
        errorCodeApi9 == MSERR_EXT_API9_NO_MEMORY ||
        errorCodeApi9 == MSERR_EXT_API9_TIMEOUT ||
        errorCodeApi9 == MSERR_EXT_API9_SERVICE_DIED ||
        errorCodeApi9 == MSERR_EXT_API9_UNSUPPORT_FORMAT) {
        Format infoBody;
        AVPlayerCallback::OnInfo(INFO_TYPE_STATE_CHANGE, PLAYER_STATE_ERROR, infoBody);
    }
    AVPlayerCallback::OnErrorCb(errorCodeApi9, errorMsg);
}

void AVPlayerCallback::OnErrorCb(MediaServiceExtErrCodeAPI9 errorCode, const std::string &errorMsg)
{
    std::string message = MSExtAVErrorToString(errorCode) + errorMsg;
    MEDIA_LOGE("OnErrorCb:errorCode %{public}d, errorMsg %{public}s", errorCode, message.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (refMap_.find(AVPlayerEvent::EVENT_ERROR) == refMap_.end()) {
        MEDIA_LOGW("can not find error callback!");
        return;
    }

    NapiCallback::Error *cb = new(std::nothrow) NapiCallback::Error();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Error");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_ERROR);
    cb->callbackName = AVPlayerEvent::EVENT_ERROR;
    cb->errorCode = errorCode;
    cb->errorMsg = message;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("OnInfo is called, PlayerOnInfoType: %{public}d", type);

    switch (type) {
        case INFO_TYPE_STATE_CHANGE:
            AVPlayerCallback::OnStateChangeCb(static_cast<PlayerStates>(extra), infoBody);
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            AVPlayerCallback::OnVolumeChangeCb(infoBody);
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
        case INFO_TYPE_DURATION_UPDATE:
            AVPlayerCallback::OnDurationUpdateCb(extra);
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

void AVPlayerCallback::OnStateChangeCb(PlayerStates state, const Format &infoBody)
{
    MEDIA_LOGI("OnStateChanged is called, current state: %{public}d", state);

    if (listener_ != nullptr) {
        listener_->NotifyState(state);
    }

    if (state_ != state) {
        state_ = state;
        static std::map<PlayerStates, std::string> stateMap = {
            { PLAYER_IDLE, AVPlayerState::STATE_IDLE },
            { PLAYER_INITIALIZED, AVPlayerState::STATE_INITIALIZED },
            { PLAYER_PREPARED, AVPlayerState::STATE_PREPARED },
            { PLAYER_STARTED, AVPlayerState::STATE_PLAYING },
            { PLAYER_PAUSED, AVPlayerState::STATE_PAUSED },
            { PLAYER_STOPPED, AVPlayerState::STATE_STOPPED },
            { PLAYER_PLAYBACK_COMPLETE, AVPlayerState::STATE_COMPLETED },
            { PLAYER_RELEASED, AVPlayerState::STATE_RELEASED },
            { PLAYER_STATE_ERROR, AVPlayerState::STATE_ERROR },
        };

        if (stateMap.find(state) != stateMap.end()) {
            if (refMap_.find(AVPlayerEvent::EVENT_STATE_CHANGE) == refMap_.end()) {
                MEDIA_LOGW("can not find state change callback!");
                return;
            }
            NapiCallback::StateChange *cb = new(std::nothrow) NapiCallback::StateChange();
            CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new StateChange");

            int32_t reason = StateChangeReason::USER;
            if (infoBody.ContainKey(PlayerKeys::PLAYER_STATE_CHANGED_REASON)) {
                (void)infoBody.GetIntValue(PlayerKeys::PLAYER_STATE_CHANGED_REASON, reason);
            }
            cb->callback = refMap_.at(AVPlayerEvent::EVENT_STATE_CHANGE);
            cb->callbackName = AVPlayerEvent::EVENT_STATE_CHANGE;
            cb->state = stateMap.at(state);
            cb->reason = reason;
            NapiCallback::CompleteCallback(env_, cb);
        }
    }
}

void AVPlayerCallback::OnVolumeChangeCb(const Format &infoBody)
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    float volumeLevel = 0.0;
    (void)infoBody.GetFloatValue(PlayerKeys::PLAYER_VOLUME_LEVEL, volumeLevel);

    MEDIA_LOGI("OnVolumeChangeCb in volume=%{public}f", volumeLevel);
    if (refMap_.find(AVPlayerEvent::EVENT_VOLUME_CHANGE) == refMap_.end()) {
        MEDIA_LOGW("can not find vol change callback!");
        return;
    }

    NapiCallback::Double *cb = new(std::nothrow) NapiCallback::Double();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Double");
    cb->callback = refMap_.at(AVPlayerEvent::EVENT_VOLUME_CHANGE);
    cb->callbackName = AVPlayerEvent::EVENT_VOLUME_CHANGE;
    cb->value = static_cast<double>(volumeLevel);
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnSeekDoneCb(int32_t currentPositon) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnSeekDone is called, currentPositon: %{public}d", currentPositon);
    if (refMap_.find(AVPlayerEvent::EVENT_SEEK_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find seekdone callback!");
        return;
    }

    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_SEEK_DONE);
    cb->callbackName = AVPlayerEvent::EVENT_SEEK_DONE;
    cb->value = currentPositon;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnSpeedDoneCb(int32_t speedMode) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnSpeedDoneCb is called, speedMode: %{public}d", speedMode);
    if (refMap_.find(AVPlayerEvent::EVENT_SPEED_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find speeddone callback!");
        return;
    }

    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_SPEED_DONE);
    cb->callbackName = AVPlayerEvent::EVENT_SPEED_DONE;
    cb->value = speedMode;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnBitRateDoneCb(int32_t bitRate) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnBitRateDoneCb is called, bitRate: %{public}d", bitRate);
    if (refMap_.find(AVPlayerEvent::EVENT_BITRATE_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find bitrate callback!");
        return;
    }

    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_BITRATE_DONE);
    cb->callbackName = AVPlayerEvent::EVENT_BITRATE_DONE;
    cb->value = bitRate;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnPositionUpdateCb(int32_t position) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnPositionUpdateCb is called, position: %{public}d", position);

    if (listener_ != nullptr) {
        listener_->NotifyPosition(position);
    }

    if (refMap_.find(AVPlayerEvent::EVENT_TIME_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find timeupdate callback!");
        return;
    }

    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_TIME_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_TIME_UPDATE;
    cb->value = position;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnDurationUpdateCb(int32_t duration) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnDurationUpdateCb is called, duration: %{public}d", duration);

    if (listener_ != nullptr) {
        listener_->NotifyDuration(duration);
    }

    if (refMap_.find(AVPlayerEvent::EVENT_DURATION_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find duration update callback!");
        return;
    }

    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_DURATION_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_DURATION_UPDATE;
    cb->value = duration;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnBufferingUpdateCb(const Format &infoBody) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnBufferingUpdateCb is called");
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

    MEDIA_LOGI("OnBufferingUpdateCb is called, buffering type: %{public}d value: %{public}d", bufferingType, value);
    NapiCallback::IntVec *cb = new(std::nothrow) NapiCallback::IntVec();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new IntVec");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_BUFFERING_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_BUFFERING_UPDATE;
    cb->valueVec.push_back(bufferingType);
    cb->valueVec.push_back(value);
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnMessageCb(int32_t extra, const Format &infoBody) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnMessageCb is called, extra: %{public}d", extra);
    if (extra == PlayerMessageType::PLAYER_INFO_VIDEO_RENDERING_START) {
        AVPlayerCallback::OnStartRenderFrameCb();
    }
}

void AVPlayerCallback::OnStartRenderFrameCb() const
{
    MEDIA_LOGI("OnStartRenderFrameCb is called");
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    if (refMap_.find(AVPlayerEvent::EVENT_START_RENDER_FRAME) == refMap_.end()) {
        MEDIA_LOGW("can not find start render callback!");
        return;
    }

    NapiCallback::Base *cb = new(std::nothrow) NapiCallback::Base();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Base");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_START_RENDER_FRAME);
    cb->callbackName = AVPlayerEvent::EVENT_START_RENDER_FRAME;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnVideoSizeChangedCb(const Format &infoBody)
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t width = 0;
    int32_t height = 0;
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_WIDTH, width);
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_HEIGHT, height);
    MEDIA_LOGI("OnVideoSizeChangedCb is called, width = %{public}d, height = %{public}d", width, height);

    if (listener_ != nullptr) {
        listener_->NotifyVideoSize(width, height);
    }

    if (refMap_.find(AVPlayerEvent::EVENT_VIDEO_SIZE_CHANGE) == refMap_.end()) {
        MEDIA_LOGW("can not find video size changed callback!");
        return;
    }
    NapiCallback::IntVec *cb = new(std::nothrow) NapiCallback::IntVec();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new IntVec");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_VIDEO_SIZE_CHANGE);
    cb->callbackName = AVPlayerEvent::EVENT_VIDEO_SIZE_CHANGE;
    cb->valueVec.push_back(width);
    cb->valueVec.push_back(height);
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnAudioInterruptCb(const Format &infoBody) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    if (refMap_.find(AVPlayerEvent::EVENT_AUDIO_INTERRUPT) == refMap_.end()) {
        MEDIA_LOGW("can not find audio interrupt callback!");
        return;
    }

    NapiCallback::PropertyInt *cb = new(std::nothrow) NapiCallback::PropertyInt();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new PropertyInt");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_AUDIO_INTERRUPT);
    cb->callbackName = AVPlayerEvent::EVENT_AUDIO_INTERRUPT;
    int32_t eventType = 0;
    int32_t forceType = 0;
    int32_t hintType = 0;
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_TYPE, eventType);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_FORCE, forceType);
    (void)infoBody.GetIntValue(PlayerKeys::AUDIO_INTERRUPT_HINT, hintType);
    MEDIA_LOGI("OnAudioInterruptCb is called, eventType = %{public}d, forceType = %{public}d, hintType = %{public}d",
        eventType, forceType, hintType);
    // ohos.multimedia.audio.d.ts interface InterruptEvent
    cb->valueMap["eventType"] = eventType;
    cb->valueMap["forceType"] = forceType;
    cb->valueMap["hintType"] = hintType;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnBitRateCollectedCb(const Format &infoBody) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    if (refMap_.find(AVPlayerEvent::EVENT_AVAILABLE_BITRATES) == refMap_.end()) {
        MEDIA_LOGW("can not find bitrate collected callback!");
        return;
    }

    std::vector<int32_t> bitrateVec;
    if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BITRATE))) {
        uint8_t *addr = nullptr;
        size_t size  = 0;
        infoBody.GetBuffer(std::string(PlayerKeys::PLAYER_BITRATE), &addr, size);
        CHECK_AND_RETURN_LOG(addr != nullptr, "bitrate addr is nullptr");

        MEDIA_LOGI("bitrate size = %{public}zu", size / sizeof(uint32_t));
        while (size > 0) {
            if (size < sizeof(uint32_t)) {
                break;
            }

            uint32_t bitrate = *(static_cast<uint32_t *>(static_cast<void *>(addr)));
            MEDIA_LOGI("bitrate = %{public}u", bitrate);
            addr += sizeof(uint32_t);
            size -= sizeof(uint32_t);
            bitrateVec.push_back(static_cast<int32_t>(bitrate));
        }
    }

    NapiCallback::IntArray *cb = new(std::nothrow) NapiCallback::IntArray();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new IntArray");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_AVAILABLE_BITRATES);
    cb->callbackName = AVPlayerEvent::EVENT_AVAILABLE_BITRATES;
    cb->valueVec = bitrateVec;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnEosCb(int32_t isLooping) const
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    MEDIA_LOGI("OnEndOfStream is called, isloop: %{public}d", isLooping);
    if (refMap_.find(AVPlayerEvent::EVENT_END_OF_STREAM) == refMap_.end()) {
        MEDIA_LOGW("can not find EndOfStream callback!");
        return;
    }

    NapiCallback::Base *cb = new(std::nothrow) NapiCallback::Base();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Base");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_END_OF_STREAM);
    cb->callbackName = AVPlayerEvent::EVENT_END_OF_STREAM;
    NapiCallback::CompleteCallback(env_, cb);
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

void AVPlayerCallback::ClearCallbackReference(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    refMap_.erase(name);
}

void AVPlayerCallback::Start()
{
    isloaded_ = true;
}

void AVPlayerCallback::Pause()
{
    isloaded_ = false;
}

void AVPlayerCallback::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);

    Format infoBody;
    AVPlayerCallback::OnStateChangeCb(PlayerStates::PLAYER_RELEASED, infoBody);
    listener_ = nullptr;
}
} // namespace Media
} // namespace OHOS