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

#include "player_server.h"
#include <map>
#include <unistd.h>
#include <unordered_set>
#include "media_log.h"
#include "media_errors.h"
#include "media_utils.h"
#include "engine_factory_repo.h"
#include "player_server_state.h"
#include "media_dfx.h"
#include "ipc_skeleton.h"
#include "media_permission.h"
#include "accesstoken_kit.h"
#include "av_common.h"
#include "parameter.h"
#include "parameters.h"
#include "concurrent_task_client.h"
#include "qos.h"
#include "player_server_event_receiver.h"

using namespace OHOS::QOS;

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServer"};
    constexpr int32_t MAX_SUBTITLE_TRACK_NUN = 8;
}

namespace OHOS {
namespace Media {
const std::string START_TAG = "PlayerCreate->Start";
const std::string STOP_TAG = "PlayerStop->Destroy";
static const std::unordered_map<int32_t, std::string> STATUS_TO_STATUS_DESCRIPTION_TABLE = {
    {PLAYER_STATE_ERROR, "PLAYER_STATE_ERROR"},
    {PLAYER_IDLE, "PLAYER_IDLE"},
    {PLAYER_INITIALIZED, "PLAYER_INITIALIZED"},
    {PLAYER_PREPARING, "PLAYER_PREPARING"},
    {PLAYER_PREPARED, "PLAYER_PREPARED"},
    {PLAYER_STARTED, "PLAYER_STARTED"},
    {PLAYER_PAUSED, "PLAYER_PAUSED"},
    {PLAYER_STOPPED, "PLAYER_STOPPED"},
    {PLAYER_PLAYBACK_COMPLETE, "PLAYER_PLAYBACK_COMPLETE"},
};
#ifdef SUPPORT_VIDEO
class VideoPlayerManager : public NoCopyable {
public:
    static VideoPlayerManager &GetInstance();
    int32_t RegisterVideoPlayer(PlayerServer *player);
    void UnRegisterVideoPlayer(PlayerServer *player);
private:
    VideoPlayerManager() = default;
    ~VideoPlayerManager() = default;
    std::unordered_set<PlayerServer *> videoPlayerList;
    std::mutex mutex_;
};

VideoPlayerManager &VideoPlayerManager::GetInstance()
{
    static VideoPlayerManager instance;
    return instance;
}

int32_t VideoPlayerManager::RegisterVideoPlayer(PlayerServer *player)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (videoPlayerList.find(player) != videoPlayerList.end()) {
        return MSERR_OK;
    }
    videoPlayerList.insert(player);
    return MSERR_OK;
}

void VideoPlayerManager::UnRegisterVideoPlayer(PlayerServer *player)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (videoPlayerList.erase(player) == 0) {
        MEDIA_LOGI("0x%{public}06" PRIXPTR " Not in videoPlayer list", FAKE_POINTER(player));
    }
}
#endif
std::shared_ptr<IPlayerService> PlayerServer::Create()
{
    std::shared_ptr<PlayerServer> server = std::make_shared<PlayerServer>();
    CHECK_AND_RETURN_RET_LOG(server != nullptr, nullptr, "failed to new PlayerServer");

    (void)server->Init();
    return server;
}

PlayerServer::PlayerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    instanceId_ = HiviewDFX::HiTraceChain::GetId().GetChainId();
}

PlayerServer::~PlayerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
#ifdef SUPPORT_VIDEO
    VideoPlayerManager::GetInstance().UnRegisterVideoPlayer(this);
#endif
}

int32_t PlayerServer::Init()
{
    MediaTrace trace("PlayerServer::Init");

    idleState_ = std::make_shared<IdleState>(*this);
    initializedState_ = std::make_shared<InitializedState>(*this);
    preparingState_ = std::make_shared<PreparingState>(*this);
    preparedState_ = std::make_shared<PreparedState>(*this);
    playingState_ = std::make_shared<PlayingState>(*this);
    pausedState_ = std::make_shared<PausedState>(*this);
    stoppedState_ = std::make_shared<StoppedState>(*this);
    playbackCompletedState_ = std::make_shared<PlaybackCompletedState>(*this);
    appTokenId_ = IPCSkeleton::GetCallingTokenID();
    appUid_ = IPCSkeleton::GetCallingUid();
    appPid_ = IPCSkeleton::GetCallingPid();
    MEDIA_LOGD("Get app uid: %{public}d, app pid: %{public}d, app tokenId: %{public}u", appUid_, appPid_, appTokenId_);

    PlayerServerStateMachine::Init(idleState_);

    std::string bootState = system::GetParameter("bootevent.boot.completed", "false");
    isBootCompleted_.store(bootState == "true");
    if (isBootCompleted_.load() && appUid_ != 0) {
        int32_t userId = -1;
        AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(appUid_, userId);
        userId_.store(userId);
        std::weak_ptr<PlayerServer> server = std::static_pointer_cast<PlayerServer>(shared_from_this());
        commonEventReceiver_ = std::make_shared<PlayerServerCommonEventReceiver>(server);
    }
    MEDIA_LOGI("0x%{public}06" PRIXPTR "Dump Info: lastOpStatus: %{public}s, lastErrMsg: %{public}s, "
        "speedMode: %{public}d, looping: %{public}s, effectMode: %{public}d, leftVolume: %{public}f, "
        "rightVolume: %{public}f", FAKE_POINTER(this), lastOpStatus_?"true":"false",
        lastErrMsg_.c_str(), config_.speedMode, config_.looping?"true":"false", config_.effectMode,
        config_.leftVolume, config_.rightVolume);
    return MSERR_OK;
}

int32_t PlayerServer::SetSource(const std::string &url)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetSource url");
    CHECK_AND_RETURN_RET_LOG(!url.empty(), MSERR_INVALID_VAL, "url is empty");

    MEDIA_LOGW("0x%{public}06" PRIXPTR " KPI-TRACE: PlayerServer SetSource in(url)", FAKE_POINTER(this));
    if (url.find("http") != std::string::npos) {
        int32_t permissionResult = MediaPermission::CheckNetWorkPermission(appUid_, appPid_, appTokenId_);
        if (permissionResult != Security::AccessToken::PERMISSION_GRANTED) {
            MEDIA_LOGE("user do not have the right to access INTERNET");
            OnErrorMessage(MSERR_USER_NO_PERMISSION, "user do not have the right to access INTERNET");
            return MSERR_INVALID_OPERATION;
        }
    }
    config_.url = url;
    int32_t ret = InitPlayEngine(url);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");
    return ret;
}

int32_t PlayerServer::SetSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetSource dataSrc");
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, MSERR_INVALID_VAL, "data source is nullptr");
    MEDIA_LOGW("KPI-TRACE: PlayerServer SetSource in(dataSrc)");
    dataSrc_ = dataSrc;
    std::string url = "media data source";
    config_.url = url;
    int32_t ret = InitPlayEngine(url);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "InitPlayEngine Failed!");
    int64_t size = 0;
    (void)dataSrc_->GetSize(size);
    if (size == -1) {
        config_.looping = false;
        config_.speedMode = SPEED_FORWARD_1_00_X;
        isLiveStream_ = true;
    }
    return ret;
}

int32_t PlayerServer::SetSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetSource fd");
    MEDIA_LOGW("KPI-TRACE: PlayerServer SetSource in(fd), fd: %{public}d, offset: %{public}" PRId64
        ", size: %{public}" PRId64, fd, offset, size);
    int32_t ret;
    if (uriHelper_ != nullptr) {
        std::string uri = uriHelper_->FormattedUri();
        MEDIA_LOGI("UriHelper already existed, uri: %{public}s", uri.c_str());
        ret = InitPlayEngine(uri);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");
    } else {
        MEDIA_LOGI("UriHelper is nullptr, create a new instance.");
        auto uriHelper = std::make_unique<UriHelper>(fd, offset, size);
        CHECK_AND_RETURN_RET_LOG(uriHelper->AccessCheck(UriHelper::URI_READ),
            MSERR_INVALID_VAL, "Failed to read the fd");
        ret = InitPlayEngine(uriHelper->FormattedUri());
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");
        uriHelper_ = std::move(uriHelper);
    }
    config_.url = "file descriptor source";

    return ret;
}

int32_t PlayerServer::InitPlayEngine(const std::string &url)
{
    if (lastOpStatus_ != PLAYER_IDLE) {
        MEDIA_LOGE("current state is: %{public}s, not support SetSource", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    // only remove OHOS::ConcurrentTask::ConcurrentTaskClient::GetInstance().RequestAuth on master branch

    int32_t ret = taskMgr_.Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "task mgr init failed");
    MEDIA_LOGI("current url is : %{public}s", url.c_str());
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(
        IEngineFactory::Scene::SCENE_PLAYBACK, appUid_, url);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_PLAYER_ENGINE_FAILED,
        "failed to get engine factory");
    playerEngine_ = engineFactory->CreatePlayerEngine(appUid_, appPid_, appTokenId_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_CREATE_PLAYER_ENGINE_FAILED,
        "failed to create player engine");

    if (dataSrc_ == nullptr) {
        ret = playerEngine_->SetSource(url);
    } else {
        ret = playerEngine_->SetSource(dataSrc_);
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetSource Failed!");

    std::shared_ptr<IPlayerEngineObs> obs = shared_from_this();
    ret = playerEngine_->SetObs(obs);

    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetObs Failed!");

    lastOpStatus_ = PLAYER_INITIALIZED;
    ChangeState(initializedState_);

    Format format;
    OnInfo(INFO_TYPE_STATE_CHANGE, PLAYER_INITIALIZED, format);
    return MSERR_OK;
}

int32_t PlayerServer::AddSubSource(const std::string &url)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr!");

    if (lastOpStatus_ != PLAYER_PAUSED && lastOpStatus_ != PLAYER_PREPARED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE && lastOpStatus_ != PLAYER_STARTED) {
        MEDIA_LOGE("Can not add sub source, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (subtitleTrackNum_ >= MAX_SUBTITLE_TRACK_NUN) {
        MEDIA_LOGE("Can not add sub source, subtitle track num is %{public}u, exceed the max num.", subtitleTrackNum_);
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGD("PlayerServer AddSubSource in(url).");
    auto task = std::make_shared<TaskHandler<void>>([this, url]() {
        MediaTrace::TraceBegin("PlayerServer::AddSubSource", FAKE_POINTER(this));
        (void)playerEngine_->AddSubSource(url);
    });
    (void)taskMgr_.LaunchTask(task, PlayerServerTaskType::STATE_CHANGE, "subsource");

    return MSERR_OK;
}

int32_t PlayerServer::AddSubSource(int32_t fd, int64_t offset, int64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not add sub source, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (subtitleTrackNum_ >= MAX_SUBTITLE_TRACK_NUN) {
        MEDIA_LOGE("Can not add sub source, subtitle track num is %{public}u, exceed the max num", subtitleTrackNum_);
        return MSERR_INVALID_OPERATION;
    }

    auto uriHelper = std::make_shared<UriHelper>(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(uriHelper->AccessCheck(UriHelper::URI_READ), MSERR_INVALID_VAL, "Failed to read the fd");

    MEDIA_LOGD("PlayerServer AddSubSource in(fd)");
    auto task = std::make_shared<TaskHandler<void>>([this, uriHelper]() {
        MediaTrace::TraceBegin("PlayerServer::AddSubSource", FAKE_POINTER(this));
        (void)playerEngine_->AddSubSource(uriHelper->FormattedUri());
        subUriHelpers_.emplace_back(uriHelper);
    });
    (void)taskMgr_.LaunchTask(task, PlayerServerTaskType::STATE_CHANGE, "subsource");

    return MSERR_OK;
}

int32_t PlayerServer::Prepare()
{
    if (inReleasing_.load()) {
        MEDIA_LOGE("Can not Prepare, now in releasing");
        return MSERR_INVALID_OPERATION;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("KPI-TRACE: PlayerServer Prepare in");

    if (lastOpStatus_ == PLAYER_INITIALIZED || lastOpStatus_ == PLAYER_STOPPED) {
        return OnPrepare(false);
    } else {
        MEDIA_LOGE("Can not Prepare, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
}

int32_t PlayerServer::SetRenderFirstFrame(bool display)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ != PLAYER_INITIALIZED) {
        MEDIA_LOGE("Can not SetRenderFirstFrame, currentState is not PLAYER_INITIALIZED");
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer SetRenderFirstFrame in, display %{public}d", display);

    if (isLiveStream_) {
        MEDIA_LOGE("Can not SetRenderFirstFrame, it is live-stream");
        return MSERR_OK;
    }

    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->SetRenderFirstFrame(display);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetRenderFirstFrame Failed!");
    }
    return MSERR_OK;
}

int32_t PlayerServer::PrepareAsync()
{
    if (inReleasing_.load()) {
        MEDIA_LOGE("Can not Prepare, now in releasing");
        return MSERR_INVALID_OPERATION;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("KPI-TRACE: PlayerServer PrepareAsync in");

    if (lastOpStatus_ == PLAYER_INITIALIZED || lastOpStatus_ == PLAYER_STOPPED) {
        return OnPrepare(false);
    } else {
        MEDIA_LOGE("Can not Prepare, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
}

int32_t PlayerServer::OnPrepare(bool sync)
{
    MEDIA_LOGD("KPI-TRACE: PlayerServer OnPrepare in");
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    int32_t ret = MSERR_OK;
    lastOpStatus_ = PLAYER_PREPARED;
    playerEngine_->SetInterruptState(false);
    auto preparedTask = std::make_shared<TaskHandler<int32_t>>([this]() {
        MediaTrace::TraceBegin("PlayerServer::PrepareAsync", FAKE_POINTER(this));
#ifdef SUPPORT_VIDEO
        if (surface_ != nullptr) {
            int32_t res = playerEngine_->SetVideoSurface(surface_);
            CHECK_AND_RETURN_RET_LOG(res == MSERR_OK,
                static_cast<int32_t>(MSERR_INVALID_OPERATION), "Engine SetVideoSurface Failed!");
        }
#endif
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        return currState->Prepare();
    });

    ret = taskMgr_.LaunchTask(preparedTask, PlayerServerTaskType::STATE_CHANGE, "prepare");
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Prepare launch task failed");

    if (sync) {
        (void)preparedTask->GetResult(); // wait HandlePrpare
    }
    MEDIA_LOGD("KPI-TRACE: PlayerServer OnPrepare out");
    return MSERR_OK;
}

int32_t PlayerServer::HandlePrepare()
{
    MEDIA_LOGI("KPI-TRACE: PlayerServer HandlePrepare in");
    int32_t ret = playerEngine_->PrepareAsync();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Server Prepare Failed!");

    if (config_.leftVolume < 1.0f) {
        (void)playerEngine_->SetVolume(config_.leftVolume, config_.rightVolume);
    }
    if (config_.looping) {
        (void)playerEngine_->SetLooping(config_.looping);
    }
    if (config_.speedMode != SPEED_FORWARD_1_00_X) {
        MediaTrace::TraceBegin("PlayerServer::SetPlaybackSpeed", FAKE_POINTER(this));
        auto rateTask = std::make_shared<TaskHandler<void>>([this]() {
            int ret = playerEngine_->SetPlaybackSpeed(config_.speedMode);
            CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Engine SetPlaybackSpeed Failed!");
        });
        auto cancelTask = std::make_shared<TaskHandler<void>>([this]() {
            MEDIA_LOGI("Interrupted speed action");
            taskMgr_.MarkTaskDone("interrupted speed done");
        });

        (void)taskMgr_.SpeedTask(rateTask, cancelTask, "prepare-speed", config_.speedMode);
    }

    if (config_.effectMode != OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT) {
        MediaTrace::TraceBegin("PlayerServer::SetAudioEffectMode", FAKE_POINTER(this));
        auto effectTask = std::make_shared<TaskHandler<void>>([this]() {
            int ret = playerEngine_->SetAudioEffectMode(config_.effectMode);
            taskMgr_.MarkTaskDone("SetAudioEffectMode done");
            CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Engine SetAudioEffectMode Failed!");
        });
        (void)taskMgr_.LaunchTask(effectTask, PlayerServerTaskType::STATE_CHANGE, "SetAudioEffectMode", nullptr);
    }
    
    return MSERR_OK;
}

int32_t PlayerServer::Play()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("PlayerServer Play in");
    if (lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE ||
        lastOpStatus_ == PLAYER_PAUSED) {
        return OnPlay();
    } else {
        MEDIA_LOGE("Can not Play, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
}

int32_t PlayerServer::OnPlay()
{
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    if (lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE && dataSrc_ != nullptr) {
        int64_t size = 0;
        (void)dataSrc_->GetSize(size);
        if (size == -1) {
            MEDIA_LOGE("Can not play in complete status, it is live-stream");
            OnErrorMessage(MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "Can not play in complete status, it is live-stream");
            return MSERR_INVALID_OPERATION;
        }
    }

    auto playingTask = std::make_shared<TaskHandler<void>>([this]() {
        MediaTrace::TraceBegin("PlayerServer::Play", FAKE_POINTER(this));
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Play();
    });
    MEDIA_LOGD("PlayerServer OnPlay in");
    int ret = taskMgr_.LaunchTask(playingTask, PlayerServerTaskType::STATE_CHANGE, "play");
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Play failed");

    lastOpStatus_ = PLAYER_STARTED;
    return MSERR_OK;
}

int32_t PlayerServer::HandlePlay()
{
    int32_t ret = playerEngine_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Play Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::BackGroundChangeState(PlayerStates state, bool isBackGroundCb)
{
    backgroundState_ = state;
    isBackgroundCb_ = isBackGroundCb;
    if (state == PLAYER_PAUSED) {
        isBackgroundChanged_ = true;
        return PlayerServer::Pause();
    } else if (state == PLAYER_STARTED) {
        isBackgroundChanged_ = true;
        return PlayerServer::Play();
    }
    return MSERR_INVALID_OPERATION;
}

int32_t PlayerServer::Pause()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGI("0x%{public}06" PRIXPTR " PlayerServer Pause in", FAKE_POINTER(this));

    if (lastOpStatus_ == PLAYER_STARTED) {
        return OnPause();
    } else {
        MEDIA_LOGE("Can not Pause, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
}

int32_t PlayerServer::OnPause()
{
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    MEDIA_LOGI("0x%{public}06" PRIXPTR " PlayerServer OnPause in", FAKE_POINTER(this));

    auto pauseTask = std::make_shared<TaskHandler<void>>([this]() {
        MediaTrace::TraceBegin("PlayerServer::Pause", FAKE_POINTER(this));
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Pause();
    });

    int ret = taskMgr_.LaunchTask(pauseTask, PlayerServerTaskType::STATE_CHANGE, "pause");
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Pause failed");

    lastOpStatus_ = PLAYER_PAUSED;
    return MSERR_OK;
}

int32_t PlayerServer::HandlePause()
{
    MEDIA_LOGI("KPI-TRACE: PlayerServer HandlePause in");
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_INVALID_OPERATION, "playerEngine_ is nullptr");
    int32_t ret = playerEngine_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Pause Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE || lastOpStatus_ == PLAYER_PAUSED) {
        MediaTrace::TraceBegin("PlayerServer::Stop", FAKE_POINTER(this));
        return OnStop(false);
    } else {
        MEDIA_LOGE("Can not Stop, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
}

int32_t PlayerServer::OnStop(bool sync)
{
    MEDIA_LOGD("PlayerServer OnStop in");
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    playerEngine_->SetInterruptState(true);
    taskMgr_.ClearAllTask();

    auto stopTask = std::make_shared<TaskHandler<void>>([this]() {
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Stop();
    });

    (void)taskMgr_.LaunchTask(stopTask, PlayerServerTaskType::STATE_CHANGE, "stop");
    if (sync) {
        (void)stopTask->GetResult(); // wait HandleStop
    }
    lastOpStatus_ = PLAYER_STOPPED;
    MEDIA_LOGD("PlayerServer OnStop out");
    return MSERR_OK;
}

int32_t PlayerServer::HandleStop()
{
    int32_t ret = playerEngine_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Stop Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::Reset");
    if (lastOpStatus_ == PLAYER_IDLE) {
        MEDIA_LOGE("Can not Reset, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    return OnReset();
}

int32_t PlayerServer::OnReset()
{
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    if (lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE || lastOpStatus_ == PLAYER_PAUSED) {
        disableStoppedCb_ = true;
        (void)OnStop(true);
    }

    taskMgr_.ClearAllTask();
    auto idleTask = std::make_shared<TaskHandler<void>>([this]() {
        ChangeState(idleState_);
    });
    (void)taskMgr_.LaunchTask(idleTask, PlayerServerTaskType::STATE_CHANGE, "reset");
    (void)idleTask->GetResult();
    (void)taskMgr_.Reset();
    lastOpStatus_ = PLAYER_IDLE;
    isLiveStream_ = false;
    subtitleTrackNum_ = 0;

    return MSERR_OK;
}

int32_t PlayerServer::HandleReset()
{
    (void)playerEngine_->Reset();
    playerEngine_ = nullptr;
    dataSrc_ = nullptr;
    config_.looping = false;
    uriHelper_ = nullptr;
    {
        decltype(subUriHelpers_) temp;
        temp.swap(subUriHelpers_);
    }
    lastErrMsg_.clear();
    errorCbOnce_ = false;
    disableStoppedCb_ = false;
    disableNextSeekDone_ = false;
    Format format;
    OnInfo(INFO_TYPE_STATE_CHANGE, PLAYER_IDLE, format);
    return MSERR_OK;
}

int32_t PlayerServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::Release");
    inReleasing_ = true;
    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        playerCb_ = nullptr;
    }
    MEDIA_LOGD("PlayerServer Release in");
    if (lastOpStatus_ != PLAYER_IDLE) {
        (void)OnReset();
    }
#ifdef SUPPORT_VIDEO
    if (surface_ != nullptr) {
        surface_ = nullptr;
    }
#endif
    return MSERR_OK;
}

int32_t PlayerServer::SetVolume(float leftVolume, float rightVolume)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetVolume, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer SetVolume in leftVolume %{public}f %{public}f", leftVolume, rightVolume);
    constexpr float maxVolume = 1.0f;
    if ((leftVolume < 0) || (leftVolume > maxVolume) || (rightVolume < 0) || (rightVolume > maxVolume)) {
        MEDIA_LOGE("SetVolume failed, the volume should be set to a value ranging from 0 to 5");
        return MSERR_INVALID_OPERATION;
    }

    config_.leftVolume = leftVolume;
    config_.rightVolume = rightVolume;
    if (IsEngineStarted()) {
        auto task = std::make_shared<TaskHandler<void>>([this]() {
            (void)playerEngine_->SetVolume(config_.leftVolume, config_.rightVolume);
            taskMgr_.MarkTaskDone("volume done");
        });
        (void)taskMgr_.LaunchTask(task, PlayerServerTaskType::STATE_CHANGE, "volume");
    } else {
        MEDIA_LOGI("Waiting for the engine state is <prepared> to take effect");
    }

    Format format;
    (void)format.PutFloatValue(PlayerKeys::PLAYER_VOLUME_LEVEL, leftVolume);
    MEDIA_LOGI("SetVolume callback");
    OnInfoNoChangeStatus(INFO_TYPE_VOLUME_CHANGE, 0, format);
    return MSERR_OK;
}

bool PlayerServer::IsEngineStarted()
{
    if (playerEngine_ != nullptr) {
        if (GetCurrState() == preparedState_ || GetCurrState() == playingState_ ||
            GetCurrState() == pausedState_ || GetCurrState() == playbackCompletedState_) {
            return true;
        }
    }
    return false;
}

bool PlayerServer::IsValidSeekMode(PlayerSeekMode mode)
{
    switch (mode) {
        case SEEK_PREVIOUS_SYNC:
        case SEEK_NEXT_SYNC:
        case SEEK_CLOSEST_SYNC:
        case SEEK_CLOSEST:
            break;
        default:
            MEDIA_LOGE("Unknown seek mode %{public}d", mode);
            return false;
    }
    return true;
}

int32_t PlayerServer::Seek(int32_t mSeconds, PlayerSeekMode mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    MEDIA_LOGI("KPI-TRACE: PlayerServer Seek in");
    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not Seek, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    if (IsValidSeekMode(mode) != true) {
        MEDIA_LOGE("Seek failed, inValid mode");
        return MSERR_INVALID_VAL;
    }

    if (isLiveStream_) {
        MEDIA_LOGE("Can not Seek, it is live-stream");
        OnErrorMessage(MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "Can not Seek, it is live-stream");
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGD("seek position %{public}d, seek mode is %{public}d", mSeconds, mode);
    mSeconds = std::max(0, mSeconds);

    auto seekTask = std::make_shared<TaskHandler<void>>([this, mSeconds, mode]() {
        MediaTrace::TraceBegin("PlayerServer::Seek", FAKE_POINTER(this));
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->Seek(mSeconds, mode);
    });

    auto cancelTask = std::make_shared<TaskHandler<void>>([this, mSeconds]() {
        MEDIA_LOGI("Interrupted seek action");
        Format format;
        OnInfoNoChangeStatus(INFO_TYPE_SEEKDONE, mSeconds, format);
        taskMgr_.MarkTaskDone("interrupted seek done");
    });

    int32_t ret = taskMgr_.SeekTask(seekTask, cancelTask, "seek", mode, mSeconds);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Seek failed");

    MEDIA_LOGI("Queue seekTask end, position %{public}d, seek mode is %{public}d", mSeconds, mode);
    return MSERR_OK;
}

int32_t PlayerServer::HandleSeek(int32_t mSeconds, PlayerSeekMode mode)
{
    MEDIA_LOGI("KPI-TRACE: PlayerServer HandleSeek in, mSeconds: %{public}d, mSeconds: %{public}d, "
        "instanceId: %{public}" PRIu64 "", mSeconds, mode, instanceId_);
    int32_t ret = playerEngine_->Seek(mSeconds, mode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine Seek Failed!");

    return MSERR_OK;
}

int32_t PlayerServer::GetCurrentTime(int32_t &currentTime)
{
    // delete lock, cannot be called concurrently with Reset or Release
    currentTime = -1;
    if (lastOpStatus_ == PLAYER_IDLE || lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetCurrentTime, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    if (isLiveStream_ && dataSrc_ == nullptr) {
        MEDIA_LOGD("It is live-stream");
        return MSERR_OK;
    }

    MEDIA_LOGD("PlayerServer GetCurrentTime in, currentState is %{public}s",
        GetStatusDescription(lastOpStatus_).c_str());
    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        currentTime = 0;
        MEDIA_LOGD("get position at state: %{public}s, return 0", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_OK;
    }

    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->GetCurrentTime(currentTime);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetCurrentTime Failed!");
    }
    return MSERR_OK;
}

int32_t PlayerServer::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer GetVideoTrackInfo in");
    int32_t ret = playerEngine_->GetVideoTrackInfo(videoTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetVideoTrackInfo Failed!");
    return MSERR_OK;
}

int32_t PlayerServer::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer GetAudioTrackInfo in");
    int32_t ret = playerEngine_->GetAudioTrackInfo(audioTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetAudioTrackInfo Failed!");
    return MSERR_OK;
}

int32_t PlayerServer::GetSubtitleTrackInfo(std::vector<Format> &subtitleTrack)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer GetSubtitleTrackInfo in");
    int32_t ret = playerEngine_->GetSubtitleTrackInfo(subtitleTrack);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetSubtitleTrackInfo Failed!");
    return MSERR_OK;
}

int32_t PlayerServer::GetVideoWidth()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_STOPPED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer GetVideoWidth in");
    return playerEngine_->GetVideoWidth();
}

int32_t PlayerServer::GetVideoHeight()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (lastOpStatus_ != PLAYER_PREPARED && lastOpStatus_ != PLAYER_PAUSED &&
        lastOpStatus_ != PLAYER_STARTED && lastOpStatus_ != PLAYER_STOPPED &&
        lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE) {
        MEDIA_LOGE("Can not get track info, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer GetVideoHeight in");
    return playerEngine_->GetVideoHeight();
}

int32_t PlayerServer::GetDuration(int32_t &duration)
{
    // delete lock, cannot be called concurrently with Reset or Release
    if (lastOpStatus_ == PLAYER_IDLE || lastOpStatus_ == PLAYER_INITIALIZED || lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetDuration, currentState is %{public}s", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    MEDIA_LOGD("PlayerServer GetDuration in");
    duration = -1;
    if (playerEngine_ != nullptr) {
        int ret = playerEngine_->GetDuration(duration);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine GetDuration Failed!");
    }
    MEDIA_LOGD("PlayerServer GetDuration %{public}d", duration);
    return MSERR_OK;
}

void PlayerServer::ClearConfigInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);

    config_.looping = false;
    config_.leftVolume = INVALID_VALUE;
    config_.rightVolume = INVALID_VALUE;
    config_.speedMode = SPEED_FORWARD_1_00_X;
    config_.url = "";
}

int32_t PlayerServer::SetPlaybackSpeed(PlaybackRateMode mode)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if ((lastOpStatus_ != PLAYER_STARTED) && (lastOpStatus_ != PLAYER_PREPARED) &&
        (lastOpStatus_ != PLAYER_PAUSED) && (lastOpStatus_ != PLAYER_PLAYBACK_COMPLETE)) {
        MEDIA_LOGE("Can not SetPlaybackSpeed, currentState is %{public}s",
            GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer SetPlaybackSpeed in, mode %{public}d", mode);
    if (isLiveStream_) {
        MEDIA_LOGE("Can not SetPlaybackSpeed, it is live-stream");
        OnErrorMessage(MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "Can not SetPlaybackSpeed, it is live-stream");
        return MSERR_INVALID_OPERATION;
    }

    auto rateTask = std::make_shared<TaskHandler<void>>([this, mode]() {
        MediaTrace::TraceBegin("PlayerServer::SetPlaybackSpeed", FAKE_POINTER(this));
        auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
        (void)currState->SetPlaybackSpeed(mode);
    });

    auto cancelTask = std::make_shared<TaskHandler<void>>([this, mode]() {
        MEDIA_LOGI("Interrupted speed action");
        Format format;
        OnInfoNoChangeStatus(INFO_TYPE_SPEEDDONE, mode, format);
        taskMgr_.MarkTaskDone("interrupted speed done");
    });

    int ret = taskMgr_.SpeedTask(rateTask, cancelTask, "speed", config_.speedMode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SetPlaybackSpeed failed");

    return MSERR_OK;
}

int32_t PlayerServer::HandleSetPlaybackSpeed(PlaybackRateMode mode)
{
    if (config_.speedMode == mode) {
        MEDIA_LOGD("The speed mode is same, mode = %{public}d", mode);
        Format format;
        OnInfoNoChangeStatus(INFO_TYPE_SPEEDDONE, mode, format);
        taskMgr_.MarkTaskDone("set speed mode is same");
        MediaTrace::TraceEnd("PlayerServer::SetPlaybackSpeed", FAKE_POINTER(this));
        return MSERR_OK;
    }

    if (playerEngine_ != nullptr) {
        int ret = playerEngine_->SetPlaybackSpeed(mode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine SetPlaybackSpeed Failed!");
    }
    config_.speedMode = mode;
    return MSERR_OK;
}

int32_t PlayerServer::SetMediaSource(const std::shared_ptr<AVMediaSource> &mediaSource, AVPlayStrategy strategy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MediaTrace trace("PlayerServer::SetMediaSource");
    CHECK_AND_RETURN_RET_LOG(mediaSource != nullptr, MSERR_INVALID_VAL, "mediaSource is nullptr");
    InitPlayEngine(mediaSource->url);
    int ret = playerEngine_->SetMediaSource(mediaSource, strategy);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetMediaSource Failed!");
    config_.url = mediaSource->url;
    config_.header = mediaSource->header;
    config_.strategy_ = strategy;
    return MSERR_OK;
}

void PlayerServer::HandleEos()
{
    if (config_.looping.load()) {
        auto seekTask = std::make_shared<TaskHandler<void>>([this]() {
            MediaTrace::TraceBegin("PlayerServer::Seek", FAKE_POINTER(this));
            disableNextSeekDone_ = true;
            auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
            (void)currState->Seek(0, SEEK_PREVIOUS_SYNC);
        });

        auto cancelTask = std::make_shared<TaskHandler<void>>([this]() {
            MEDIA_LOGI("Interrupted seek action");
            taskMgr_.MarkTaskDone("interrupted seek done");
            disableNextSeekDone_ = false;
        });

        int32_t ret = taskMgr_.SeekTask(seekTask, cancelTask, "eos seek", SEEK_PREVIOUS_SYNC, 0);
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "Seek failed");
    }
}

int32_t PlayerServer::GetPlaybackSpeed(PlaybackRateMode &mode)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not GetDuration, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer GetPlaybackSpeed in");

    mode = config_.speedMode;
    return MSERR_OK;
}

int32_t PlayerServer::SelectBitRate(uint32_t bitRate)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (playerEngine_ != nullptr) {
        int ret = playerEngine_->SelectBitRate(bitRate);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "Engine SelectBitRate Failed!");
    }
    return MSERR_OK;
}

#ifdef SUPPORT_VIDEO
int32_t PlayerServer::SetVideoSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(VideoPlayerManager::GetInstance().RegisterVideoPlayer(this) == MSERR_OK,
        MSERR_DATA_SOURCE_OBTAIN_MEM_ERROR, "video player is no more than 13");
    CHECK_AND_RETURN_RET_LOG(surface != nullptr, MSERR_INVALID_VAL, "surface is nullptr");

    bool setSurfaceFirst = lastOpStatus_ == PLAYER_INITIALIZED;
    bool switchSurface = lastOpStatus_ == PLAYER_PREPARED ||
        lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PAUSED ||
        lastOpStatus_ == PLAYER_STOPPED ||
        lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE;
    
    if (setSurfaceFirst) {
        MEDIA_LOGI("set surface first in %{public}s state", GetStatusDescription(lastOpStatus_).c_str());
    } else if (switchSurface) {
        MEDIA_LOGI("switch surface in %{public}s state", GetStatusDescription(lastOpStatus_).c_str());
        if (surface_ == nullptr) {
            MEDIA_LOGE("old surface is required before switching surface");
            return MSERR_INVALID_OPERATION;
        }
    } else {
        MEDIA_LOGE("current state: %{public}s, can not SetVideoSurface", GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer SetVideoSurface in");
    surface_ = surface;
    if (switchSurface && playerEngine_ != nullptr) {
        int32_t res = playerEngine_->SetVideoSurface(surface_);
        CHECK_AND_RETURN_RET_LOG(res == MSERR_OK,
            static_cast<int32_t>(MSERR_INVALID_OPERATION), "Engine switch surface failed!");
    }
    return MSERR_OK;
}
#endif

int32_t PlayerServer::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
    bool svp)
{
    MEDIA_LOGI("PlayerServer SetDecryptConfig");
#ifdef SUPPORT_DRM
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(keySessionProxy != nullptr, MSERR_INVALID_VAL, "keySessionProxy is nullptr");

    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_INVALID_VAL, "playerEngine_ is nullptr");
    int32_t res = playerEngine_->SetDecryptConfig(keySessionProxy, svp);
    CHECK_AND_RETURN_RET_LOG(res == MSERR_OK,
        static_cast<int32_t>(MSERR_INVALID_OPERATION), "Engine SetDecryptConfig Failed!");
    MEDIA_LOGI("PlayerServer SetDecryptConfig out");
    return MSERR_OK;
#else
    (void)keySessionProxy;
    (void)svp;
    return 0;
#endif
}

bool PlayerServer::IsPlaying()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("0x%{public}06" PRIXPTR " Can not judge IsPlaying, currentState is PLAYER_STATE_ERROR",
            FAKE_POINTER(this));
        return false;
    }

    return lastOpStatus_ == PLAYER_STARTED;
}

bool PlayerServer::IsPrepared()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsPrepared, currentState is PLAYER_STATE_ERROR");
        return false;
    }

    return lastOpStatus_ == PLAYER_PREPARED;
}

bool PlayerServer::IsCompleted()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsCompleted, currentState is PLAYER_STATE_ERROR");
        return false;
    }

    return lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE;
}

bool PlayerServer::IsLooping()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not judge IsLooping, currentState is PLAYER_STATE_ERROR");
        return false;
    }

    return config_.looping;
}

int32_t PlayerServer::SetLooping(bool loop)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetLooping, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("PlayerServer SetLooping in, loop %{public}d", loop);

    if (isLiveStream_) {
        MEDIA_LOGE("Can not SetLooping, it is live-stream");
        OnErrorMessage(MSERR_EXT_API9_UNSUPPORT_CAPABILITY, "Can not SetLooping, it is live-stream");
        return MSERR_INVALID_OPERATION;
    }

    if (lastOpStatus_ == PLAYER_IDLE || lastOpStatus_ == PLAYER_INITIALIZED || GetCurrState() == preparingState_) {
        MEDIA_LOGI("Waiting for the engine state is <prepared> to take effect");
        config_.looping = loop;
        return MSERR_OK;
    }

    if (playerEngine_ != nullptr) {
        int32_t ret = playerEngine_->SetLooping(loop);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetLooping Failed!");
    }
    config_.looping = loop;
    return MSERR_OK;
}

int32_t PlayerServer::SetParameter(const Format &param)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (lastOpStatus_ == PLAYER_STATE_ERROR) {
        MEDIA_LOGE("Can not SetParameter, currentState is PLAYER_STATE_ERROR");
        return MSERR_INVALID_OPERATION;
    }

    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    if (param.ContainKey(PlayerKeys::AUDIO_EFFECT_MODE)) {
        int32_t effectMode = OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT;
        CHECK_AND_RETURN_RET(param.GetIntValue(PlayerKeys::AUDIO_EFFECT_MODE, effectMode), MSERR_INVALID_VAL);
        return SetAudioEffectMode(effectMode);
    }

    int32_t ret = playerEngine_->SetParameter(param);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_OPERATION, "SetParameter Failed!");

    if (param.ContainKey(PlayerKeys::CONTENT_TYPE)) {
        config_.effectMode = OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT;
    }

    return MSERR_OK;
}

int32_t PlayerServer::SetAudioEffectMode(const int32_t effectMode)
{
    MEDIA_LOGD("SetAudioEffectMode in");
    CHECK_AND_RETURN_RET(lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE || lastOpStatus_ == PLAYER_PAUSED, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(effectMode <= OHOS::AudioStandard::AudioEffectMode::EFFECT_DEFAULT &&
        effectMode >= OHOS::AudioStandard::AudioEffectMode::EFFECT_NONE, MSERR_INVALID_VAL,
        "Invalid effectMode parameter");
    int32_t ret = playerEngine_->SetAudioEffectMode(effectMode);
    if (ret == MSERR_OK) {
        config_.effectMode = effectMode;
    }

    return ret;
}

int32_t PlayerServer::SetPlayerCallback(const std::shared_ptr<PlayerCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");

    if (lastOpStatus_ != PLAYER_IDLE && lastOpStatus_ != PLAYER_INITIALIZED) {
        MEDIA_LOGE("Can not SetPlayerCallback, currentState is %{public}s",
            GetStatusDescription(lastOpStatus_).c_str());
        return MSERR_INVALID_OPERATION;
    }

    {
        std::lock_guard<std::mutex> lockCb(mutexCb_);
        playerCb_ = callback;
    }
    return MSERR_OK;
}

int32_t PlayerServer::SelectTrack(int32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET_LOG(lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PAUSED || lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE, MSERR_INVALID_OPERATION,
        "invalid state %{public}s", GetStatusDescription(lastOpStatus_).c_str());
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    auto task = std::make_shared<TaskHandler<void>>([this, index]() {
        MediaTrace::TraceBegin("PlayerServer::track", FAKE_POINTER(this));
        CHECK_AND_RETURN(IsEngineStarted());
        int32_t ret = playerEngine_->SelectTrack(index);
        taskMgr_.MarkTaskDone("SelectTrack done");
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "failed to SelectTrack");
    });
    int32_t ret = taskMgr_.LaunchTask(task, PlayerServerTaskType::STATE_CHANGE, "SelectTrack");
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "SelectTrack launch task failed");

    return MSERR_OK;
}

int32_t PlayerServer::DeselectTrack(int32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PAUSED || lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE, MSERR_INVALID_OPERATION,
        "invalid state %{public}s", GetStatusDescription(lastOpStatus_).c_str());
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    auto task = std::make_shared<TaskHandler<void>>([this, index]() {
        MediaTrace::TraceBegin("PlayerServer::track", FAKE_POINTER(this));
        CHECK_AND_RETURN(IsEngineStarted());
        int32_t ret = playerEngine_->DeselectTrack(index);
        taskMgr_.MarkTaskDone("DeselectTrack done");
        CHECK_AND_RETURN_LOG(ret == MSERR_OK, "failed to DeselectTrack");
    });
    int32_t ret = taskMgr_.LaunchTask(task, PlayerServerTaskType::STATE_CHANGE, "DeselectTrack");
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "DeselectTrack launch task failed");

    return MSERR_OK;
}

int32_t PlayerServer::GetCurrentTrack(int32_t trackType, int32_t &index)
{
    CHECK_AND_RETURN_RET_LOG(trackType >= MediaType::MEDIA_TYPE_AUD && trackType <= MediaType::MEDIA_TYPE_SUBTITLE,
        MSERR_INVALID_VAL, "Invalid trackType %{public}d", trackType);

    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(lastOpStatus_ == PLAYER_PREPARED || lastOpStatus_ == PLAYER_STARTED ||
        lastOpStatus_ == PLAYER_PAUSED || lastOpStatus_ == PLAYER_PLAYBACK_COMPLETE, MSERR_INVALID_OPERATION,
        "invalid state %{public}s", GetStatusDescription(lastOpStatus_).c_str());
    CHECK_AND_RETURN_RET_LOG(playerEngine_ != nullptr, MSERR_NO_MEMORY, "playerEngine_ is nullptr");

    return playerEngine_->GetCurrentTrack(trackType, index);
}

void PlayerServer::FormatToString(std::string &dumpString, std::vector<Format> &videoTrack)
{
    for (auto iter = videoTrack.begin(); iter != videoTrack.end(); iter++) {
        dumpString += iter->Stringify();
        dumpString += '\n';
    }
}

int32_t PlayerServer::DumpInfo(int32_t fd)
{
    std::string dumpString;
    if (playerEngine_ == nullptr) {
        dumpString +=
            "The engine is not created, note: engine can't be created until set source.\n";
    }
    dumpString += "PlayerServer current state is: " + GetStatusDescription(lastOpStatus_) + "\n";
    if (lastErrMsg_.size() != 0) {
        dumpString += "PlayerServer last error is: " + lastErrMsg_ + "\n";
    }
    dumpString += "PlayerServer url is: " + config_.url + "\n";
    dumpString += "PlayerServer play back speed is: " + std::to_string(config_.speedMode) + "\n";
    std::string loopflag = config_.looping ? "" : "not ";
    dumpString += "PlayerServer current " + loopflag + "in looping mode\n";
    dumpString += "PlayerServer left volume and right volume is: " +
        std::to_string(config_.leftVolume) + ", " + std::to_string(config_.rightVolume) + "\n";
    dumpString += "PlayerServer audio effect mode is: " + std::to_string(config_.effectMode) + "\n";
    if (playerEngine_ != nullptr) {
        dumpString += "PlayerServer enable HEBC: " + std::to_string(playerEngine_->GetHEBCMode()) + "\n";
        playerEngine_->OnDumpInfo(fd);
    }
    dumpString += "PlayerServer client bundle name is: " + GetClientBundleName(appUid_) + "\n";
    dumpString += "PlayerServer instance id is: " + std::to_string(instanceId_) + "\n";
    std::vector<Format> videoTrack;
    (void)GetVideoTrackInfo(videoTrack);
    dumpString += "PlayerServer video tracks info: \n";
    FormatToString(dumpString, videoTrack);
    
    std::vector<Format> audioTrack;
    (void)GetAudioTrackInfo(audioTrack);
    dumpString += "PlayerServer audio tracks info: \n";
    FormatToString(dumpString, audioTrack);
    
    int32_t currentTime = -1;
    (void)GetCurrentTime(currentTime);
    dumpString += "PlayerServer current time is: " + std::to_string(currentTime) + "\n";
    write(fd, dumpString.c_str(), dumpString.size());

    return MSERR_OK;
}

void PlayerServer::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    auto errorMsg = MSErrorToExtErrorString(static_cast<MediaServiceErrCode>(errorCode));
    return OnErrorMessage(errorCode, errorMsg);
}

void PlayerServer::OnErrorMessage(int32_t errorCode, const std::string &errorMsg)
{
    if (static_cast<MediaServiceExtErrCodeAPI9>(errorCode) == MSERR_EXT_API9_IO) {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " PlayerServer OnErrorMessage IO Error in", FAKE_POINTER(this));
        auto pauseTask = std::make_shared<TaskHandler<void>>([this, errorCode, errorMsg]() {
            MediaTrace::TraceBegin("PlayerServer::PauseIoError", FAKE_POINTER(this));
            auto currState = std::static_pointer_cast<BaseState>(GetCurrState());
            (void)currState->Pause();
            OnErrorCb(errorCode, errorMsg);
        });
        taskMgr_.LaunchTask(pauseTask, PlayerServerTaskType::STATE_CHANGE, "pause");
        MEDIA_LOGI("0x%{public}06" PRIXPTR " PlayerServer OnErrorMessage IO Error out", FAKE_POINTER(this));
        return;
    }
    OnErrorCb(errorCode, errorMsg);
}

void PlayerServer::OnErrorCb(int32_t errorCode, const std::string &errorMsg)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    lastErrMsg_ = errorMsg;
    FaultEventWrite(lastErrMsg_, "Player");
    if (playerCb_ != nullptr && !errorCbOnce_) {
        playerCb_->OnError(errorCode, errorMsg);
        errorCbOnce_ = true;
    }
}

void PlayerServer::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);
    // notify info
    int32_t ret = HandleMessage(type, extra, infoBody);
    if (type == INFO_TYPE_IS_LIVE_STREAM) {
        isLiveStream_ = true;
    } else if (type == INFO_TYPE_TRACK_NUM_UPDATE) {
        subtitleTrackNum_ = static_cast<uint32_t>(extra);
        return;
    }

    if (type == INFO_TYPE_DEFAULTTRACK || type == INFO_TYPE_TRACK_DONE || type == INFO_TYPE_ADD_SUBTITLE_DONE) {
        return;
    }

    if (playerCb_ != nullptr && type == INFO_TYPE_ERROR_MSG) {
        int32_t errorCode = extra;
        Format newInfo = infoBody;
        auto errorMsg = MSErrorToString(static_cast<MediaServiceErrCode>(errorCode));
        newInfo.PutIntValue(std::string(PlayerKeys::PLAYER_ERROR_TYPE), errorCode);
        newInfo.PutStringValue(std::string(PlayerKeys::PLAYER_ERROR_MSG), errorMsg);
        playerCb_->OnInfo(type, extra, newInfo);
        return;
    }

    if (playerCb_ != nullptr && ret == MSERR_OK) {
        if (isBackgroundChanged_ && type == INFO_TYPE_STATE_CHANGE && extra == backgroundState_) {
            MEDIA_LOGI("Background change state to %{public}d, Status reporting %{public}d", extra, isBackgroundCb_);
            if (isBackgroundCb_) {
                Format newInfo = infoBody;
                newInfo.PutIntValue(PlayerKeys::PLAYER_STATE_CHANGED_REASON, StateChangeReason::BACKGROUND);
                playerCb_->OnInfo(type, extra, newInfo);
                isBackgroundCb_ = false;
            }
            isBackgroundChanged_ = false;
        } else {
            playerCb_->OnInfo(type, extra, infoBody);
        }
    } else {
        MEDIA_LOGD("0x%{public}06" PRIXPTR " playerCb_ != nullptr %{public}d, ret %{public}d",
            FAKE_POINTER(this), playerCb_ != nullptr, ret);
    }
}

void PlayerServer::OnInfoNoChangeStatus(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    std::lock_guard<std::mutex> lockCb(mutexCb_);

    if (playerCb_ != nullptr) {
        playerCb_->OnInfo(type, extra, infoBody);
    }
}

const std::string &PlayerServer::GetStatusDescription(int32_t status)
{
    static const std::string ILLEGAL_STATE = "PLAYER_STATUS_ILLEGAL";
    if (status < PLAYER_STATE_ERROR || status > PLAYER_PLAYBACK_COMPLETE) {
        return ILLEGAL_STATE;
    }

    return STATUS_TO_STATUS_DESCRIPTION_TABLE.find(status)->second;
}

std::string PlayerServerState::GetStateName() const
{
    return name_;
}

int32_t PlayerServerStateMachine::HandleMessage(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    if (currState_ != nullptr) {
        return currState_->OnMessageReceived(type, extra, infoBody);
    }
    return MSERR_OK;
}

void PlayerServerStateMachine::Init(const std::shared_ptr<PlayerServerState> &state)
{
    currState_ = state;
}

void PlayerServerStateMachine::ChangeState(const std::shared_ptr<PlayerServerState> &state)
{
    {
        // Resolve the deadlock between reset and state callback
        std::unique_lock<std::recursive_mutex> lock(recMutex_);

        if (state == nullptr || (state == currState_)) {
            return;
        }

        if (currState_) {
            MEDIA_LOGD("exit state %{public}s", currState_->name_.c_str());
            currState_->StateExit();
        }
        MEDIA_LOGI("instance: 0x%{public}06" PRIXPTR " change state to %{public}s",
            FAKE_POINTER(this), state->name_.c_str());
        currState_ = state;
    }
    state->StateEnter();
}

std::shared_ptr<PlayerServerState> PlayerServerStateMachine::GetCurrState()
{
    std::unique_lock<std::recursive_mutex> lock(recMutex_);
    return currState_;
}

void PlayerServer::OnCommonEventReceived(const std::string &event)
{
    MEDIA_LOGI("instance: 0x%{public}06" PRIXPTR " receive event %{public}s",
            FAKE_POINTER(this), event.c_str());
    if (event == EventFwk::CommonEventSupport::COMMON_EVENT_USER_BACKGROUND) {
        std::weak_ptr<PlayerServer> server = std::static_pointer_cast<PlayerServer>(shared_from_this());
        auto pauseTask = std::make_shared<TaskHandler<void>>([server]() {
            std::shared_ptr<PlayerServer> spServer = server.lock();
            if (spServer != nullptr) {
                spServer->taskMgr_.MarkTaskDone("receiveccommonevent done");
                (void)spServer->Pause();
            }
        });
        taskMgr_.LaunchTask(pauseTask, PlayerServerTaskType::STATE_CHANGE, "receiveccommonevent");
    }
}

int32_t PlayerServer::GetUserId()
{
    return userId_.load();
}

std::shared_ptr<CommonEventReceiver> PlayerServer::GetCommonEventReceiver()
{
    return commonEventReceiver_;
}

bool PlayerServer::IsBootCompleted()
{
    return isBootCompleted_.load();
}
} // namespace Media
} // namespace OHOS
