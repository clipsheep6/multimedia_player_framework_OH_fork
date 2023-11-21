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

#ifndef SYSTEM_TONE_PLAYER_H
#define SYSTEM_TONE_PLAYER_H

#include <string>

#include "audio_info.h"

namespace OHOS {
namespace Media {
struct SystemToneOptions {
    bool muteAudio;
    bool muteHaptics;
};

class SystemTonePlayer {
public:
    virtual ~SystemTonePlayer() = default;

    /**
     * @brief Returns the title of the system tone uri set.
     *
     * @return Returns title as string if the title is obtained successfully.
     * returns an empty string otherwise.
     * @since 11
     */
    virtual std::string GetTitle() const = 0;

    /**
     * @brief Prepare the system tone player.
     *
     * @return Returns {@link MSERR_OK} if prepare successfully;
     * returns an error code defined in {@link media_errors.h} otherwise.
     * @since 11
     */
    virtual int32_t Prepare() = 0;

    /**
     * @brief Start playing system tone
     *
     * @return Returns a non-zero streamID if successful, zero if it fails.
     * @since 11
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Start playing system tone with SystemToneOptions
     *
     * @param soundID Indicates the sound ID returned by sound pool.
     * @param systemToneOptions Indicates the mute status of audio and haptic.
     * @return Returns a non-zero streamID if successful, zero if it fails.
     * @since 11
     */
    virtual int32_t Start(const SystemToneOptions &systemToneOptions) = 0;

    /**
     * @brief Stop playing system tone
     *
     * @param streamID Indicates the streamID returned by the Start() or Start(SystemToneOptions systemToneOptions).
     * @return Returns {@link MSERR_OK} if stop playing system tone successfully;
     * returns an error code defined in {@link media_errors.h} otherwise.
     * @since 11
     */
    virtual int32_t Stop(const int32_t &streamID) = 0;

    /**
     * @brief Releases the system tone client resources
     *
     * @return Returns {@link MSERR_OK} if the looping parameter is set successfully to player;
     * returns an error code defined in {@link media_errors.h} otherwise.
     * @since 11
     */
    virtual int32_t Release() = 0;
};
} // namespace Media
} // namespace OHOS
#endif // SYSTEM_TONE_PLAYER_H
