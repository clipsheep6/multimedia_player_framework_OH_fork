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

#ifndef AUDIO_CAPTURE_AS_IMPL_H
#define AUDIO_CAPTURE_AS_IMPL_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "audio_capture.h"
#include "audio_capturer.h"
#include "nocopyable.h"
#include "refbase.h"

namespace OHOS {
namespace Media {
struct AudioCacheCtrl {
    std::mutex captureMutex_;
    std::condition_variable captureCond_;
    std::condition_variable pauseCond_;
    std::queue<std::shared_ptr<AudioBuffer>> captureQueue_;
    uint64_t lastTimeStamp_ = 0;
    uint64_t pausedTime_ = 1; // the timestamp when audio pause called
    uint64_t resumeTime_ = 0; // the timestamp when audio resume called
    uint32_t pausedCount_ = 0; // the paused count times
    uint64_t persistTime_ = 0;
    uint64_t totalPauseTime_ = 0;
};

typedef class AudioCaptureAsImpl AudioCaptureAsImpl;

class AudioCaptureAsImplCallBack : public AudioStandard::AudioCapturerInfoChangeCallback {
public:
    explicit AudioCaptureAsImplCallBack(AudioCaptureAsImpl *callback):callback_(callback) {}
    virtual ~AudioCaptureAsImplCallBack() = default;
    void OnStateChange(const AudioStandard::AudioCapturerChangeInfo &capturerChangeInfo) override;

private:
   AudioCaptureAsImpl* callback_;
};

class AudioCaptureAsImpl : public AudioCapture, public NoCopyable {
public:
    AudioCaptureAsImpl();
    virtual ~AudioCaptureAsImpl();

    int32_t SetCaptureParameter(uint32_t bitrate, uint32_t channels, uint32_t sampleRate, AudioSourceType sourceType,
        const AppInfo &appInfo) override;
    bool IsSupportedCaptureParameter(uint32_t bitrate, uint32_t channels, uint32_t sampleRate) override;
    int32_t GetCaptureParameter(uint32_t &bitrate, uint32_t &channels, uint32_t &sampleRate) override;
    int32_t GetSegmentInfo(uint64_t &start) override;
    int32_t StartAudioCapture() override;
    int32_t StopAudioCapture() override;
    int32_t PauseAudioCapture() override;
    int32_t ResumeAudioCapture() override;
    std::shared_ptr<AudioBuffer> GetBuffer() override;
    int32_t WakeUpAudioThreads() override;

    bool CheckAndGetCaptureParameter(uint32_t bitrate, uint32_t channels, uint32_t sampleRate,
        AudioStandard::AudioCapturerParams &params);
    bool CheckAndGetCaptureOptions(uint32_t bitrate, uint32_t channels, uint32_t sampleRate,
        AudioStandard::AudioCapturerOptions &options);

    int32_t GetActiveMicrophones() override;
    int32_t NotifyAudioCaptureChangeEvent(OHOS::AudioStandard::AudioCapturerChangeInfo changeInfo);
    int32_t GetCreateUid() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.createrUID;
        }
        return -1;
    }

    int32_t GetClientUid() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.clientUID;
        }
        return -1;
    }

    int32_t GetSessionId() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.sessionId;
        }
        return -1;
    }

    int32_t GetInputSource() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.capturerInfo.sourceType);
        }
        return -1;
    }

    int32_t GetCapturerFlag() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.capturerInfo.capturerFlags);
        }
        return -1;
    }

    int32_t GetRecorderState() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.capturerState);
        }
        return -1;
    }

    int32_t GetDeviceType() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.deviceType);
        }
        return -1;
    }

    int32_t GetDeviceRole() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.deviceRole);
        }
        return -1;
    }

    int32_t GetDeviceID() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.deviceId);
        }
        return -1;
    }

    int32_t GetChannelMasks() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.channelMasks);
        }
        return -1;
    }

    int32_t GetChannelIndexMasks() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.channelIndexMasks);
        }
        return -1;
    }

    std::string GetDeviceName() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.inputDeviceInfo.deviceName;
        }
        std::string devicename = "";
        return devicename;
    }

    std::string GetMacAddress() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.inputDeviceInfo.macAddress;
        }
        std::string macAddress = "";
        return macAddress;
    }

    int32_t GetSamPlingRate() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.audioStreamInfo.samplingRate);
        }
        return -1;
    }

    int32_t GetEncoding() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.audioStreamInfo.encoding);
        }
        return -1;
    }

    int32_t GetAudioFormat() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.audioStreamInfo.format);
        }
        return -1;
    }

    int32_t GetAudioChannels() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.audioStreamInfo.channels);
        }
        return -1;
    }

    std::string GetNetWorkId() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.inputDeviceInfo.networkId;
        }
        std::string networkId = "";
        return networkId;
    }

    std::string GetDisplayName() override
    {
        if (getChangeInfoSucess_) {
            return changeInfo_.inputDeviceInfo.displayName;
        }
        std::string displayName = "";
        return displayName;
    }

    int32_t GetInterruptGroupId() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.interruptGroupId);
        }
        return -1;
    }

    int32_t GetVolumeGroupId() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<int32_t>(changeInfo_.inputDeviceInfo.volumeGroupId);
        }
        return -1;
    }

    gboolean GetIsLowatencyDevice() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<gboolean>(changeInfo_.inputDeviceInfo.isLowLatencyDevice);
        }
        return false;
    }

    gboolean Getmuted() override
    {
        if (getChangeInfoSucess_) {
            return static_cast<gboolean>(changeInfo_.muted);
        }
        return false;
    }

    int32_t GetMaxAmpitude() override
    {
        return maxAmpitude_;
    }

    int32_t GetActiveMicrophonesMicId() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->micId_;
            }
        }
        return 0;
    }

    int32_t GetActiveMicrophonesDviceType() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return static_cast<int32_t>(microphoneDescriptor->deviceType_);
            }
        }
        return 0;
    }

    int32_t GetActiveMicrophonesSensitivity() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return static_cast<int32_t>(microphoneDescriptor->sensitivity_);
            }
        }
        return 0;
    }

    float GetActiveMicrophonesPositionX() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->position_.x;
            }
        }
        return 0.0f;
    }

    float GetActiveMicrophonesPositionY() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->position_.y;
            }
        }
        return 0.0f;
    }

    float GetActiveMicrophonesPositionZ() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->position_.z;
            }
        }
        return 0.0f;
    }

    float GetActiveMicrophonesOrientationX() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->orientation_.x;
            }
        }
        return 0.0f;
    }

    float GetActiveMicrophonesOrientationY() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->orientation_.y;
            }
        }
        return 0.0f;
    }

    float GetActiveMicrophonesOrientationZ() override
    {
        for (auto microphoneDescriptor : microphoneDescriptors_) {
            if (microphoneDescriptor != nullptr) {
                return microphoneDescriptor->orientation_.z;
            }
        }
        return 0.0f;
    }

private:
    std::unique_ptr<OHOS::AudioStandard::AudioCapturer> audioCapturer_ = nullptr;
    size_t bufferSize_ = 0; // minimum size of each buffer acquired from AudioServer
    uint64_t bufferDurationNs_ = 0; // each buffer

    // audio cache
    enum AudioRecorderState : int32_t {
        RECORDER_INITIALIZED = 0,
        RECORDER_RUNNING,
        RECORDER_PAUSED,
        RECORDER_RESUME,
        RECORDER_STOP,
    };

    static constexpr int MAX_QUEUE_SIZE = 100;

    int32_t GetCurrentCapturerChangeInfo();
    int32_t AudioCaptureLoop();
    void GetAudioCaptureBuffer();
    void EmptyCaptureQueue();
    std::shared_ptr<AudioBuffer> GetSegmentData();
    std::unique_ptr<AudioCacheCtrl> audioCacheCtrl_;
    std::unique_ptr<std::thread> captureLoop_;
    std::mutex pauseMutex_;
    std::atomic<int32_t> curState_ = RECORDER_INITIALIZED;
    std::atomic<bool> captureLoopErr_ { false };
    uint64_t lastInputTime_ = 0;
    OHOS::AudioStandard::AudioCapturerChangeInfo changeInfo_;
    bool getChangeInfoSucess_ = false;
    std::mutex mutex_;
    int32_t maxAmpitude_ = 0;
    std::shared_ptr<AudioCaptureAsImplCallBack> audioCaptureCb_;
    std::vector<sptr<OHOS::AudioStandard::MicrophoneDescriptor>> microphoneDescriptors_;
};
} // namespace Media
} // namespace OHOS

#endif // AUDIO_CAPTURE_AS_IMPL_H
