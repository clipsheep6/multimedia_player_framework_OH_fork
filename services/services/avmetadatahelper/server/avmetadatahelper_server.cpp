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

#include "avmetadatahelper_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"
#include "uri_helper.h"
#include "media_dfx.h"
#include "ipc_skeleton.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperServer"};
}

namespace OHOS {
namespace Media {
static const std::unordered_map<int32_t, std::string> STATUS_TO_STATUS_DESCRIPTION_TABLE = {
    {HELPER_STATE_ERROR, "HELPER_STATE_ERROR"},
    {HELPER_IDLE, "HELPER_IDLE"},
    {HELPER_PREPARED, "HELPER_PREPARED"},
    {HELPER_CALL_DONE, "HELPER_CALL_DONE"},
    {HELPER_RELEASED, "HELPER_RELEASED"},
};

std::shared_ptr<IAVMetadataHelperService> AVMetadataHelperServer::Create()
{
    std::shared_ptr<AVMetadataHelperServer> server = std::make_shared<AVMetadataHelperServer>();
    CHECK_AND_RETURN_RET_LOG(server != nullptr, nullptr, "Failed to new AVMetadataHelperServer");
    return server;
}

AVMetadataHelperServer::AVMetadataHelperServer()
    : taskQue_("AVMetadata")
{
    appUid_ = IPCSkeleton::GetCallingUid();
    (void)taskQue_.Start();
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperServer::~AVMetadataHelperServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(mutex_);
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        avMetadataHelperEngine_ = nullptr;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
    uriHelper_ = nullptr;
    taskQue_.Stop();
}

int32_t AVMetadataHelperServer::SetSource(const std::string &uri, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::SetSource_uri");
    MEDIA_LOGD("Current uri is : %{public}s %{public}u", uri.c_str(), usage);
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), MSERR_INVALID_VAL, "uri is empty");

    uriHelper_ = std::make_unique<UriHelper>(uri);
    CHECK_AND_RETURN_RET_LOG(!uriHelper_->FormattedUri().empty(),
                             MSERR_INVALID_VAL,
                             "Failed to construct formatted uri");
    if (!uriHelper_->AccessCheck(UriHelper::URI_READ)) {
        MEDIA_LOGE("Failed to read the file");
        return MSERR_INVALID_VAL;
    }
    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto metaEngineFactory = EngineFactoryRepo::Instance().GetEngineFactory(
            IEngineFactory::Scene::SCENE_AVMETADATA, appUid_, uriHelper_->FormattedUri());
        CHECK_AND_RETURN_RET_LOG(metaEngineFactory != nullptr, (int32_t)MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED,
            "Failed to get engine factory.");
        avMetadataHelperEngine_ = metaEngineFactory->CreateAVMetadataHelperEngine();
        CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr,
            (int32_t)MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED, "Failed to create avmetadatahelper engine.");
        int32_t ret = avMetadataHelperEngine_->SetSource(uriHelper_->FormattedUri(), usage);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "0x%{public}06" PRIXPTR " SetSource failed", FAKE_POINTER(this));
        return ret;
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed.");

    auto result = task->GetResult();
    ChangeState(HelperStates::HELPER_PREPARED);
    return result.Value();
}

int32_t AVMetadataHelperServer::SetSource(int32_t fd, int64_t offset, int64_t size, int32_t usage)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::SetSource_fd");
    MEDIA_LOGD("Current is fd source, offset: %{public}" PRIi64 ", size: %{public}" PRIi64 " usage: %{public}u",
               offset, size, usage);

    uriHelper_ = std::make_unique<UriHelper>(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(!uriHelper_->FormattedUri().empty(),
                             MSERR_INVALID_VAL,
                             "Failed to construct formatted uri");
    CHECK_AND_RETURN_RET_LOG(uriHelper_->AccessCheck(UriHelper::URI_READ), MSERR_INVALID_VAL, "Failed to read the fd");

    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(
            IEngineFactory::Scene::SCENE_AVMETADATA, appUid_, uriHelper_->FormattedUri());
        CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, (int32_t)MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED,
            "Failed to get engine factory");
        avMetadataHelperEngine_ = engineFactory->CreateAVMetadataHelperEngine();
        CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr,
            (int32_t)MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED, "Failed to create avmetadatahelper engine");

        int32_t ret = avMetadataHelperEngine_->SetSource(uriHelper_->FormattedUri(), usage);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "0x%{public}06" PRIXPTR " SetSource failed!",
            FAKE_POINTER(this));
        return ret;
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    if (result.Value() == MSERR_OK) {
        ChangeState(HelperStates::HELPER_PREPARED);
    } else {
        ChangeState(HelperStates::HELPER_STATE_ERROR);
    }
    return result.Value();
}

int32_t AVMetadataHelperServer::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::SetSource dataSrc");
    MEDIA_LOGD("AVMetadataHelperServer SetSource");
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "data source is nullptr");
    dataSrc_ = dataSrc;
    std::string url = "media data source";
    config_.url = url;

    auto task = std::make_shared<TaskHandler<int32_t>>([&, this] {
        auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(
            IEngineFactory::Scene::SCENE_AVMETADATA, appUid_, config_.url);
        CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, (int32_t)MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED,
            "Failed to get engine factory");
        avMetadataHelperEngine_ = engineFactory->CreateAVMetadataHelperEngine();
        CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr,
            (int32_t)MSERR_CREATE_AVMETADATAHELPER_ENGINE_FAILED, "Failed to create avmetadatahelper engine");

        int32_t ret = avMetadataHelperEngine_->SetSource(dataSrc_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetSource failed!");

        int64_t size = 0;
        (void)dataSrc_->GetSize(size);
        if (size == -1) {
            config_.looping = false;
            isLiveStream_ = true;
        }
        return ret;
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "EnqueueTask failed");

    auto result = task->GetResult();
    ChangeState(HelperStates::HELPER_PREPARED);
    return result.Value();
}

std::string AVMetadataHelperServer::ResolveMetadata(int32_t key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::ResolveMetadata_key");
    MEDIA_LOGD("Key is %{public}d", key);
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, "", "avMetadataHelperEngine_ is nullptr");
    auto task = std::make_shared<TaskHandler<std::string>>([&, this] {
        return avMetadataHelperEngine_->ResolveMetadata(key);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, "", "EnqueueTask failed");

    auto result = task->GetResult();
    ChangeState(HelperStates::HELPER_CALL_DONE);
    return result.Value();
}

std::unordered_map<int32_t, std::string> AVMetadataHelperServer::ResolveMetadata()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::ResolveMetadata");
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, {}, "avMetadataHelperEngine_ is nullptr");
    auto task = std::make_shared<TaskHandler<std::unordered_map<int32_t, std::string>>>([&, this] {
        return avMetadataHelperEngine_->ResolveMetadata();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, {}, "EnqueueTask failed");

    auto result = task->GetResult();
    ChangeState(HelperStates::HELPER_CALL_DONE);
    return result.Value();
}

std::shared_ptr<Meta> AVMetadataHelperServer::GetAVMetadata()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::ResolveMetadata");
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, {}, "avMetadataHelperEngine_ is nullptr");
    auto task = std::make_shared<TaskHandler<std::shared_ptr<Meta>>>([&, this] {
        return avMetadataHelperEngine_->GetAVMetadata();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, {}, "EnqueueTask failed");

    auto result = task->GetResult();
    ChangeState(HelperStates::HELPER_CALL_DONE);
    return result.Value();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServer::FetchArtPicture()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::FetchArtPicture");
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, {}, "avMetadataHelperEngine_ is nullptr");
    auto task = std::make_shared<TaskHandler<std::shared_ptr<AVSharedMemory>>>([&, this] {
        return avMetadataHelperEngine_->FetchArtPicture();
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "EnqueueTask failed");

    auto result = task->GetResult();
    if (result.Value() == nullptr) {
        MEDIA_LOGE("FetchArtPicture result is nullptr.");
        NotifyErrorCallback(HelperErrorType::INVALID_RESULT, "FetchArtPicture result is nullptr.");
        return nullptr;
    }
    ChangeState(HelperStates::HELPER_CALL_DONE);
    return result.Value();
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServer::FetchFrameAtTime(int64_t timeUs, int32_t option,
    const OutputConfiguration &param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::FetchFrameAtTime");
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperEngine_ != nullptr, nullptr, "avMetadataHelperEngine_ is nullptr");
    auto task = std::make_shared<TaskHandler<std::shared_ptr<AVSharedMemory>>>([&, this] {
        return avMetadataHelperEngine_->FetchFrameAtTime(timeUs, option, param);
    });
    int32_t ret = taskQue_.EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "EnqueueTask failed");

    auto result = task->GetResult();
    ChangeState(HelperStates::HELPER_CALL_DONE);
    return result.Value();
}

void AVMetadataHelperServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("AVMetadataHelperServer::Release");
    auto task = std::make_shared<TaskHandler<void>>([&, this] {
        avMetadataHelperEngine_ = nullptr;
    });
    (void)taskQue_.EnqueueTask(task);
    (void)task->GetResult();
    uriHelper_ = nullptr;
    ChangeState(HelperStates::HELPER_RELEASED);
    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        helperCb_ = nullptr;
    }
}

int32_t AVMetadataHelperServer::SetHelperCallback(const std::shared_ptr<HelperCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");

    if (currState_ != HELPER_IDLE) {
        MEDIA_LOGE("Can not SetHelperCallback, currentState is %{public}s",
            GetStatusDescription(currState_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        helperCb_ = callback;
    }
    return MSERR_OK;
}

void AVMetadataHelperServer::ChangeState(const HelperStates state)
{
    switch (state) {
        case HELPER_PREPARED:
            if (currState_ == HELPER_IDLE) {
                currState_ = HELPER_PREPARED;
                NotifyInfoCallback(HELPER_INFO_TYPE_STATE_CHANGE, currState_);
            } else {
                NotifyErrorCallback(HelperErrorType::INVALID_OPERATION, "State error, current Operation is invalid.");
            }
            break;
        case HELPER_CALL_DONE:
            if (currState_ == HELPER_CALL_DONE || currState_ == HELPER_PREPARED) {
                currState_ = HELPER_CALL_DONE;
                NotifyInfoCallback(HELPER_INFO_TYPE_STATE_CHANGE, currState_);
            } else {
                NotifyErrorCallback(HelperErrorType::INVALID_OPERATION, "State error, current Operation is invalid.");
            }
            break;
        case HELPER_RELEASED:
            if (currState_ == HELPER_IDLE || currState_ == HELPER_PREPARED || currState_ == HELPER_CALL_DONE) {
                currState_ = HELPER_RELEASED;
                NotifyInfoCallback(HELPER_INFO_TYPE_STATE_CHANGE, currState_);
            } else {
                NotifyErrorCallback(HelperErrorType::INVALID_OPERATION, "State error, current Operation is invalid.");
            }
            break;
        case HELPER_STATE_ERROR:
            currState_ = HELPER_STATE_ERROR;
            NotifyInfoCallback(HELPER_INFO_TYPE_STATE_CHANGE, currState_);
            break;
        default:
            MEDIA_LOGI("Changed state is invalid.");
            break;
    }
}

void AVMetadataHelperServer::NotifyErrorCallback(int32_t code, const std::string msg)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    MEDIA_LOGD("NotifyErrorCallback error code: %{public}d", code);
    if (helperCb_ != nullptr) {
        helperCb_->OnError(code, msg);
    }
}

void AVMetadataHelperServer::NotifyInfoCallback(HelperOnInfoType type, int32_t extra)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    MEDIA_LOGD("NotifyInfoCallback, extra: %{public}d", extra);
    if (helperCb_ != nullptr) {
        helperCb_->OnInfo(type, extra);
    }
}

const std::string &AVMetadataHelperServer::GetStatusDescription(int32_t status)
{
    static const std::string ILLEGAL_STATE = "PLAYER_STATUS_ILLEGAL";
    if (status < HELPER_STATE_ERROR || status > HELPER_RELEASED) {
        return ILLEGAL_STATE;
    }

    return STATUS_TO_STATUS_DESCRIPTION_TABLE.find(status)->second;
}
} // namespace Media
} // namespace OHOS
