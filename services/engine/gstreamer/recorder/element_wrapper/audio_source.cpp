/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_source.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioSource"};
}

namespace OHOS {
namespace Media {
int32_t AudioSource::Init()
{
    gstElem_ = gst_element_factory_make("audiocapturesrc", name_.c_str());
    if (gstElem_ == nullptr) {
        MEDIA_LOGE("Create audiosource gst element failed! sourceId: %{public}d", desc_.handle_);
        return MSERR_INVALID_OPERATION;
    }
    g_object_set(gstElem_, "source-type", desc_.type_, nullptr);

    return MSERR_OK;
}

int32_t AudioSource::Configure(const RecorderParam &recParam)
{
    int ret = MSERR_OK;
    switch (recParam.type) {
        case RecorderPublicParamType::AUD_SAMPLERATE:
            ret = ConfigAudioSampleRate(recParam);
            break;
        case RecorderPublicParamType::AUD_CHANNEL:
            ret = ConfigAudioChannels(recParam);
            break;
        case RecorderPublicParamType::AUD_BITRATE:
            ret = ConfigAudioBitRate(recParam);
            break;
        case RecorderPrivateParamType::APP_INFO:
            ret = ConfigAppInfo(recParam);
            break;
        default:
            break;
    }

    return ret;
}

int32_t AudioSource::ConfigAudioSampleRate(const RecorderParam &recParam)
{
    const AudSampleRate &param = static_cast<const AudSampleRate &>(recParam);
    if (param.sampleRate <= 0) {
        MEDIA_LOGE("The required audio sample rate %{public}d invalid.", param.sampleRate);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGI("Set audio sample rate: %{public}d", param.sampleRate);
    g_object_set(gstElem_, "sample-rate", static_cast<uint32_t>(param.sampleRate), nullptr);

    MarkParameter(static_cast<int32_t>(RecorderPublicParamType::AUD_SAMPLERATE));
    sampleRate_ = param.sampleRate;
    return MSERR_OK;
}

int32_t AudioSource::ConfigAudioChannels(const RecorderParam &recParam)
{
    const AudChannel &param = static_cast<const AudChannel &>(recParam);
    if (param.channel <= 0) {
        MEDIA_LOGE("The required audio channels %{public}d is invalid", param.channel);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGI("Set audio channels: %{public}d", param.channel);
    g_object_set(gstElem_, "channels", static_cast<uint32_t>(param.channel), nullptr);

    MarkParameter(static_cast<int32_t>(RecorderPublicParamType::AUD_CHANNEL));
    channels_ = param.channel;
    return MSERR_OK;
}

int32_t AudioSource::ConfigAudioBitRate(const RecorderParam &recParam)
{
    const AudBitRate &param = static_cast<const AudBitRate &>(recParam);
    if (param.bitRate <= 0) {
        MEDIA_LOGE("The required audio bitrate %{public}d is invalid", param.bitRate);
        return MSERR_INVALID_VAL;
    }
    MEDIA_LOGI("Set audio bitrate: %{public}d", param.bitRate);
    g_object_set(gstElem_, "bitrate", static_cast<uint32_t>(param.bitRate), nullptr);

    MarkParameter(static_cast<int32_t>(RecorderPublicParamType::AUD_BITRATE));
    bitRate_ = param.bitRate;
    return MSERR_OK;
}

int32_t AudioSource::ConfigAppInfo(const RecorderParam &recParam)
{
    const AppInfo &param = static_cast<const AppInfo &>(recParam);
    g_object_set(gstElem_, "token-id", param.appTokenId_, nullptr);
    g_object_set(gstElem_, "full-token-id", param.appFullTokenId_, nullptr);
    g_object_set(gstElem_, "app-uid", param.appUid_, nullptr);
    g_object_set(gstElem_, "app-pid", param.appPid_, nullptr);

    MEDIA_LOGI("Set app info done");
    MarkParameter(static_cast<int32_t>(RecorderPrivateParamType::APP_INFO));
    return MSERR_OK;
}

int32_t AudioSource::CheckConfigReady()
{
    std::set<int32_t> expectedParam = {
        RecorderPublicParamType::AUD_SAMPLERATE,
        RecorderPublicParamType::AUD_CHANNEL,
        RecorderPrivateParamType::APP_INFO,
    };

    if (!CheckAllParamsConfigured(expectedParam)) {
        MEDIA_LOGE("audiosource required parameter not configured completely, failed !");
        return MSERR_INVALID_OPERATION;
    }

    int32_t isSupportedParams = 0;
    g_object_get(gstElem_, "supported-audio-params", &isSupportedParams, nullptr);
    CHECK_AND_RETURN_RET_LOG(isSupportedParams != 0, MSERR_UNSUPPORT_AUD_PARAMS, "unsupport audio params");

    return MSERR_OK;
}

int32_t AudioSource::Prepare()
{
    MEDIA_LOGD("audio source prepare enter");
    g_object_set(gstElem_, "bypass-audio-service", FALSE, nullptr);
    return MSERR_OK;
}

int32_t AudioSource::Stop()
{
    MEDIA_LOGD("audio source stop enter");
    g_object_set(gstElem_, "bypass-audio-service", TRUE, nullptr);
    return MSERR_OK;
}

void AudioSource::Dump()
{
    MEDIA_LOGI("Audio [sourceId = 0x%{public}x]: sample rate = %{public}d, "
               "channels = %{public}d, bitRate = %{public}d",
               desc_.handle_, sampleRate_, channels_, bitRate_);
}

int32_t AudioSource::GetParameter(RecorderParam &recParam)
{
    GValue val = G_VALUE_INIT;
    G_VALUE_TYPE(&val) = G_TYPE_POINTER;
    if (recParam.type == RecorderPrivateParamType::AUDIO_CAPTURE_CAHNGE_INFO) {
        return GetAudioCaptureChangeInfoToParam(recParam);
    } else if (recParam.type == RecorderPrivateParamType::MAX_AMPLITUDE) {
        CHECK_AND_RETURN_RET_LOG(
            g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "max-Amplitude") != nullptr,
            MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
        int32_t maxAmplitude;
        g_object_get(gstElem_, "max-Amplitude", &maxAmplitude, nullptr);
        CHECK_AND_RETURN_RET_LOG(maxAmplitude > 0, MSERR_UNSUPPORT_AUD_PARAMS, "get maxAmplitude  failed");
        AudioMaxAmplitudeParam &param = (AudioMaxAmplitudeParam &)recParam;
        param.maxAmplitude_ = maxAmplitude;
    } else if (recParam.type == RecorderPrivateParamType::MICRO_PHONE_DESCRIPTOR_LENGTH) {
        CHECK_AND_RETURN_RET_LOG(
            g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_),
            "microphonedescriptors-length") != nullptr,
            MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
        int32_t mic_Length;
        g_object_get(gstElem_, "microphonedescriptors-length", &mic_Length, nullptr);
        CHECK_AND_RETURN_RET_LOG(mic_Length > 0, MSERR_UNSUPPORT_AUD_PARAMS,
            "get microPhoneDescriptors length  failed");
        MicrophoneDescriptorSizeParam &param = (MicrophoneDescriptorSizeParam &)recParam;
        param.length_ = mic_Length;
    } else if (recParam.type == RecorderPrivateParamType::MICRO_PHONE_DESCRIPTOR) {
        return GetMicInfoToParam(recParam);
    }
    return MSERR_OK;
}

int32_t AudioSource::GetAudioStreamInfoToParam(AudioRecordChangeInfoParam &param)
{
    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "sampling-Rate") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "sampling-Rate", &param.samplingRate_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "encoding") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "encoding", &param.encoding_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "audio-format") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "audio-format", &param.format_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "audio-channels") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "audio-channels", &param.channels_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "network-Id") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "network-Id", &param.networkId_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "display-Name") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "display-Name", &param.displayName_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "interrupt-GroupId") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "interrupt-GroupId", &param.interruptGroupId_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "volume-GroupId") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "volume-GroupId", &param.volumeGroupId_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "isLowLatency-Device") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    gboolean isLowLatencyDevice = false;
    g_object_get(gstElem_, "isLowLatency-Device", &isLowLatencyDevice, nullptr);
    param.isLowLatencyDevice_ = isLowLatencyDevice;

    return MSERR_OK;
}

int32_t AudioSource::GetAudioDeviceInfoToParam(AudioRecordChangeInfoParam &param)
{
    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "device-Type") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "device-Type", &param.deviceType_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "device-Role") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "device-Role", &param.deviceRole_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "device-Id") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "device-Id", &param.deviceId_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "channel-Masks") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "channel-Masks", &param.channelMasks_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "channel-Index-Masks") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "channel-Index-Masks", &param.channelIndexMasks_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "device-Name") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "device-Name", &param.deviceName_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "mac-Address") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "mac-Address", &param.macAddress_, nullptr);

    GetAudioStreamInfoToParam(param);
    return MSERR_OK;
}

int32_t AudioSource::GetAudioCaptureChangeInfoToParam(RecorderParam &recParam)
{
    AudioRecordChangeInfoParam &param = (AudioRecordChangeInfoParam &)recParam;
    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "creater-uid") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "creater-uid", &param.createrUID_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "client-uid") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "client-uid", &param.clientUID_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "session-id") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "session-id", &param.sessionId_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "input-source") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "input-source", &param.inputSource_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "capturer-flag") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "capturer-flag", &param.capturerFlags_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "recorder-state") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    g_object_get(gstElem_, "recorder-state", &param.aRecorderState_, nullptr);

    GetAudioDeviceInfoToParam(param);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "muted") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no audiocapturechange property!");
    gboolean muted = false;
    g_object_get(gstElem_, "muted", &muted, nullptr);
    param.muted_ = muted;

    return MSERR_OK;
}

int32_t AudioSource::GetMicInfoToParam(RecorderParam &recParam)
{
    MicrophoneDescriptorParam &param = (MicrophoneDescriptorParam &)recParam;
    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "mic-id") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "mic-id", &param.micId_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "mic-device-type") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "mic-device-type", &param.deviceType_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "sensitivity") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "sensitivity", &param.sensitivity_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "position-x") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "position-x", &param.position_x_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "position-y") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "position-y", &param.position_y_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "position_z") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "position_z", &param.position_z_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "orientation-x") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "orientation-x", &param.orientation_x_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "orientation-y") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "orientation-y", &param.orientation_y_, nullptr);

    CHECK_AND_RETURN_RET_LOG(
        g_object_class_find_property(G_OBJECT_GET_CLASS((GObject *)gstElem_), "orientation-z") != nullptr,
        MSERR_INVALID_OPERATION, "gstelem has no MicrophoneDescriptorParam property!");
    g_object_get(gstElem_, "orientation-z", &param.orientation_z_, nullptr);

    return MSERR_OK;
}

REGISTER_RECORDER_ELEMENT(AudioSource);
} // namespace Media
} // namespace OHOS
