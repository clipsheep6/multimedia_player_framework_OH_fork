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

#include "i_engine_factory.h"
#include "media_errors.h"
#include "common/log.h"
#include "hirecorder_impl.h"
#include "hiplayer_impl.h"
#include "avmetadatahelper_impl.h"

namespace OHOS {
namespace Media {
class HstEngineFactory : public IEngineFactory {
public:
    HstEngineFactory() = default;
    ~HstEngineFactory() override = default;

    int32_t Score(Scene scene, const std::string& uri) override;
#ifdef SUPPORT_PLAYER
    std::unique_ptr<IPlayerEngine> CreatePlayerEngine(int32_t uid = 0, int32_t pid = 0, uint32_t tokenId = 0) override;
#endif
#ifdef SUPPORT_RECORDER
    std::unique_ptr<IRecorderEngine> CreateRecorderEngine(int32_t appUid, int32_t appPid, uint32_t appTokenId,
        uint64_t appFullTokenId) override;
#endif
#ifdef SUPPORT_METADATA
    std::unique_ptr<IAVMetadataHelperEngine> CreateAVMetadataHelperEngine() override;
#endif
#ifdef SUPPORT_CODEC
    std::unique_ptr<IAVCodecEngine> CreateAVCodecEngine() override;
    std::unique_ptr<IAVCodecListEngine> CreateAVCodecListEngine() override;
#endif
};

int32_t HstEngineFactory::Score(Scene scene, const std::string& uri)
{
    MEDIA_LOG_E("Score in");
    (void)uri;
    return MAX_SCORE;
}

#ifdef SUPPORT_RECORDER
std::unique_ptr<IRecorderEngine> HstEngineFactory::CreateRecorderEngine(
    int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
{
    MEDIA_LOG_E("CreateRecorderEngine enter.");
    auto recorder = std::unique_ptr<HiRecorderImpl>(new (std::nothrow) HiRecorderImpl(
        appUid, appPid, appTokenId, appFullTokenId));
    if (recorder && recorder->Init() == 0) {
        return recorder;
    }
    MEDIA_LOG_E("create recorder failed or recorder init failed");
    return nullptr;
}
#endif

#ifdef SUPPORT_PLAYER
std::unique_ptr<IPlayerEngine> HstEngineFactory::CreatePlayerEngine(int32_t uid, int32_t pid, uint32_t tokenId)
{
    MEDIA_LOG_I("Hst CreatePlayerEngine enter.");
    auto player = std::unique_ptr<HiPlayerImpl>(new (std::nothrow) HiPlayerImpl(
        uid, pid, tokenId, 0));
    if (player) {
        return player;
    }
    MEDIA_LOG_E("create player failed");
    return nullptr;
}
#endif

#ifdef SUPPORT_METADATA
std::unique_ptr<IAVMetadataHelperEngine> HstEngineFactory::CreateAVMetadataHelperEngine()
{
    MEDIA_LOG_I("CreateAVMetadataHelperEngine enter.");
    auto helper = std::unique_ptr<AVMetadataHelperImpl>(new (std::nothrow) AVMetadataHelperImpl());
    if (helper) {
        return helper;
    }
    MEDIA_LOG_E("create AVMetadataHelperImpl failed");
    return nullptr;
}
#endif

#ifdef SUPPORT_CODEC
std::unique_ptr<IAVCodecEngine> HstEngineFactory::CreateAVCodecEngine()
{
    return nullptr;
}

std::unique_ptr<IAVCodecListEngine> HstEngineFactory::CreateAVCodecListEngine()
{
    return nullptr;
}
#endif
} // namespace Media
} // namespace OHOS

#ifdef __cplusplus
extern "C" {
#endif
__attribute__((visibility("default"))) OHOS::Media::IEngineFactory *CreateEngineFactory()
{
    return new (std::nothrow) OHOS::Media::HstEngineFactory();
}
#ifdef __cplusplus
}
#endif