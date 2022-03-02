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

#include <sys/time.h>
#include "avmetadatahelper_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperServer"};
}

namespace OHOS {
namespace Media {
const int32_t scale=1000;
std::shared_ptr<IAVMetadataHelperService> AVMetadataHelperServer::Create()
{
    std::shared_ptr<AVMetadataHelperServer> server = std::make_shared<AVMetadataHelperServer>();
    CHECK_AND_RETURN_RET_LOG(server != nullptr, nullptr, "Failed to new AVMetadataHelperServer");
    return server;
}

AVMetadataHelperServer::AVMetadataHelperServer()
{
    pid = IPCSkeleton::GetCallingPid();
    uid = IPCSkeleton::GetCallingUid();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperServer::~AVMetadataHelperServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(mutex_);
    avMetadataHelperEngine_ = nullptr;
}

int32_t AVMetadataHelperServer::SetSource(const std::string &uri, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("Current uri is : %{public}s %{public}u", uri.c_str(), usage);
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_AVMETADATA, uri);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED,
        "Failed to get engine factory");
    avMetadataHelperEngine_ = engineFactory->CreateAVMetadataHelperEngine();
    avMetaDataHelperSourceUrl = uri;
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED,
        "Failed to create avmetadatahelper engine");
    int32_t ret = avMetadataHelperEngine_->SetSource(uri, usage);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetSource failed!");
    return MSERR_OK;
}

std::string AVMetadataHelperServer::ResolveMetadata(int32_t key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("Key is %{public}d", key);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, "", "avMetadataHelperEngine_ is nullptr");
    struct timeval play_begin;
    struct timeval play_end;
    long time; // ms
    gettimeofday(&play_begin, nullptr);
    std::string metaData = avMetadataHelperEngine_->ResolveMetadata(key);
    gettimeofday(&play_end, nullptr);
    time = (play_end.tv_sec - play_begin.tv_sec) * scale + (play_end.tv_usec - play_begin.tv_usec) / scale;
    resolveMetaDataTimeList.push_back(time);
    return metaData;
}

std::unordered_map<int32_t, std::string> AVMetadataHelperServer::ResolveMetadata()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, {}, "avMetadataHelperEngine_ is nullptr");
    struct timeval play_begin;
    struct timeval play_end;
    long time; // ms
    gettimeofday(&play_begin, nullptr);
    std::unordered_map<int32_t, std::string> metaData = avMetadataHelperEngine_->ResolveMetadata();
    gettimeofday(&play_end, nullptr);
    time = (play_end.tv_sec - play_begin.tv_sec) * scale + (play_end.tv_usec - play_begin.tv_usec) / scale;
    resolveMetaDataTimeList.push_back(time);
    return metaData;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServer::FetchArtPicture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, {}, "avMetadataHelperEngine_ is nullptr");
    struct timeval play_begin;
    struct timeval play_end;
    long time; // ms
    gettimeofday(&play_begin, nullptr);
    std::shared_ptr<AVSharedMemory> artPicture = avMetadataHelperEngine_->FetchArtPicture();
    gettimeofday(&play_end, nullptr);
    time = (play_end.tv_sec - play_begin.tv_sec) * scale + (play_end.tv_usec - play_begin.tv_usec) / scale;
    fetchThumbnailTimeList.push_back(time);
    return artPicture;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServer::FetchFrameAtTime(int64_t timeUs, int32_t option,
    const OutputConfiguration &param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, nullptr, "avMetadataHelperEngine_ is nullptr");
    struct timeval play_begin;
    struct timeval play_end;
    long time; // ms
    gettimeofday(&play_begin, nullptr);
    std::shared_ptr<AVSharedMemory> frame = avMetadataHelperEngine_->FetchFrameAtTime(timeUs, option, param);
    gettimeofday(&play_end, nullptr);
    time = (play_end.tv_sec - play_begin.tv_sec) * scale + (play_end.tv_usec - play_begin.tv_usec) / scale;
    fetchThumbnailTimeList.push_back(time);
    return frame;
}

void AVMetadataHelperServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    avMetadataHelperEngine_ = nullptr;
}
}
}