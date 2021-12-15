#include "avmuxer_server.h"
#include "media_errors.h"
#include "media_log.h"
#include "engine_factory_repo.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IAVMuxerService> AVMuxerServer::Create()
{
    std::shared_ptr<AVMuxerServer> avmuxerServer = std::make_shared<AVMuxerServer>();
    CHECK_AND_RETURN_RET_LOG(avmuxerServer != nullptr, nullptr, "AVMuxer Service does not exist");
    int32_t ret = avmuxerServer->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init avmuxer server");
    return avmuxerServer;
}

AVMuxerServer::AVMuxerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerServer::~AVMuxerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(mutex_);
    avmuxerEngine_ = nullptr;
}

int32_t AVMuxerServer::Init()
{
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_AVMUXER);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_AVMUXER_ENGINE_FAILED, "Failed to get engine factory");
    avmuxerEngine_ = engineFactory->CreateAVMuxerEngine();
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_NO_MEMORY, "Failed to create avmuxer engine");
    int32_t ret = avmuxerEngine_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to init avmuxer engine");

    return MSERR_OK;
}

int32_t AVMuxerServer::SetOutput(const std::string& path, const std::string& format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_IDEL && curState_ != AVMUXER_OUTPUT_SET) {
       MEDIA_LOGE("Failed to call SetOutput, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("Current output path is: %{public}s, and the format is: %{public}s", path.c_str(), format.c_str());
    
    int32_t ret = avmuxerEngine_->SetOutput(path, format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetOutput");
    curState_ = AVMUXER_OUTPUT_SET;
    return MSERR_OK;
}

int32_t AVMuxerServer::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_OUTPUT_SET && curState_ != AVMUXER_PARAMETER_SET) {
       MEDIA_LOGE("Failed to call SetLocation, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->SetLocation(latitude, longitude);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetLocation");
    curState_ = AVMUXER_PARAMETER_SET;
    return MSERR_OK;
}

int32_t AVMuxerServer::SetOrientationHint(int degrees)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_OUTPUT_SET && curState_ != AVMUXER_PARAMETER_SET) {
       MEDIA_LOGE("Failed to call SetOrientationHint, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->SetOrientationHint(degrees);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetOrientationHint");
    curState_ = AVMUXER_PARAMETER_SET;
    return MSERR_OK;
}

int32_t AVMuxerServer::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_OUTPUT_SET && curState_ != AVMUXER_PARAMETER_SET) {
       MEDIA_LOGE("Failed to call AddTrack, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->AddTrack(trackDesc, trackId);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call AddTrack");
    curState_ = AVMUXER_PARAMETER_SET;
    trackNum_ += 1;
    return MSERR_OK;
}

int32_t AVMuxerServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_PARAMETER_SET || trackNum_ == 0) {
       MEDIA_LOGE("Failed to call Start, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Start");
    curState_ = AVMUXER_STARTED;
    return MSERR_OK;
}

int32_t AVMuxerServer::WriteTrackSample(std::shared_ptr<AVSharedMemory> data, const TrackSampleInfo& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_STARTED && curState_ != AVMUXER_SAMPLE_WRITING) {
       MEDIA_LOGE("Failed to call Start, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->WriteTrackSample(data, info);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteTrackSample");
    curState_ = AVMUXER_SAMPLE_WRITING;
    return MSERR_OK;
}

int32_t AVMuxerServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(avmuxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "AVMuxer engine does not exist");
    int32_t ret = avmuxerEngine_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Stop");
    curState_ = AVMUXER_IDEL;
    return MSERR_OK;
}

void AVMuxerServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != AVMUXER_IDEL) {
        Stop();
    }
    avmuxerEngine_ = nullptr;
}
}  // namespace Media
}  // namespace OHOS