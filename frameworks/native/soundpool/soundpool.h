/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef SOUNDPOOL_H
#define SOUNDPOOL_H

#include "isoundpool.h"
#include "audio_renderer.h"
#include "stream_id_manager.h"
#include "sound_id_manager.h"

namespace OHOS {
namespace Media {
class SoundPool : public ISoundPool {
public:
    SoundPool();
    ~SoundPool();
    static bool CheckInitParam(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo);

    int32_t Init(int maxStreams, AudioStandard::AudioRendererInfo audioRenderInfo);

    int32_t Load(const std::string url) override;

    int32_t Load(int32_t fd, int64_t offset, int64_t length) override;

    int32_t Play(int32_t soundID, PlayParams playParameters) override;

    int32_t Stop(int32_t streamID) override;

    int32_t SetLoop(int32_t streamID, int32_t loop) override;

    int32_t SetPriority(int32_t streamID, int32_t priority) override;

    int32_t SetRate(int32_t streamID, AudioStandard::AudioRendererRate renderRate) override;

    int32_t SetVolume(int32_t streamID, float leftVolume, float rightVolume) override;

    int32_t Unload(int32_t soundID) override;

    int32_t Release() override;

    int32_t SetSoundPoolCallback(const std::shared_ptr<ISoundPoolCallback> &soundPoolCallback) override;

    int32_t SetSoundPoolFrameWriteCallback(
        const std::shared_ptr<ISoundPoolFrameWriteCallback> &frameWriteCallback) override;

private:
    bool CheckVolumeVaild(float *leftVol, float *rightVol);
    int32_t ReleaseInner();
    std::shared_ptr<SoundIDManager> soundIDManager_;
    std::shared_ptr<StreamIDManager> streamIdManager_;
    std::mutex soundPoolLock_;
    std::shared_ptr<ISoundPoolCallback> callback_ = nullptr;
    std::shared_ptr<ISoundPoolFrameWriteCallback> frameWriteCallback_ = nullptr;
    static constexpr int32_t MIN_STREAM_PRIORITY = 0;
};
} // namespace Media
} // namespace OHOS
#endif // SOUNDPOOL_H
