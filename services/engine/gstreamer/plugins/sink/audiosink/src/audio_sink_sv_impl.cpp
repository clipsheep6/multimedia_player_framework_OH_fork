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

#include "audio_sink_sv_impl.h"
#include <vector>
#include <unistd.h>
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "param_wrapper.h"
#include "player_xcollie.h"
#include "scope_guard.h"
#include "audio_effect.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioSinkSvImpl"};
    const std::string DEFAULT_CAPS = "audio/x-raw, format = (string) S16LE, layout = (string) interleaved";
}

namespace OHOS {
namespace Media {
AudioRendererMediaCallback::AudioRendererMediaCallback(GstBaseSink *audioSink)
    : audioSink_(audioSink), taskQue_("AudioCallback")
{
    (void)taskQue_.Start();
}

AudioRendererMediaCallback::~AudioRendererMediaCallback()
{
    (void)taskQue_.Stop();
}

void AudioRendererMediaCallback::SaveInterruptCallback(InterruptCbFunc interruptCb)
{
    interruptCb_ = interruptCb;
}

void AudioRendererMediaCallback::SaveStateCallback(StateCbFunc stateCb)
{
    stateCb_ = stateCb;
}

void AudioRendererMediaCallback::SaveFirstFrameCallback(FirstFrameCbFunc firstFrameCb)
{
    firstFrameCb_ = firstFrameCb;
}

void AudioRendererMediaCallback::SaveDeviceChangeCallback(DeviceChangeFunc deviceCb)
{
    deviceCb_ = deviceCb;
}

AudioServiceDiedCallback::AudioServiceDiedCallback(GstBaseSink *audioSink) : audioSink_(audioSink)
{
    MEDIA_LOGD("AudioServiceDiedCallback create");
}

void AudioServiceDiedCallback::SaveAudioPolicyServiceDiedCb(AudioDiedCbFunc diedCb)
{
    diedCb_ = diedCb;
}

void AudioServiceDiedCallback::OnAudioPolicyServiceDied()
{
    CHECK_AND_RETURN_LOG(diedCb_ != nullptr, "audio policy died cb is null");
    diedCb_(audioSink_);
}

void AudioRendererMediaCallback::OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent)
{
    auto task = std::make_shared<TaskHandler<void>>([this, interruptEvent] {
        if (interruptCb_ != nullptr) {
            interruptCb_(audioSink_, interruptEvent.eventType, interruptEvent.forceType, interruptEvent.hintType);
        }
    });
    (void)taskQue_.EnqueueTask(task);
}

void AudioRendererMediaCallback::OnStateChange(const AudioStandard::RendererState state,
    const AudioStandard::StateChangeCmdType cmdType)
{
    MEDIA_LOGD("RenderState is %{public}d, type is %{public}d",
        static_cast<int32_t>(state), static_cast<int32_t>(cmdType));
    if (cmdType == AudioStandard::StateChangeCmdType::CMD_FROM_SYSTEM) {
        auto task = std::make_shared<TaskHandler<void>>([this, state] {
            if (stateCb_ != nullptr) {
                stateCb_(audioSink_, static_cast<guint>(state));
            }
        });
        (void)taskQue_.EnqueueTask(task);
    }
}

void AudioRendererMediaCallback::OnOutputDeviceChange(const AudioStandard::DeviceInfo &deviceInfo,
    const AudioStandard::AudioStreamDeviceChangeReason reason)
{
    MEDIA_LOGD("OnOutputDeviceChange reason is %{public}d", static_cast<int32_t>(reason));
    auto task = std::make_shared<TaskHandler<void>>([this, deviceInfo, reason] {
        if (deviceCb_ != nullptr) {
            gpointer gstDeviceInfo;
            gstDeviceInfo = new AudioStandard::DeviceInfo(deviceInfo);
            // kwtest
            deviceCb_(audioSink_, gstDeviceInfo, static_cast<const gint32>(reason));
        }
    });
    (void)taskQue_.EnqueueTask(task);
}

void AudioRendererMediaCallback::OnFirstFrameWriting(uint64_t latency)
{
    auto task = std::make_shared<TaskHandler<void>>([this, latency] {
        if (firstFrameCb_ != nullptr) {
            firstFrameCb_(audioSink_, static_cast<gulong>(latency));
        }
    });
    (void)taskQue_.EnqueueTask(task);
}

AudioSinkSvImpl::AudioSinkSvImpl(GstBaseSink *sink)
    : audioSink_(sink)
{
    audioRendererMediaCallback_ = std::make_shared<AudioRendererMediaCallback>(sink);
    audioServiceDiedCallback_ = std::make_shared<AudioServiceDiedCallback>(sink);
    SetAudioDumpBySysParam();
}

AudioSinkSvImpl::~AudioSinkSvImpl()
{
    if (audioRenderer_ != nullptr) {
        LISTENER((void)audioRenderer_->Release(); audioRenderer_ = nullptr,
            "AudioRenderer::Release", PlayerXCollie::timerTimeout)
    }
    if (dumpFile_ != nullptr) {
        (void)fclose(dumpFile_);
        dumpFile_ = nullptr;
    }
}

GstCaps *AudioSinkSvImpl::GetCaps()
{
    GstCaps *caps = gst_caps_from_string(DEFAULT_CAPS.c_str());
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "caps is null");
    InitChannelRange(caps);
    InitRateRange(caps);
    return caps;
}

void AudioSinkSvImpl::InitChannelRange(GstCaps *caps) const
{
    CHECK_AND_RETURN_LOG(caps != nullptr, "caps is null");
    std::vector<AudioStandard::AudioChannel> supportedChannelsList = AudioStandard::
                                                                     AudioRenderer::GetSupportedChannels();
    GValue list = G_VALUE_INIT;
    (void)g_value_init(&list, GST_TYPE_LIST);
    for (auto channel : supportedChannelsList) {
        GValue value = G_VALUE_INIT;
        (void)g_value_init(&value, G_TYPE_INT);
        g_value_set_int(&value, channel);
        gst_value_list_append_value(&list, &value);
        g_value_unset(&value);
    }
    gst_caps_set_value(caps, "channels", &list);
    g_value_unset(&list);
}

void AudioSinkSvImpl::InitRateRange(GstCaps *caps) const
{
    CHECK_AND_RETURN_LOG(caps != nullptr, "caps is null");
    std::vector<AudioStandard::AudioSamplingRate> supportedSampleList = AudioStandard::
                                                                        AudioRenderer::GetSupportedSamplingRates();
    GValue list = G_VALUE_INIT;
    (void)g_value_init(&list, GST_TYPE_LIST);
    for (auto rate : supportedSampleList) {
        GValue value = G_VALUE_INIT;
        (void)g_value_init(&value, G_TYPE_INT);
        g_value_set_int(&value, rate);
        gst_value_list_append_value(&list, &value);
        g_value_unset(&value);
    }
    gst_caps_set_value(caps, "rate", &list);
    g_value_unset(&list);
}

void AudioSinkSvImpl::SetMuteVolumeBySysParam()
{
    std::string onMute;
    int32_t res = OHOS::system::GetStringParameter("sys.media.set.mute", onMute, "");
    if (res == 0 && !onMute.empty()) {
        if (onMute == "TRUE") {
            isMute_ = true;
            (void)SetVolume(0.0);
            MEDIA_LOGD("SetVolume as 0");
        } else if (onMute == "FALSE") {
            isMute_ = false;
            (void)SetVolume(1.0);
            MEDIA_LOGD("SetVolume as 1");
        }
    }
}

int32_t AudioSinkSvImpl::SetVolume(float volume)
{
    MediaTrace trace("AudioSink::SetVolume");
    MEDIA_LOGD("audioRenderer SetVolume(%{public}lf) In", volume);
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED, "audioRenderer_ is nullptr");
    volume = (isMute_ == false) ? volume : 0.0;
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->SetVolume(volume), "AudioRenderer::SetVolume", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET_LOG(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED, "audio server setvolume failed!");
    MEDIA_LOGD("audioRenderer SetVolume(%{public}lf) Out", volume);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::SetRendererInfo(int32_t desc, int32_t rendererFlags)
{
    int32_t contentType = (static_cast<uint32_t>(desc) & 0x0000FFFF);
    int32_t streamUsage = static_cast<uint32_t>(desc) >> AudioStandard::RENDERER_STREAM_USAGE_SHIFT;
    rendererOptions_.rendererInfo.contentType = static_cast<AudioStandard::ContentType>(contentType);
    rendererOptions_.rendererInfo.streamUsage = static_cast<AudioStandard::StreamUsage>(streamUsage);
    rendererOptions_.rendererInfo.rendererFlags = rendererFlags;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetVolume(float &volume)
{
    MEDIA_LOGD("GetVolume");
    CHECK_AND_RETURN_RET_LOG(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED, "audioRenderer_ is nullptr");
    XcollieTimer xCollie("AudioRenderer::GetVolume", PlayerXCollie::timerTimeout);
    volume = audioRenderer_->GetVolume();
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMaxVolume(float &volume)
{
    MEDIA_LOGD("GetMaxVolume");
    volume = 1.0; // audioRenderer maxVolume
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinVolume(float &volume)
{
    MEDIA_LOGD("GetMinVolume");
    volume = 0.0; // audioRenderer minVolume
    return MSERR_OK;
}

bool AudioSinkSvImpl::DisableAudioEffectBySysParam() const
{
    std::string cmd;
    int32_t ret = OHOS::system::GetStringParameter("sys.media.audio.effect.disable", cmd, "");
    if (ret == 0 && !cmd.empty()) {
        return cmd == "TRUE" ? TRUE : FALSE;
    }
    return FALSE;
}

int32_t AudioSinkSvImpl::Prepare(int32_t appUid, int32_t appPid, uint32_t appTokenId)
{
    MediaTrace trace("AudioSink::Prepare");
    MEDIA_LOGD("audioRenderer Prepare In");
    AudioStandard::AppInfo appInfo = {};
    appInfo.appUid = appUid;
    appInfo.appPid = appPid;
    appInfo.appTokenId = appTokenId;
    rendererOptions_.streamInfo.samplingRate = AudioStandard::SAMPLE_RATE_8000;
    rendererOptions_.streamInfo.encoding = AudioStandard::ENCODING_PCM;
    rendererOptions_.streamInfo.format = AudioStandard::SAMPLE_S16LE;
    rendererOptions_.streamInfo.channels = AudioStandard::MONO;
    LISTENER(audioRenderer_ = AudioStandard::AudioRenderer::Create(rendererOptions_, appInfo),
        "AudioRenderer::Create", false, 15) // 15 timeout for create
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    SetMuteVolumeBySysParam();
    OHOS::AudioStandard::AudioEffectMode effectMode = DisableAudioEffectBySysParam() ?
        OHOS::AudioStandard::AudioEffectMode::EFFECT_NONE :
        OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT;
    audioRenderer_->SetAudioEffectMode(effectMode);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " audioRenderer Prepare Out", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Start()
{
    MediaTrace trace("AudioSink::Start");
    MEDIA_LOGD("audioRenderer Start In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    bool ret = false;
    LISTENER(
        ret = audioRenderer_->Start(),
        "AudioRenderer::Start",
        PlayerXCollie::timerTimeout)
    MEDIA_LOGD("0x%{public}06" PRIXPTR " audioRenderer Start Out", FAKE_POINTER(this));
    return ret ? MSERR_OK : MSERR_INVALID_OPERATION;
}

int32_t AudioSinkSvImpl::Stop()
{
    MediaTrace trace("AudioSink::Stop");
    MEDIA_LOGD("audioRenderer Stop In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    LISTENER((void)audioRenderer_->Stop(), "AudioRenderer::Stop", PlayerXCollie::timerTimeout)
    MEDIA_LOGI("0x%{public}06" PRIXPTR " audioRenderer Stop Out", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Pause()
{
    MediaTrace trace("AudioSink::Pause");
    MEDIA_LOGD("audioRenderer Pause In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    if (audioRenderer_->GetStatus() == OHOS::AudioStandard::RENDERER_RUNNING) {
        LISTENER(
            bool ret = audioRenderer_->Pause();
            if (ret == false) {
                MEDIA_LOGE("audio Renderer Pause failed!");
            }
            CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED),
            "AudioRenderer::Pause",
            PlayerXCollie::timerTimeout)
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " audioRenderer Pause Out", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::PauseTransitent()
{
    MediaTrace trace("AudioSink::PauseTransitent");
    MEDIA_LOGD("audioRenderer PauseTransitent In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    if (audioRenderer_->GetStatus() == OHOS::AudioStandard::RENDERER_RUNNING) {
        LISTENER(
            bool ret = audioRenderer_->PauseTransitent();
            if (ret == false) {
                MEDIA_LOGE("audio Renderer PauseTransitent failed!");
            }
            CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED),
            "AudioRenderer::PauseTransitent",
            PlayerXCollie::timerTimeout)
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instance audioRenderer PauseTransitent Out", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Drain()
{
    MediaTrace trace("AudioSink::Drain");
    MEDIA_LOGD("Drain");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    bool ret = false;
    LISTENER(ret = audioRenderer_->Drain(), "AudioRenderer::Drain", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Flush()
{
    MediaTrace trace("AudioSink::Flush");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    OHOS::AudioStandard::RendererState state = audioRenderer_->GetStatus();
    MEDIA_LOGD("AudioSinkSvImpl Flush in, audioRender state = %{public}d", state);
    if (state == OHOS::AudioStandard::RENDERER_RUNNING || state == OHOS::AudioStandard::RENDERER_PAUSED ||
        state == OHOS::AudioStandard::RENDERER_STOPPED) {
        bool ret = false;
        LISTENER(ret = audioRenderer_->Flush(), "AudioRenderer::Flush", PlayerXCollie::timerTimeout)
        CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    }
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Release()
{
    MediaTrace trace("AudioSink::Release");
    MEDIA_LOGD("audioRenderer Release In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    LISTENER((void)audioRenderer_->Release(), "AudioRenderer::Release", PlayerXCollie::timerTimeout)
    MEDIA_LOGI("0x%{public}06" PRIXPTR " audioRenderer Release Out", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::SetParameters(uint32_t bitsPerSample, uint32_t channels, uint32_t sampleRate)
{
    MediaTrace trace("AudioSink::SetParameters");
    (void)bitsPerSample;
    MEDIA_LOGD("SetParameters in, channels:%{public}d, sampleRate:%{public}d", channels, sampleRate);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);

    AudioStandard::AudioRendererParams params;
    std::vector<AudioStandard::AudioSamplingRate> supportedSampleList = AudioStandard::
                                                                        AudioRenderer::GetSupportedSamplingRates();
    CHECK_AND_RETURN_RET(supportedSampleList.size() > 0, MSERR_AUD_RENDER_FAILED);
    bool isValidSampleRate = false;
    for (auto iter = supportedSampleList.cbegin(); iter != supportedSampleList.end(); ++iter) {
        CHECK_AND_RETURN_RET(static_cast<int32_t>(*iter) > 0, MSERR_AUD_RENDER_FAILED);
        uint32_t supportedSampleRate = static_cast<uint32_t>(*iter);
        if (sampleRate <= supportedSampleRate) {
            params.sampleRate = *iter;
            isValidSampleRate = true;
            break;
        }
    }
    CHECK_AND_RETURN_RET(isValidSampleRate == true, MSERR_UNSUPPORT_AUD_SAMPLE_RATE);

    std::vector<AudioStandard::AudioChannel> supportedChannelsList = AudioStandard::
                                                                     AudioRenderer::GetSupportedChannels();
    CHECK_AND_RETURN_RET(supportedChannelsList.size() > 0, MSERR_AUD_RENDER_FAILED);
    bool isValidChannels = false;
    for (auto iter = supportedChannelsList.cbegin(); iter != supportedChannelsList.end(); ++iter) {
        CHECK_AND_RETURN_RET(static_cast<int32_t>(*iter) > 0, MSERR_AUD_RENDER_FAILED);
        uint32_t supportedChannels = static_cast<uint32_t>(*iter);
        if (channels == supportedChannels) {
            params.channelCount = *iter;
            isValidChannels = true;
            break;
        }
    }
    CHECK_AND_RETURN_RET(isValidChannels == true, MSERR_UNSUPPORT_AUD_CHANNEL_NUM);

    params.sampleFormat = AudioStandard::SAMPLE_S16LE;
    params.encodingType = AudioStandard::ENCODING_PCM;
    MEDIA_LOGD("SetParameters out, channels:%{public}d, sampleRate:%{public}d", params.channelCount, params.sampleRate);
    MEDIA_LOGD("audioRenderer SetParams In");
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->SetParams(params), "AudioRenderer::SetParams", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " audioRenderer SetParams Out", FAKE_POINTER(this));
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate)
{
    (void)bitsPerSample;
    MEDIA_LOGD("GetParameters");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    AudioStandard::AudioRendererParams params;
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->GetParams(params), "AudioRenderer::GetParams", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    channels = params.channelCount;
    sampleRate = params.sampleRate;
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinimumBufferSize(uint32_t &bufferSize)
{
    MEDIA_LOGD("GetMinimumBufferSize");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    size_t size = 0;
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->GetBufferSize(size), "AudioRenderer::GetBufferSize", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    CHECK_AND_RETURN_RET(size > 0, MSERR_AUD_RENDER_FAILED);
    bufferSize = static_cast<uint32_t>(size);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetMinimumFrameCount(uint32_t &frameCount)
{
    MEDIA_LOGD("GetMinimumFrameCount");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    uint32_t count = 0;
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->GetFrameCount(count), "AudioRenderer::GetFrameCount", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    CHECK_AND_RETURN_RET(count > 0, MSERR_AUD_RENDER_FAILED);
    frameCount = count;
    return MSERR_OK;
}

bool AudioSinkSvImpl::Writeable() const
{
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, false);
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->GetStatus(), "AudioRenderer::GetStatus", PlayerXCollie::timerTimeout)
    return ret == AudioStandard::RENDERER_RUNNING;
}

bool AudioSinkSvImpl::IsMediaAudioActive()
{
    bool ret = false;
    LISTENER(
        ret = AudioStandard::AudioStreamManager::GetInstance()->
            IsStreamActive(AudioStandard::AudioVolumeType::STREAM_MUSIC),
        "AudioStreamManager::IsStreamActive",
        PlayerXCollie::timerTimeout)
    return ret;
}

void AudioSinkSvImpl::OnError(std::string errMsg)
{
    if (errorCb_ != nullptr) {
        errorCb_(audioSink_, errMsg);
    }
}

int32_t AudioSinkSvImpl::Write(uint8_t *buffer, size_t size)
{
    MediaTrace trace("AudioSink::Write");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    CHECK_AND_RETURN_RET(buffer != nullptr, MSERR_AUD_RENDER_FAILED);
    CHECK_AND_RETURN_RET(size > 0, MSERR_AUD_RENDER_FAILED);

    size_t bytesWritten = 0;
    LISTENER(
        while (bytesWritten < size) {
            MediaTrace trace("AudioSink::Write:" + std::to_string(size - bytesWritten));
            int32_t bytesSingle = audioRenderer_->Write(buffer + bytesWritten, size - bytesWritten);
            if (bytesSingle <= 0) {
                MEDIA_LOGE("[AudioSinkSvImpl] audioRenderer write failed, drop an audio packet!");
                return MSERR_OK;
            }
            DumpAudioBuffer(buffer, bytesWritten, static_cast<size_t>(bytesSingle));
            bytesWritten += static_cast<size_t>(bytesSingle);
            CHECK_AND_RETURN_RET(bytesWritten >= static_cast<size_t>(bytesSingle), MSERR_AUD_RENDER_FAILED);
        }, "AudioRenderer::Write", PlayerXCollie::timerTimeout)
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetAudioTime(uint64_t &time)
{
    MediaTrace trace("AudioSink::GetAudioTime");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    AudioStandard::Timestamp timeStamp;
    bool ret = false;
    LISTENER(ret = audioRenderer_->GetAudioTime(timeStamp, AudioStandard::Timestamp::Timestampbase::MONOTONIC),
        "AudioRenderer::GetAudioTime", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    time = static_cast<uint64_t>(timeStamp.time.tv_nsec);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetLatency(uint64_t &latency) const
{
    MediaTrace trace("AudioSink::GetLatency");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t ret = -1;
    LISTENER(ret = audioRenderer_->GetLatency(latency), "AudioRenderer::GetLatency", PlayerXCollie::timerTimeout)
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    return MSERR_OK;
}

void AudioSinkSvImpl::SetAudioSinkCb(void (*interruptCb)(GstBaseSink *, guint, guint, guint),
                                     void (*stateCb)(GstBaseSink *, guint),
                                     void (*firstFrameCb)(GstBaseSink *, gulong),
                                     void (*deviceCb)(GstBaseSink *, gpointer deviceInfo,
                                        gint32 reason),
                                     void (*errorCb)(GstBaseSink *, const std::string &),
                                     void (*audioDiedCb)(GstBaseSink *))
{
    CHECK_AND_RETURN(audioRendererMediaCallback_ != nullptr);
    errorCb_ = errorCb;
    audioRendererMediaCallback_->SaveInterruptCallback(interruptCb);
    audioRendererMediaCallback_->SaveStateCallback(stateCb);
    audioRendererMediaCallback_->SaveFirstFrameCallback(firstFrameCb);
    audioRendererMediaCallback_->SaveDeviceChangeCallback(deviceCb);
    XcollieTimer xCollie("AudioRenderer::SetRendererCallback", PlayerXCollie::timerTimeout);
    CHECK_AND_RETURN(audioRenderer_ != nullptr);
    audioRenderer_->SetRendererCallback(audioRendererMediaCallback_);
    audioRenderer_->SetRendererFirstFrameWritingCallback(audioRendererMediaCallback_);
    audioRenderer_->RegisterOutputDeviceChangeWithInfoCallback(audioRendererMediaCallback_);
    audioServiceDiedCallback_->SaveAudioPolicyServiceDiedCb(audioDiedCb);
    audioRenderer_->RegisterAudioPolicyServerDiedCb(getpid(), audioServiceDiedCallback_);
}

void AudioSinkSvImpl::SetAudioInterruptMode(int32_t interruptMode)
{
    MediaTrace trace("AudioSink::SetInterruptMode");
    XcollieTimer xCollie("AudioRenderer::SetInterruptMode", PlayerXCollie::timerTimeout);
    CHECK_AND_RETURN(audioRenderer_ != nullptr);
    audioRenderer_->SetInterruptMode(static_cast<AudioStandard::InterruptMode>(interruptMode));
}

int32_t AudioSinkSvImpl::SetAudioEffectMode(int32_t effectMode)
{
    MediaTrace trace("AudioSink::SetAudioEffectMode");
    MEDIA_LOGD("SetAudioEffectMode %{public}d", effectMode);
    XcollieTimer xCollie("AudioRenderer::SetAudioEffectMode", PlayerXCollie::timerTimeout);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t ret = audioRenderer_->SetAudioEffectMode(static_cast<OHOS::AudioStandard::AudioEffectMode>(effectMode));
    CHECK_AND_RETURN_RET_LOG(ret == AudioStandard::SUCCESS, ret, "failed to SetAudioEffectMode!");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetAudioEffectMode(int32_t &effectMode)
{
    MediaTrace trace("AudioSink::GetAudioEffectMode");
    XcollieTimer xCollie("AudioRenderer::GetAudioEffectMode", PlayerXCollie::timerTimeout);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    effectMode = audioRenderer_->GetAudioEffectMode();
    MEDIA_LOGD("GetAudioEffectMode %{public}d", effectMode);
    return MSERR_OK;
}

void AudioSinkSvImpl::SetAudioDumpBySysParam()
{
    std::string dump_enable;
    enableDump_ = false;
    int32_t res = OHOS::system::GetStringParameter("sys.media.dump.audiowrite.enable", dump_enable, "");
    if (res != 0 || dump_enable.empty()) {
        MEDIA_LOGI("0x%{public}06" PRIXPTR " sys.media.dump.audiowrite.enable is not set, dump audio is not required",
            FAKE_POINTER(this));
        return;
    }
    MEDIA_LOGI("sys.media.dump.audiowrite.enable=%s", dump_enable.c_str());
    if (dump_enable == "true") {
        enableDump_ = true;
    }
}

void AudioSinkSvImpl::DumpAudioBuffer(uint8_t *buffer, const size_t &bytesWritten, const size_t &bytesSingle)
{
    if (enableDump_ == false) {
        return;
    }

    if (dumpFile_ == nullptr) {
        std::string dumpFilePath = "/data/media/audio-write-" +
        std::to_string(static_cast<int32_t>(FAKE_POINTER(this))) + ".pcm";
        dumpFile_ = fopen(dumpFilePath.c_str(), "wb+");
    }
    CHECK_AND_RETURN(dumpFile_ != nullptr);
    (void)fwrite(buffer + bytesWritten, bytesSingle, 1, dumpFile_);
    (void)fflush(dumpFile_);
}
} // namespace Media
} // namespace OHOS
