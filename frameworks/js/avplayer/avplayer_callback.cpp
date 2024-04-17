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

#include <map>
#include <iostream>
#include <sstream>
#include <uv.h>
#include "avplayer_napi.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"
#include "event_queue.h"
#include "avplayer_callback.h"

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

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(ref->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(ref->env_, scope);
            };

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
        virtual void JsCallback()
        {
            UvWork();
            delete this;
        }
    };

    struct Error : public Base {
        std::string errorMsg = "unknown";
        MediaServiceExtErrCodeAPI9 errorCode = MSERR_EXT_API9_UNSUPPORT_FORMAT;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> errorRef = callback.lock();
            CHECK_AND_RETURN_LOG(errorRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(errorRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(errorRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status napiStatus = napi_get_reference_value(errorRef->env_, errorRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(napiStatus == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            (void)CommonNapi::CreateError(errorRef->env_, errorCode, errorMsg, args[0]);

            // Call back function
            napi_value result = nullptr;
            napiStatus = napi_call_function(errorRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(napiStatus == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct Int : public Base {
        int32_t value = 0;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> intRef = callback.lock();
            CHECK_AND_RETURN_LOG(intRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(intRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(intRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(intRef->env_, intRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr}; // callback: (int)
            (void)napi_create_int32(intRef->env_, value, &args[0]);

            napi_value result = nullptr;
            status = napi_call_function(intRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct IntVec : public Base {
        std::vector<int32_t> valueVec;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> intVecRef = callback.lock();
            CHECK_AND_RETURN_LOG(intVecRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(intVecRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(intVecRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(intVecRef->env_, intVecRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[2] = {nullptr}; // callback: (int, int)
            (void)napi_create_int32(intVecRef->env_, valueVec[0], &args[0]);
            (void)napi_create_int32(intVecRef->env_, valueVec[1], &args[1]);

            const int32_t argCount = static_cast<int32_t>(valueVec.size());
            napi_value result = nullptr;
            status = napi_call_function(intVecRef->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct IntArray : public Base {
        std::vector<int32_t> valueVec;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> intArrayRef = callback.lock();
            CHECK_AND_RETURN_LOG(intArrayRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(intArrayRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(intArrayRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(intArrayRef->env_, intArrayRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value array = nullptr;
            (void)napi_create_array_with_length(intArrayRef->env_, valueVec.size(), &array);

            for (uint32_t i = 0; i < valueVec.size(); i++) {
                napi_value number = nullptr;
                (void)napi_create_int32(intArrayRef->env_, valueVec.at(i), &number);
                (void)napi_set_element(intArrayRef->env_, array, i, number);
            }

            napi_value result = nullptr;
            napi_value args[1] = {array};
            status = napi_call_function(intArrayRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct Double : public Base {
        double value = 0.0;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> doubleRef = callback.lock();
            CHECK_AND_RETURN_LOG(doubleRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(doubleRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(doubleRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(doubleRef->env_, doubleRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            (void)napi_create_double(doubleRef->env_, value, &args[0]);

            napi_value result = nullptr;
            status = napi_call_function(doubleRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct SubtitleProperty : public Base {
        std::string text;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> subtitleRef = callback.lock();
            CHECK_AND_RETURN_LOG(subtitleRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(subtitleRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(subtitleRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(subtitleRef->env_, subtitleRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            // callback: (textInfo: TextInfoDescriptor)
            napi_value args[1] = {nullptr};
            napi_create_object(subtitleRef->env_, &args[0]);
            (void)CommonNapi::SetPropertyString(subtitleRef->env_, args[0], "text", text);
            napi_value result = nullptr;
            status = napi_call_function(subtitleRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    struct ObjectArray : public Base {
        std::multimap<std::string, std::vector<uint8_t>> infoMap;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> mapRef = callback.lock();
            CHECK_AND_RETURN_LOG(mapRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(mapRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(mapRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(mapRef->env_, mapRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            uint32_t index = 0;
            napi_value napiMap;
            napi_create_array_with_length(mapRef->env_, infoMap.size(), &napiMap);
            for (auto item : infoMap) {
                napi_value jsObject;
                napi_value jsUuid;
                napi_value jsPssh;
                napi_create_object(mapRef->env_, &jsObject);
                napi_create_string_utf8(mapRef->env_, item.first.c_str(), NAPI_AUTO_LENGTH, &jsUuid);
                napi_set_named_property(mapRef->env_, jsObject, "uuid", jsUuid);

                status = napi_create_array_with_length(mapRef->env_, item.second.size(), &jsPssh);
                for (uint32_t i = 0; i < item.second.size(); i++) {
                    napi_value number = nullptr;
                    (void)napi_create_uint32(mapRef->env_, item.second[i], &number);
                    (void)napi_set_element(mapRef->env_, jsPssh, i, number);
                }
                napi_set_named_property(mapRef->env_, jsObject, "pssh", jsPssh);
                napi_set_element(mapRef->env_, napiMap, index, jsObject);
                index++;
            }

            const int32_t argCount = 1;
            napi_value args[argCount] = { napiMap };
            napi_value result = nullptr;
            status = napi_call_function(mapRef->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };

    struct PropertyInt : public Base {
        std::map<std::string, int32_t> valueMap;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> propertyIntRef = callback.lock();
            CHECK_AND_RETURN_LOG(propertyIntRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(propertyIntRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(propertyIntRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(propertyIntRef->env_, propertyIntRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value args[1] = {nullptr};
            napi_create_object(propertyIntRef->env_, &args[0]);
            for (auto &it : valueMap) {
                CommonNapi::SetPropertyInt32(propertyIntRef->env_, args[0], it.first, it.second);
            }

            napi_value result = nullptr;
            status = napi_call_function(propertyIntRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    struct StateChange : public Base {
        std::string state = "";
        int32_t reason = 0;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> stateChangeRef = callback.lock();
            CHECK_AND_RETURN_LOG(stateChangeRef != nullptr,
                "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(stateChangeRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr,
                "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(stateChangeRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(stateChangeRef->env_, stateChangeRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            const int32_t argCount = 2;
            // callback: (state: AVPlayerState, reason: StateChangeReason)
            napi_value args[argCount] = {nullptr};
            (void)napi_create_string_utf8(stateChangeRef->env_, state.c_str(), NAPI_AUTO_LENGTH, &args[0]);
            (void)napi_create_int32(stateChangeRef->env_, reason, &args[1]);

            napi_value result = nullptr;
            status = napi_call_function(stateChangeRef->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    static void CompleteCallback(napi_env env, NapiCallback::Base *jsCb)
    {
        ON_SCOPE_EXIT(0) {
            delete jsCb;
        };

        uv_loop_s *loop = nullptr;
        napi_get_uv_event_loop(env, &loop);
        CHECK_AND_RETURN_LOG(loop != nullptr, "Fail to napi_get_uv_event_loop");

        uv_work_t *work = new(std::nothrow) uv_work_t;
        CHECK_AND_RETURN_LOG(work != nullptr, "Fail to new uv_work_t");

        work->data = reinterpret_cast<void *>(jsCb);
        // async callback, jsWork and jsWork->data should be heap object.
        int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {}, [] (uv_work_t *work, int status) {
            CHECK_AND_RETURN_LOG(work != nullptr, "Work thread is nullptr");
            (void)status;
            NapiCallback::Base *cb = reinterpret_cast<NapiCallback::Base *>(work->data);
            if (cb != nullptr) {
                MEDIA_LOGD("JsCallBack %{public}s, uv_queue_work_with_qos start", cb->callbackName.c_str());
                cb->UvWork();
                delete cb;
            }
            delete work;
        }, uv_qos_user_initiated);
        if (ret != 0) {
            MEDIA_LOGE("Failed to execute libuv work queue");
            delete jsCb;
            delete work;
        }
        CANCEL_SCOPE_EXIT_GUARD(0);
    }

    struct TrackChange : public Base {
        int32_t number = 0;
        bool isSelect = false;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> trackChangeRef = callback.lock();
            CHECK_AND_RETURN_LOG(trackChangeRef != nullptr, "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(trackChangeRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr, "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(trackChangeRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(trackChangeRef->env_, trackChangeRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            const int32_t argCount = 2; // 2 prapm, callback: (index: number, isSelect: boolean)
            napi_value args[argCount] = {nullptr};
            (void)napi_create_int32(trackChangeRef->env_, number, &args[0]);
            (void)napi_get_boolean(trackChangeRef->env_, isSelect, &args[1]);

            napi_value result = nullptr;
            status = napi_call_function(trackChangeRef->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    struct DeviceChangeNapi : public Base {
        AudioStandard::DeviceInfo deviceInfo;
        int32_t reason;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> deviceChangeRef = callback.lock();
            CHECK_AND_RETURN_LOG(deviceChangeRef != nullptr, "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(deviceChangeRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr, "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(deviceChangeRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(deviceChangeRef->env_, deviceChangeRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            constexpr size_t argCount = 1;
            napi_value args[argCount] = {};
            napi_create_object(deviceChangeRef->env_, &args[0]);
            napi_value deviceObj = nullptr;
            status = CommonNapi::SetValueDeviceInfo(deviceChangeRef->env_, deviceInfo, deviceObj);
            CHECK_AND_RETURN_LOG(status == napi_ok && deviceObj != nullptr,
                " fail to convert to jsobj");
            napi_set_named_property(deviceChangeRef->env_, args[0], "devices", deviceObj);

            bool res = CommonNapi::SetPropertyInt32(deviceChangeRef->env_, args[0], "changeReason",
                static_cast<const int32_t> (reason));
            CHECK_AND_RETURN_LOG(res && deviceObj != nullptr,
                " fail to convert to jsobj");

            napi_value result = nullptr;
            status = napi_call_function(deviceChangeRef->env_, nullptr, jsCallback, argCount, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok, "%{public}s fail to napi_call_function", callbackName.c_str());
        }
    };

    struct TrackInfoUpdate : public Base {
        std::vector<Format> trackInfo;
        void UvWork() override
        {
            std::shared_ptr<AutoRef> trackInfoRef = callback.lock();
            CHECK_AND_RETURN_LOG(trackInfoRef != nullptr, "%{public}s AutoRef is nullptr", callbackName.c_str());

            napi_handle_scope scope = nullptr;
            napi_open_handle_scope(trackInfoRef->env_, &scope);
            CHECK_AND_RETURN_LOG(scope != nullptr, "%{public}s scope is nullptr", callbackName.c_str());
            ON_SCOPE_EXIT(0) {
                napi_close_handle_scope(trackInfoRef->env_, scope);
            };

            napi_value jsCallback = nullptr;
            napi_status status = napi_get_reference_value(trackInfoRef->env_, trackInfoRef->cb_, &jsCallback);
            CHECK_AND_RETURN_LOG(status == napi_ok && jsCallback != nullptr,
                "%{public}s failed to napi_get_reference_value", callbackName.c_str());

            napi_value array = nullptr;
            (void)napi_create_array_with_length(trackInfoRef->env_, trackInfo.size(), &array);

            for (uint32_t i = 0; i < trackInfo.size(); i++) {
                napi_value trackDescription = nullptr;
                trackDescription = CommonNapi::CreateFormatBuffer(trackInfoRef->env_, trackInfo[i]);
                (void)napi_set_element(trackInfoRef->env_, array, i, trackDescription);
            }

            napi_value result = nullptr;
            napi_value args[1] = {array};
            status = napi_call_function(trackInfoRef->env_, nullptr, jsCallback, 1, args, &result);
            CHECK_AND_RETURN_LOG(status == napi_ok,
                "%{public}s failed to napi_call_function", callbackName.c_str());
        }
    };
};

AVPlayerCallback::AVPlayerCallback(napi_env env, AVPlayerNotify *listener)
    : env_(env), listener_(listener)
{
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    onInfoFuncs_[INFO_TYPE_STATE_CHANGE] = &AVPlayerCallback::OnStateChangeCb;
    onInfoFuncs_[INFO_TYPE_VOLUME_CHANGE] = &AVPlayerCallback::OnVolumeChangeCb;
    onInfoFuncs_[INFO_TYPE_SEEKDONE] = &AVPlayerCallback::OnSeekDoneCb;
    onInfoFuncs_[INFO_TYPE_SPEEDDONE] = &AVPlayerCallback::OnSpeedDoneCb;
    onInfoFuncs_[INFO_TYPE_BITRATEDONE] = &AVPlayerCallback::OnBitRateDoneCb;
    onInfoFuncs_[INFO_TYPE_POSITION_UPDATE] = &AVPlayerCallback::OnPositionUpdateCb;
    onInfoFuncs_[INFO_TYPE_DURATION_UPDATE] = &AVPlayerCallback::OnDurationUpdateCb;
    onInfoFuncs_[INFO_TYPE_BUFFERING_UPDATE] = &AVPlayerCallback::OnBufferingUpdateCb;
    onInfoFuncs_[INFO_TYPE_MESSAGE] = &AVPlayerCallback::OnMessageCb;
    onInfoFuncs_[INFO_TYPE_RESOLUTION_CHANGE] = &AVPlayerCallback::OnVideoSizeChangedCb;
    onInfoFuncs_[INFO_TYPE_INTERRUPT_EVENT] = &AVPlayerCallback::OnAudioInterruptCb;
    onInfoFuncs_[INFO_TYPE_BITRATE_COLLECT] = &AVPlayerCallback::OnBitRateCollectedCb;
    onInfoFuncs_[INFO_TYPE_EOS] = &AVPlayerCallback::OnEosCb;
    onInfoFuncs_[INFO_TYPE_IS_LIVE_STREAM] = &AVPlayerCallback::NotifyIsLiveStream;
    onInfoFuncs_[INFO_TYPE_SUBTITLE_UPDATE] = &AVPlayerCallback::OnSubtitleUpdateCb;
    onInfoFuncs_[INFO_TYPE_TRACKCHANGE] = &AVPlayerCallback::OnTrackChangedCb;
    onInfoFuncs_[INFO_TYPE_TRACK_INFO_UPDATE] = &AVPlayerCallback::OnTrackInfoUpdate;
    onInfoFuncs_[INFO_TYPE_DRM_INFO_UPDATED] = &AVPlayerCallback::OnDrmInfoUpdatedCb;
    onInfoFuncs_[INFO_TYPE_SET_DECRYPT_CONFIG_DONE] = &AVPlayerCallback::OnSetDecryptConfigDoneCb;
    onInfoFuncs_[INFO_TYPE_AUDIO_DEVICE_CHANGE] = &AVPlayerCallback::OnAudioDeviceChangeCb;
}

void AVPlayerCallback::OnAudioDeviceChangeCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    if (refMap_.find(AVPlayerEvent::EVENT_AUDIO_DEVICE_CHANGE) == refMap_.end()) {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " can not find audio AudioDeviceChange callback!", FAKE_POINTER(this));
        return;
    }

    NapiCallback::DeviceChangeNapi *cb = new(std::nothrow) NapiCallback::DeviceChangeNapi();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new DeviceChangeNapi");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_AUDIO_DEVICE_CHANGE);
    cb->callbackName = AVPlayerEvent::EVENT_AUDIO_DEVICE_CHANGE;

    uint8_t *parcelBuffer = nullptr;
    size_t parcelSize;
    infoBody.GetBuffer(PlayerKeys::AUDIO_DEVICE_CHANGE, &parcelBuffer, parcelSize);
    Parcel parcel;
    parcel.WriteBuffer(parcelBuffer, parcelSize);
    AudioStandard::DeviceInfo deviceInfo;
    deviceInfo.Unmarshalling(parcel);

    int32_t reason;
    infoBody.GetIntValue(PlayerKeys::AUDIO_DEVICE_CHANGE_REASON, reason);

    cb->deviceInfo = deviceInfo;
    cb->reason = reason;

    NapiCallback::CompleteCallback(env_, cb);
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
        MEDIA_LOGW("0x%{public}06" PRIXPTR " can not find error callback!", FAKE_POINTER(this));
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
    MEDIA_LOGD("OnInfo is called, PlayerOnInfoType: %{public}d", type);
    if (onInfoFuncs_.count(type) > 0) {
        (this->*onInfoFuncs_[type])(extra, infoBody);
    } else {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " OnInfo: no member func supporting, %{public}d",
            FAKE_POINTER(this), type);
    }
}

void AVPlayerCallback::NotifyIsLiveStream(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    (void)infoBody;
    if (listener_ != nullptr) {
        listener_->NotifyIsLiveStream();
    }
}

void AVPlayerCallback::OnStateChangeCb(const int32_t extra, const Format &infoBody)
{
    PlayerStates state = static_cast<PlayerStates>(extra);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " Instance OnStateChanged is called, current state: %{public}d",
        FAKE_POINTER(this), state);

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

void AVPlayerCallback::OnVolumeChangeCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    float volumeLevel = 0.0;
    (void)infoBody.GetFloatValue(PlayerKeys::PLAYER_VOLUME_LEVEL, volumeLevel);

    isSetVolume_ = false;
    MEDIA_LOGD("OnVolumeChangeCb in volume=%{public}f", volumeLevel);
    if (refMap_.find(AVPlayerEvent::EVENT_VOLUME_CHANGE) == refMap_.end()) {
        MEDIA_LOGD("can not find vol change callback!");
        return;
    }

    NapiCallback::Double *cb = new(std::nothrow) NapiCallback::Double();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Double");
    cb->callback = refMap_.at(AVPlayerEvent::EVENT_VOLUME_CHANGE);
    cb->callbackName = AVPlayerEvent::EVENT_VOLUME_CHANGE;
    cb->value = static_cast<double>(volumeLevel);
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnSeekDoneCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t currentPositon = extra;
    MEDIA_LOGI("0x%{public}06" PRIXPTR " OnSeekDone is called, currentPositon: %{public}d",
        FAKE_POINTER(this), currentPositon);
    if (refMap_.find(AVPlayerEvent::EVENT_SEEK_DONE) == refMap_.end()) {
        MEDIA_LOGW("0x%{public}06" PRIXPTR " can not find seekdone callback!", FAKE_POINTER(this));
        return;
    }
    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_SEEK_DONE);
    cb->callbackName = AVPlayerEvent::EVENT_SEEK_DONE;
    cb->value = currentPositon;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnSpeedDoneCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t speedMode = extra;
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

void AVPlayerCallback::OnBitRateDoneCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t bitRate = extra;
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

void AVPlayerCallback::OnPositionUpdateCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t position = extra;
    MEDIA_LOGD("OnPositionUpdateCb is called, position: %{public}d", position);

    if (listener_ != nullptr) {
        listener_->NotifyPosition(position);
    }

    if (refMap_.find(AVPlayerEvent::EVENT_TIME_UPDATE) == refMap_.end()) {
        MEDIA_LOGD("can not find timeupdate callback!");
        return;
    }
    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_TIME_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_TIME_UPDATE;
    cb->value = position;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnDurationUpdateCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t duration = extra;
    MEDIA_LOGI("0x%{public}06" PRIXPTR " OnDurationUpdateCb is called, duration: %{public}d",
        FAKE_POINTER(this), duration);

    if (listener_ != nullptr) {
        listener_->NotifyDuration(duration);
    }

    if (refMap_.find(AVPlayerEvent::EVENT_DURATION_UPDATE) == refMap_.end()) {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " can not find duration update callback!", FAKE_POINTER(this));
        return;
    }

    NapiCallback::Int *cb = new(std::nothrow) NapiCallback::Int();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Int");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_DURATION_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_DURATION_UPDATE;
    cb->value = duration;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnSubtitleUpdateCb(const int32_t extra, const Format &infoBody)
{
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    if (refMap_.find(AVPlayerEvent::EVENT_SUBTITLE_TEXT_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find subtitle update callback!");
        return;
    }
    NapiCallback::SubtitleProperty *cb = new(std::nothrow) NapiCallback::SubtitleProperty();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new SubtitleProperty");
    if (infoBody.ContainKey(PlayerKeys::SUBTITLE_TEXT)) {
        (void)infoBody.GetStringValue(PlayerKeys::SUBTITLE_TEXT, cb->text);
    }
    cb->callback = refMap_.at(AVPlayerEvent::EVENT_SUBTITLE_TEXT_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_SUBTITLE_TEXT_UPDATE;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnBufferingUpdateCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    if (refMap_.find(AVPlayerEvent::EVENT_BUFFERING_UPDATE) == refMap_.end()) {
        MEDIA_LOGD("can not find buffering update callback!");
        return;
    }

    int32_t val = 0;
    int32_t bufferingType = -1;
    if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_START))) {
        bufferingType = BUFFERING_START;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_START), val);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_END))) {
        bufferingType = BUFFERING_END;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_END), val);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT))) {
        bufferingType = BUFFERING_PERCENT;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_BUFFERING_PERCENT), val);
    } else if (infoBody.ContainKey(std::string(PlayerKeys::PLAYER_CACHED_DURATION))) {
        bufferingType = CACHED_DURATION;
        (void)infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_CACHED_DURATION), val);
    } else {
        return;
    }

    MEDIA_LOGD("OnBufferingUpdateCb is called, buffering type: %{public}d value: %{public}d", bufferingType, val);
    NapiCallback::IntVec *cb = new(std::nothrow) NapiCallback::IntVec();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new IntVec");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_BUFFERING_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_BUFFERING_UPDATE;
    cb->valueVec.push_back(bufferingType);
    cb->valueVec.push_back(val);
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnMessageCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
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

void AVPlayerCallback::OnVideoSizeChangedCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t width = 0;
    int32_t height = 0;
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_WIDTH, width);
    (void)infoBody.GetIntValue(PlayerKeys::PLAYER_HEIGHT, height);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " OnVideoSizeChangedCb is called, width = %{public}d, height = %{public}d",
        FAKE_POINTER(this), width, height);

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

void AVPlayerCallback::OnAudioInterruptCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
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

void AVPlayerCallback::OnBitRateCollectedCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
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

void AVPlayerCallback::OnDrmInfoUpdatedCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    MEDIA_LOGI("AVPlayerCallback OnDrmInfoUpdatedCb is called");
    if (refMap_.find(AVPlayerEvent::EVENT_DRM_INFO_UPDATE) == refMap_.end()) {
        MEDIA_LOGW("can not find drm info updated callback!");
        return;
    }
    if (!infoBody.ContainKey(std::string(PlayerKeys::PLAYER_DRM_INFO_ADDR))) {
        MEDIA_LOGW("there's no drminfo-update drm_info_addr key");
        return;
    }
    if (!infoBody.ContainKey(std::string(PlayerKeys::PLAYER_DRM_INFO_COUNT))) {
        MEDIA_LOGW("there's no drminfo-update drm_info_count key");
        return;
    }

    uint8_t *drmInfoAddr = nullptr;
    size_t size  = 0;
    int32_t infoCount = 0;
    infoBody.GetBuffer(std::string(PlayerKeys::PLAYER_DRM_INFO_ADDR), &drmInfoAddr, size);
    CHECK_AND_RETURN_LOG(drmInfoAddr != nullptr && size > 0, "get drminfo buffer failed");
    infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_DRM_INFO_COUNT), infoCount);
    CHECK_AND_RETURN_LOG(infoCount > 0, "get drminfo count is illegal");

    std::multimap<std::string, std::vector<uint8_t>> drmInfoMap;
    DrmInfoItem *drmInfos = reinterpret_cast<DrmInfoItem*>(drmInfoAddr);
    CHECK_AND_RETURN_LOG(drmInfos != nullptr, "cast drmInfos nullptr");
    for (int32_t i = 0; i < infoCount; i++) {
        DrmInfoItem temp = drmInfos[i];
        std::stringstream ssConverter;
        std::string uuid;
        for (uint32_t index = 0; index < DrmConstant::DRM_MAX_M3U8_DRM_UUID_LEN; index++) {
            ssConverter << std::hex << static_cast<int32_t>(temp.uuid[index]);
            uuid = ssConverter.str();
        }
        std::vector<uint8_t> pssh(temp.pssh, temp.pssh + temp.psshLen);
        drmInfoMap.insert({ uuid, pssh });
    }

    if (listener_ != nullptr) {
        listener_->NotifyDrmInfoUpdated(drmInfoMap);
    }
    NapiCallback::ObjectArray *cb = new(std::nothrow) NapiCallback::ObjectArray();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new ObjectArray");
    cb->callback = refMap_.at(AVPlayerEvent::EVENT_DRM_INFO_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_DRM_INFO_UPDATE;
    cb->infoMap = drmInfoMap;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnSetDecryptConfigDoneCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    MEDIA_LOGI("AVPlayerCallback OnSetDecryptConfigDoneCb is called");
    if (refMap_.find(AVPlayerEvent::EVENT_SET_DECRYPT_CONFIG_DONE) == refMap_.end()) {
        MEDIA_LOGW("can not find SetDecryptConfig Done callback!");
        return;
    }

    NapiCallback::Base *cb = new(std::nothrow) NapiCallback::Base();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Base");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_SET_DECRYPT_CONFIG_DONE);
    cb->callbackName = AVPlayerEvent::EVENT_SET_DECRYPT_CONFIG_DONE;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnEosCb(const int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    CHECK_AND_RETURN_LOG(isloaded_.load(), "current source is unready");
    int32_t isLooping = extra;
    MEDIA_LOGI("0x%{public}06" PRIXPTR " OnEndOfStream is called, isloop: %{public}d", FAKE_POINTER(this), isLooping);
    if (refMap_.find(AVPlayerEvent::EVENT_END_OF_STREAM) == refMap_.end()) {
        MEDIA_LOGW("0x%{public}06" PRIXPTR " can not find EndOfStream callback!", FAKE_POINTER(this));
        return;
    }

    NapiCallback::Base *cb = new(std::nothrow) NapiCallback::Base();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new Base");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_END_OF_STREAM);
    cb->callbackName = AVPlayerEvent::EVENT_END_OF_STREAM;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnTrackChangedCb(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    int32_t index = -1;
    int32_t isSelect = -1;
    infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), index);
    infoBody.GetIntValue(std::string(PlayerKeys::PLAYER_IS_SELECT), isSelect);
    MEDIA_LOGI("OnTrackChangedCb index %{public}d, isSelect = %{public}d", index, isSelect);
 
    CHECK_AND_RETURN_LOG(refMap_.find(AVPlayerEvent::EVENT_TRACKCHANGE) != refMap_.end(),
        "can not find trackChange callback!");

    NapiCallback::TrackChange *cb = new(std::nothrow) NapiCallback::TrackChange();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new TrackChange");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_TRACKCHANGE);
    cb->callbackName = AVPlayerEvent::EVENT_TRACKCHANGE;
    cb->number = index;
    cb->isSelect = isSelect ? true : false;
    NapiCallback::CompleteCallback(env_, cb);
}

void AVPlayerCallback::OnTrackInfoUpdate(const int32_t extra, const Format &infoBody)
{
    (void)extra;
    std::vector<Format> trackInfo;
    (void)infoBody.GetFormatVector(std::string(PlayerKeys::PLAYER_TRACK_INFO), trackInfo);
    MEDIA_LOGI("OnTrackInfoUpdate callback");
 
    CHECK_AND_RETURN_LOG(refMap_.find(AVPlayerEvent::EVENT_TRACK_INFO_UPDATE) != refMap_.end(),
        "can not find trackInfoUpdate callback!");

    NapiCallback::TrackInfoUpdate *cb = new(std::nothrow) NapiCallback::TrackInfoUpdate();
    CHECK_AND_RETURN_LOG(cb != nullptr, "failed to new TrackInfoUpdate");

    cb->callback = refMap_.at(AVPlayerEvent::EVENT_TRACK_INFO_UPDATE);
    cb->callbackName = AVPlayerEvent::EVENT_TRACK_INFO_UPDATE;
    cb->trackInfo = trackInfo;
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