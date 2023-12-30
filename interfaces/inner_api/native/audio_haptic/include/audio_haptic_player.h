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

#include "audio_info.h"

namespace OHOS {
namespace Media {
enum AudioHapticType {
    AUDIO_HAPTIC_TYPE_AUDIO = 0,
    AUDIO_HAPTIC_TYPE_HAPTIC = 1,
    AUDIO_LATENCY_MODE_UNKNOWN = 2,
};

enum AudioHapticPlayerState {
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

class AudioHapticPlayerCallback;

class AudioHapticPlayer {
public:
    virtual ~AudioHapticPlayer() = default;

    virtual bool IsMuted(const AudioHapticType &audioHapticType) const = 0;

    virtual int32_t Start() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t Release() = 0;

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
} // Media
} // OHOS
#endif // AUDIO_HAPTIC_PLAYER_H
