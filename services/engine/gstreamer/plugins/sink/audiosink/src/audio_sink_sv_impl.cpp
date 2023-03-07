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
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"
#include "param_wrapper.h"
#include "player_xcollie.h"
#include "scope_guard.h"

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

AudioSinkSvImpl::AudioSinkSvImpl(GstBaseSink *sink)
    : audioSink_(sink)
{
    audioRendererMediaCallback_ = std::make_shared<AudioRendererMediaCallback>(sink);
}

AudioSinkSvImpl::~AudioSinkSvImpl()
{
    if (audioRenderer_ != nullptr) {
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Release");
        (void)audioRenderer_->Release();
        PlayerXCollie::GetInstance().CancelTimer(id);
        id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::~AudioRenderer");
        audioRenderer_ = nullptr;
        PlayerXCollie::GetInstance().CancelTimer(id);
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
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::SetVolume");
    int32_t ret = audioRenderer_->SetVolume(volume);
    CHECK_AND_RETURN_RET_LOG(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED, "audio server setvolume failed!");
    PlayerXCollie::GetInstance().CancelTimer(id);
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

int32_t AudioSinkSvImpl::Prepare(int32_t appUid, int32_t appPid)
{
    MediaTrace trace("AudioSink::Prepare");
    MEDIA_LOGD("audioRenderer Prepare In");
    AudioStandard::AppInfo appInfo = {};
    appInfo.appUid = appUid;
    appInfo.appPid = appPid;
    rendererOptions_.streamInfo.samplingRate = AudioStandard::SAMPLE_RATE_8000;
    rendererOptions_.streamInfo.encoding = AudioStandard::ENCODING_PCM;
    rendererOptions_.streamInfo.format = AudioStandard::SAMPLE_S16LE;
    rendererOptions_.streamInfo.channels = AudioStandard::MONO;
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Create");
    audioRenderer_ = AudioStandard::AudioRenderer::Create(rendererOptions_, appInfo);
    PlayerXCollie::GetInstance().CancelTimer(id);
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    SetMuteVolumeBySysParam();
    MEDIA_LOGD("audioRenderer Prepare Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Start()
{
    MediaTrace trace("AudioSink::Start");
    MEDIA_LOGD("audioRenderer Start In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Start");
    (void)audioRenderer_->Start();
    PlayerXCollie::GetInstance().CancelTimer(id);
    MEDIA_LOGD("audioRenderer Start Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Stop()
{
    MediaTrace trace("AudioSink::Stop");
    MEDIA_LOGD("audioRenderer Stop In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Stop");
    (void)audioRenderer_->Stop();
    PlayerXCollie::GetInstance().CancelTimer(id);
    MEDIA_LOGD("audioRenderer Stop Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Pause()
{
    MediaTrace trace("AudioSink::Pause");
    MEDIA_LOGD("audioRenderer Pause In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    if (audioRenderer_->GetStatus() == OHOS::AudioStandard::RENDERER_RUNNING) {
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Pause");
        bool ret = audioRenderer_->Pause();
        if (!ret) {
            MEDIA_LOGE("audio Renderer Pause failed!");
        }
        PlayerXCollie::GetInstance().CancelTimer(id);
        CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    }
    MEDIA_LOGD("audioRenderer Pause Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Drain()
{
    MediaTrace trace("AudioSink::Drain");
    MEDIA_LOGD("Drain");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Drain");
    auto ret = audioRenderer_->Drain();
    PlayerXCollie::GetInstance().CancelTimer(id);
    CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Flush()
{
    MediaTrace trace("AudioSink::Flush");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    if (audioRenderer_->GetStatus() == OHOS::AudioStandard::RENDERER_RUNNING) {
        MEDIA_LOGD("Flush");
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Flush");
        auto ret = audioRenderer_->Flush();
        PlayerXCollie::GetInstance().CancelTimer(id);
        CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    }
    
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::Release()
{
    MediaTrace trace("AudioSink::Release");
    MEDIA_LOGD("audioRenderer Release In");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Release");
    (void)audioRenderer_->Release();
    PlayerXCollie::GetInstance().CancelTimer(id);
    id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::~AudioRenderer");
    audioRenderer_ = nullptr;
    PlayerXCollie::GetInstance().CancelTimer(id);
    MEDIA_LOGD("audioRenderer Release Out");
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
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::SetParams");
    auto ret = audioRenderer_->SetParams(params);
    PlayerXCollie::GetInstance().CancelTimer(id);
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    MEDIA_LOGD("audioRenderer SetParams Out");
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate)
{
    (void)bitsPerSample;
    MEDIA_LOGD("GetParameters");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    AudioStandard::AudioRendererParams params;
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::GetParams");
    auto ret = audioRenderer_->GetParams(params);
    PlayerXCollie::GetInstance().CancelTimer(id);
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
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::GetBufferSize");
    auto ret = audioRenderer_->GetBufferSize(size);
    PlayerXCollie::GetInstance().CancelTimer(id);
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
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::GetFrameCount");
    auto ret = audioRenderer_->GetFrameCount(count);
    PlayerXCollie::GetInstance().CancelTimer(id);
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    CHECK_AND_RETURN_RET(count > 0, MSERR_AUD_RENDER_FAILED);
    frameCount = count;
    return MSERR_OK;
}

bool AudioSinkSvImpl::Writeable() const
{
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, false);
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::GetStatus");
    auto ret = audioRenderer_->GetStatus();
    PlayerXCollie::GetInstance().CancelTimer(id);
    return ret == AudioStandard::RENDERER_RUNNING;
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
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::Write");
    ON_SCOPE_EXIT(0) {
        PlayerXCollie::GetInstance().CancelTimer(id);
    };
    while (bytesWritten < size) {
        int32_t bytesSingle = audioRenderer_->Write(buffer + bytesWritten, size - bytesWritten);
        if (bytesSingle <= 0) {
            MEDIA_LOGE("[AudioSinkSvImpl] audioRenderer write failed, drop an audio packet!");
            return MSERR_OK;
        }
        bytesWritten += static_cast<size_t>(bytesSingle);
        CHECK_AND_RETURN_RET(bytesWritten >= static_cast<size_t>(bytesSingle), MSERR_AUD_RENDER_FAILED);
    }
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetAudioTime(uint64_t &time)
{
    MediaTrace trace("AudioSink::GetAudioTime");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    AudioStandard::Timestamp timeStamp;
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::GetAudioTime");
    bool ret = audioRenderer_->GetAudioTime(timeStamp, AudioStandard::Timestamp::Timestampbase::MONOTONIC);
    PlayerXCollie::GetInstance().CancelTimer(id);
    CHECK_AND_RETURN_RET(ret == true, MSERR_AUD_RENDER_FAILED);
    time = static_cast<uint64_t>(timeStamp.time.tv_nsec);
    return MSERR_OK;
}

int32_t AudioSinkSvImpl::GetLatency(uint64_t &latency) const
{
    MediaTrace trace("AudioSink::GetLatency");
    CHECK_AND_RETURN_RET(audioRenderer_ != nullptr, MSERR_AUD_RENDER_FAILED);
    int32_t id = PlayerXCollie::GetInstance().SetTimerByLog("AudioRenderer::GetLatency");
    auto ret = audioRenderer_->GetLatency(latency);
    PlayerXCollie::GetInstance().CancelTimer(id);
    CHECK_AND_RETURN_RET(ret == AudioStandard::SUCCESS, MSERR_AUD_RENDER_FAILED);
    return MSERR_OK;
}

void AudioSinkSvImpl::SetAudioSinkCb(void (*interruptCb)(GstBaseSink *, guint, guint, guint),
                                     void (*stateCb)(GstBaseSink *, guint),
                                     void (*errorCb)(GstBaseSink *, const std::string &))
{
    CHECK_AND_RETURN(audioRendererMediaCallback_ != nullptr);
    errorCb_ = errorCb;
    audioRendererMediaCallback_->SaveInterruptCallback(interruptCb);
    audioRendererMediaCallback_->SaveStateCallback(stateCb);
    audioRenderer_->SetRendererCallback(audioRendererMediaCallback_);
}

void AudioSinkSvImpl::SetAudioInterruptMode(int32_t interruptMode)
{
    CHECK_AND_RETURN(audioRendererMediaCallback_ != nullptr);
    audioRenderer_->SetInterruptMode(static_cast<AudioStandard::InterruptMode>(interruptMode));
}
} // namespace Media
} // namespace OHOS
