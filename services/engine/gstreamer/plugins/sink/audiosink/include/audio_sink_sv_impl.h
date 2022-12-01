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

#ifndef AUDIO_SINK_SV_IMPL_H
#define AUDIO_SINK_SV_IMPL_H

#include "audio_sink.h"
#include "audio_renderer.h"
#include "audio_system_manager.h"
#include "audio_errors.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class AudioRendererMediaCallback : public AudioStandard::AudioRendererCallback {
public:
    using InterruptCbFunc = std::function<void(GstBaseSink *, guint, guint, guint)>;
    using StateCbFunc = std::function<void(GstBaseSink *, guint)>;
    explicit AudioRendererMediaCallback(GstBaseSink *audioSink);
    ~AudioRendererMediaCallback();
    void SaveInterruptCallback(InterruptCbFunc interruptCb);
    void SaveStateCallback(StateCbFunc stateCb);
    void OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent) override;
    void OnStateChange(const AudioStandard::RendererState state,
        const AudioStandard::StateChangeCmdType cmdType) override;
private:
    GstBaseSink *audioSink_ = nullptr;
    InterruptCbFunc interruptCb_ = nullptr;
    StateCbFunc stateCb_ = nullptr;
    TaskQueue taskQue_;
};


class AudioSinkSvImpl : public AudioSink {
public:
    explicit AudioSinkSvImpl(GstBaseSink *audioSink);
    virtual ~AudioSinkSvImpl();

    GstCaps *GetCaps() override;
    int32_t SetVolume(float volume) override;
    int32_t GetVolume(float &volume) override;
    int32_t GetMaxVolume(float &volume) override;
    int32_t GetMinVolume(float &volume) override;
    int32_t Prepare(int32_t appUid, int32_t appPid) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Pause() override;
    int32_t Drain() override;
    int32_t Flush() override;
    int32_t Release() override;
    int32_t SetParameters(uint32_t bitsPerSample, uint32_t channels, uint32_t sampleRate) override;
    int32_t GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate) override;
    int32_t GetMinimumBufferSize(uint32_t &bufferSize) override;
    int32_t GetMinimumFrameCount(uint32_t &frameCount) override;
    int32_t Write(uint8_t *buffer, size_t size) override;
    int32_t GetAudioTime(uint64_t &time) override;
    int32_t GetLatency(uint64_t &latency) const override;
    int32_t SetRendererInfo(int32_t desc, int32_t rendererFlags) override;
    void SetAudioInterruptMode(int32_t interruptMode) override;
    void SetAudioSinkCb(void (*interruptCb)(GstBaseSink *, guint, guint, guint),
                        void (*stateCb)(GstBaseSink *, guint),
                        void (*errorCb)(GstBaseSink *, const std::string &)) override;
    bool Writeable() const override;

private:
    void OnError(std::string errMsg);
    using ErrorCbFunc = std::function<void(GstBaseSink *, const std::string &)>;
    ErrorCbFunc errorCb_ = nullptr;
    GstBaseSink *audioSink_ = nullptr;
    std::unique_ptr<OHOS::AudioStandard::AudioRenderer> audioRenderer_ = nullptr;
    AudioStandard::AudioRendererOptions rendererOptions_ = {};
    void InitChannelRange(GstCaps *caps) const;
    void InitRateRange(GstCaps *caps) const;
    void SetMuteVolumeBySysParam();
    bool isMute_ = false;
    std::shared_ptr<AudioRendererMediaCallback> audioRendererMediaCallback_ = nullptr;
};

class AudioSinkBypass : public AudioSinkSvImpl {
public:
    explicit AudioSinkBypass(GstBaseSink *audioSink) : AudioSinkSvImpl(audioSink) {}
    virtual ~AudioSinkBypass() = default;

    int32_t SetVolume(float volume) override
    {
        (void)volume;
        return MSERR_OK;
    }
    int32_t GetVolume(float &volume) override
    {
        volume = 1.0; // default volume 1.0
        return MSERR_OK;
    }
    int32_t GetMaxVolume(float &volume) override
    {
        volume = 1.0; // default max volume 1.0
        return MSERR_OK;
    }
    int32_t GetMinVolume(float &volume) override
    {
        volume = 0.0; // default min volume 0.0
        return MSERR_OK;
    }
    int32_t Prepare(int32_t appUid, int32_t appPid) override
    {
        (void)appUid;
        (void)appPid;
        return MSERR_OK;
    }
    int32_t Start() override
    {
        return MSERR_OK;
    }
    int32_t Stop() override
    {
        return MSERR_OK;
    }
    int32_t Pause() override
    {
        return MSERR_OK;
    }
    int32_t Drain() override
    {
        return MSERR_OK;
    }
    int32_t Flush() override
    {
        return MSERR_OK;
    }
    int32_t Release() override
    {
        return MSERR_OK;
    }
    int32_t SetParameters(uint32_t bitsPerSample, uint32_t channels, uint32_t sampleRate) override
    {
        (void)bitsPerSample;
        (void)channels;
        (void)sampleRate;
        return MSERR_OK;
    }
    int32_t GetParameters(uint32_t &bitsPerSample, uint32_t &channels, uint32_t &sampleRate) override
    {
        bitsPerSample = 16; // default bitsPerSample is 16
        channels = 2; // default channels is 2
        sampleRate = 44100; // default sampleRate is 44100
        return MSERR_OK;
    }
    int32_t GetMinimumBufferSize(uint32_t &bufferSize) override
    {
        bufferSize = 3528; // default size is 3528
        return MSERR_OK;
    }
    int32_t GetMinimumFrameCount(uint32_t &frameCount) override
    {
        frameCount = 882; // default count is 882
        return MSERR_OK;
    }
    int32_t Write(uint8_t *buffer, size_t size) override
    {
        (void)buffer;
        (void)size;
        return MSERR_OK;
    }
    int32_t GetAudioTime(uint64_t &time) override
    {
        (void)time;
        return MSERR_OK;
    }
    int32_t GetLatency(uint64_t &latency) const override
    {
        latency = 0;
        return MSERR_OK;
    }
    int32_t SetRendererInfo(int32_t desc, int32_t rendererFlags) override
    {
        (void)desc;
        (void)rendererFlags;
        return MSERR_OK;
    }
    void SetAudioInterruptMode(int32_t interruptMode) override
    {
        (void)interruptMode;
    }
    void SetAudioSinkCb(void (*interruptCb)(GstBaseSink *, guint, guint, guint),
                        void (*stateCb)(GstBaseSink *, guint),
                        void (*errorCb)(GstBaseSink *, const std::string &)) override
    {
        (void)interruptCb;
        (void)stateCb;
        (void)errorCb;
    }
    bool Writeable() const override
    {
        return true;
    }
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_SINK_SV_IMPL_H
