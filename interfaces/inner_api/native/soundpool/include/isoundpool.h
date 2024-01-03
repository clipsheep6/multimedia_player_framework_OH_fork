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

#ifndef ISOUNDPOOL_H
#define ISOUNDPOOL_H

#include <string>
#include "audio_info.h"

namespace OHOS {
namespace Media {
struct PlayParams {
    int32_t loop = 0;
    int32_t rate = 0; // default AudioRendererRate::RENDER_RATE_NORMAL
    float leftVolume = (float)1.0;
    float rightVolume = (float)1.0;
    int32_t priority = 0;
    bool parallelPlayFlag = false;
    std::string cacheDir;
};

class ISoundPoolCallback;
class ISoundPoolFrameWriteCallback;

class ISoundPool {
public:
    virtual ~ISoundPool() = default;

    /**
     * @brief Load the sound from the specified path.
     *
     * @param url The path to the audio file
     * @return Returns a sound ID. This value can be used to play or unload the sound.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Load(std::string url) = 0;

    /**
     * @brief Load the sound from a FileDescriptor..
     *
     * @param fd A FileDescriptor object
     * @param offset Offset to the start of the sound
     * @param length Length of the sound
     * @return Returns a sound ID. This value can be used to play or unload the sound.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Load(int32_t fd, int64_t offset, int64_t length) = 0;

    /**
     * @brief Play a sound from a sound ID.
     *
     * @param soundID Returned by the load()
     * @param playParameters params Player parameters
     * @return Returns a non-zero streamID if successful, zero if it fails.
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Play(int32_t soundID, PlayParams playParameters) = 0;

    /**
     * @brief Stop a stream which is playing.
     *
     * @param streamID Returned by the play()
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Stop(int32_t streamID) = 0;

    /**
     * @brief Set loop mode.
     *
     * @param streamID Returned by the play()
     * @param loop Loop mode (0 = no loop, -1 = loop forever)
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetLoop(int32_t streamID, int32_t loop) = 0;

    /**
     * @brief Set stream priority.
     *
     * @param streamID Returned by the play()
     * @param priority Stream priority (0 = lowest priority)
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetPriority(int32_t streamID, int32_t priority) = 0;

    /**
     * @brief Set playback rate.
     *
     * @param streamID Returned by the play()
     * @param renderRate Playback rate
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetRate(int32_t streamID, AudioStandard::AudioRendererRate renderRate) = 0;

    /**
     * @brief Set stream volume.
     *
     * @param streamID Returned by the play()
     * @param leftVolume leftVolume Volume value(range = 0.0 to 1.0),current leftVolume = rightVolume
     * @param rigthVolume rightVolume Volume value(range = 0.0 to 1.0),current leftVolume = rightVolume
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetVolume(int32_t streamID, float leftVolume, float rigthVolume) = 0;

    /**
     * @brief Unload a sound from a sound ID.
     *
     * @param soundID Returned by the load()
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Unload(int32_t soundID) = 0;

    /**
     * @brief Releases the soundPool. This method uses an asynchronous callback to return the result.
     *
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t Release() = 0;

    /**
     * @brief Register listens for soundpool
     *
     * @param soundPoolCallback The listen class for soundpool
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetSoundPoolCallback
        (const std::shared_ptr<ISoundPoolCallback> &soundPoolCallback) = 0;

    /**
     * @brief Register frame write listens for soundpool
     *
     * @param frameWriteCallback The frame writ listen class for soundpool
     * @return Returns used to return the result. MSERR_OK if success
     * @since 1.0
     * @version 1.0
     */
    virtual int32_t SetSoundPoolFrameWriteCallback
        (const std::shared_ptr<ISoundPoolFrameWriteCallback> &frameWriteCallback) = 0;
};

class ISoundPoolCallback {
public:
    virtual ~ISoundPoolCallback() = default;

    /**
     * @brief Register listens for load result event.
     *
     * @param result used to listen for loaded soundId event
     * @since 1.0
     * @version 1.0
     */
    virtual void OnLoadCompleted(int32_t soundId) = 0;

    /**
     * @brief Register the play finish event to listen for.
     *
     * @since 1.0
     * @version 1.0
     */
    virtual void OnPlayFinished() = 0;

    /**
     * @brief Register listens for sound play error events.
     *
     * @param errorCode Type of the sound play error event to listen for.
     * @since 1.0
     * @version 1.0
     */
    virtual void OnError(int32_t errorCode) = 0;
};

class ISoundPoolFrameWriteCallback {
public:
    virtual ~ISoundPoolFrameWriteCallback() = default;

    virtual void OnFirstAudioFrameWritingCallback(uint64_t &latency) = 0;
};

class __attribute__((visibility("default"))) SoundPoolFactory {
public:
#ifdef UNSUPPORT_SOUND_POOL
    static std::shared_ptr<ISoundPool> CreateSoundPool(int maxStreams,
        AudioStandard::AudioRendererInfo audioRenderInfo)
    {
        return nullptr;
    }
#else
    static std::shared_ptr<ISoundPool> CreateSoundPool(int maxStreams,
        AudioStandard::AudioRendererInfo audioRenderInfo);
#endif
private:
    SoundPoolFactory() = default;
    ~SoundPoolFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // ISOUNDPOOL_H
