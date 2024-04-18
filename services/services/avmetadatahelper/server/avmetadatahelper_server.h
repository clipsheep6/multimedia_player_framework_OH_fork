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

#ifndef AVMETADATAHELPER_SERVICE_SERVER_H
#define AVMETADATAHELPER_SERVICE_SERVER_H

#include <mutex>
#include "i_avmetadatahelper_service.h"
#include "i_avmetadatahelper_engine.h"
#include "nocopyable.h"
#include "uri_helper.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperServer : public IAVMetadataHelperService, public NoCopyable {
public:
    static std::shared_ptr<IAVMetadataHelperService> Create();
    AVMetadataHelperServer();
    virtual ~AVMetadataHelperServer();

    int32_t SetSource(const std::string &uri, int32_t usage) override;
    int32_t SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage) override;
    int32_t SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc) override;
    std::string ResolveMetadata(int32_t key) override;
    std::unordered_map<int32_t, std::string> ResolveMetadata() override;
    std::shared_ptr<Meta> GetAVMetadata() override;
    std::shared_ptr<AVSharedMemory> FetchArtPicture() override;
    std::shared_ptr<AVSharedMemory> FetchFrameAtTime(int64_t timeUs,
        int32_t option, const OutputConfiguration &param) override;
    void Release() override;
    int32_t SetHelperCallback(const std::shared_ptr<HelperCallback> &callback) override;

private:
    const std::string &GetStatusDescription(int32_t status);
    void ChangeState(const HelperStates state);
    void NotifyErrorCallback(int32_t code, const std::string msg);
    void NotifyInfoCallback(HelperOnInfoType type, int32_t extra);

    int32_t appUid_;
    std::shared_ptr<IAVMetadataHelperEngine> avMetadataHelperEngine_ = nullptr;
    std::mutex mutex_;
    std::unique_ptr<UriHelper> uriHelper_;
    TaskQueue taskQue_;
    std::shared_ptr<IMediaDataSource> dataSrc_ = nullptr;
    bool isLiveStream_ = false;
    struct ConfigInfo {
        std::atomic<bool> looping = false;
        float leftVolume = INVALID_VALUE;
        float rightVolume = INVALID_VALUE;
        std::string url;
    } config_;
    static constexpr float INVALID_VALUE = 2.0f;

    std::shared_ptr<HelperCallback> helperCb_ = nullptr;
    HelperStates currState_ = HelperStates::HELPER_IDLE;
    std::mutex mutexCb_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_SERVICE_SERVER_H
