/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef AUDIO_HAPTIC_PLAYER_H
#define AUDIO_HAPTIC_PLAYER_H

#include<mutex>

#include "audio_info.h"

namespace OHOS {
namespace Media {
enum AudioHapticType {
    AUDIO_HAPTIC_TYPE_AUDIO = 0,
    AUDIO_HAPTIC_TYPE_HAPTIC = 1,
};

enum class AudioHapticPlayerState {
    /** INVALID state */
    STATE_INVALID = -1,
    /** Create New instance */
    STATE_NEW,
    /** Prepared state */
    STATE_PREPARED,
    /** Running state */
    STATE_RUNNING,
    /** Stopped state */
    STATE_STOPPED,
    /** Released state */
    STATE_RELEASED,
    /** Paused state */
    STATE_PAUSED
};

enum AudioLatencyMode {
    AUDIO_LATENCY_MODE_NORMAL = 0,
    AUDIO_LATENCY_MODE_FAST = 1
};

struct AudioHapticPlayerOptions {
    bool muteAudio;
    bool muteHaptics;
};

struct HapticSource {
    std::string hapticUri = "";
    std::string effectId = "";
};

struct AudioHapticPlayerParam {
    AudioHapticPlayerOptions options;
    std::string audioUri;
    HapticSource hapticSource;
    AudioLatencyMode latencyMode;
    AudioStandard::StreamUsage streamUsage;

    AudioHapticPlayerParam() {};
    AudioHapticPlayerParam(const AudioHapticPlayerOptions &options,
        const std::string &audioUri, const HapticSource &hapticSource,
        const AudioLatencyMode &latencyMode, const AudioStandard::StreamUsage &streamUsage)
        : options(options),
          audioUri(audioUri),
          hapticSource(hapticSource),
          latencyMode(latencyMode),
          streamUsage(streamUsage) {};
};

class AudioHapticPlayerCallback;

class AudioHapticPlayer {
public:
    virtual ~AudioHapticPlayer() = default;

    virtual bool IsMuted(const AudioHapticType &audioHapticType) const = 0;

    virtual int32_t Prepare() = 0;

    virtual int32_t Start() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t SetVolume(float volume) = 0;

    virtual int32_t SetHapticIntensity(float intensity) = 0;

    virtual int32_t SetLoop(bool loop) = 0;

    virtual int32_t SetAudioHapticPlayerCallback(
        const std::shared_ptr<AudioHapticPlayerCallback> &playerCallback) = 0;

    virtual int32_t GetAudioCurrentTime() = 0;
};

class AudioHapticPlayerCallback {
public:
    virtual ~AudioHapticPlayerCallback() = default;

    /**
     * Called when an interrupt is received.
     *
     * @param interruptEvent Indicates the InterruptEvent information needed by client.
     * For details, refer InterruptEventInternal struct in audio_info.h
     */
    virtual void OnInterrupt(const AudioStandard::InterruptEvent &interruptEvent) = 0;

    /**
     * Called when reaching the end of stream.
     */
    virtual void OnEndOfStream(void) = 0;
};

class __attribute__((visibility("default"))) AudioHapticPlayerFactory {
public:
    static std::shared_ptr<AudioHapticPlayer> CreateAudioHapticPlayer(const AudioHapticPlayerParam &param);

private:
    static std::mutex createPlayerMutex_;
    AudioHapticPlayerFactory() = default;
    ~AudioHapticPlayerFactory() = default;
};
} // Media
} // OHOS
#endif // AUDIO_HAPTIC_PLAYER_H
