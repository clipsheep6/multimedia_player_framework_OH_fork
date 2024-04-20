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

#ifndef AUDIO_HAPTIC_MANAGER_IMPL_H
#define AUDIO_HAPTIC_MANAGER_IMPL_H

#include "audio_haptic_manager.h"
#include "audio_haptic_player.h"

namespace OHOS {
namespace Media {
struct AudioHapticPlayerInfo {
    std::string audioUri_;
    HapticSource hapticSource_;
    AudioLatencyMode latencyMode_;
    AudioStandard::StreamUsage streamUsage_;
    std::shared_ptr<AudioHapticPlayer> audioHapticPlayer_;

    AudioHapticPlayerInfo() {};
    AudioHapticPlayerInfo(const std::string &audioUri, const HapticSource &hapticSource,
        const AudioLatencyMode &latencyMode, const AudioStandard::StreamUsage &streamUsage,
        const std::shared_ptr<AudioHapticPlayer> &audioHapticPlayer)
        : audioUri_(audioUri),
          hapticSource_(hapticSource),
          latencyMode_(latencyMode),
          streamUsage_(streamUsage),
          audioHapticPlayer_(audioHapticPlayer) {};
};

class AudioHapticManagerImpl : public AudioHapticManager {
public:
    AudioHapticManagerImpl();
    ~AudioHapticManagerImpl();

    int32_t RegisterSource(const std::string &audioUri, const std::string &hapticUri) override;

    int32_t RegisterSourceWithEffectId(const std::string &audioUri, const std::string &effectId) override;

    int32_t UnregisterSource(const int32_t &sourceID) override;

    int32_t SetAudioLatencyMode(const int32_t &sourceId, const AudioLatencyMode &latencyMode) override;

    int32_t SetStreamUsage(const int32_t &sourceID, const AudioStandard::StreamUsage &streamUsage) override;

    std::shared_ptr<AudioHapticPlayer> CreatePlayer(const int32_t &sourceID,
        const AudioHapticPlayerOptions &audioHapticPlayerOptions) override;

private:
    bool CheckAudioLatencyMode(const int32_t &sourceId, const AudioLatencyMode &latencyMode);
    bool CheckAudioStreamUsage(const AudioStandard::StreamUsage &streamUsage);

    std::unordered_map<int32_t, std::shared_ptr<AudioHapticPlayerInfo>> audioHapticPlayerMap_;
    int32_t curPlayerIndex_;
    int32_t curPlayerCount_;

    std::mutex audioHapticManagerMutex_;
};
} // namespace Media
} // namespace OHOS
#endif // AUDIO_HAPTIC_MANAGER_IMPL_H
