#include "muxer_server.h"
#include "media_errors.h"
#include "media_log.h"
#include "engine_factory_repo.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IMuxerService> MuxerServer::Create()
{
    std::shared_ptr<MuxerServer> muxerServer = std::make_shared<MuxerServer>();
    CHECK_AND_RETURN_RET_LOG(muxerServer != nullptr, nullptr, "Muxer Service does not exist");
    int32_t ret = muxerServer->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init muxer server");
    return muxerServer;
}

MuxerServer::MuxerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerServer::~MuxerServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    std::lock_guard<std::mutex> lock(mutex_);
    muxerEngine_ = nullptr;
}

int32_t MuxerServer::Init()
{
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_MUXER);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_MUXER_ENGINE_FAILED, "Failed to get engine factory");
    muxerEngine_ = engineFactory->CreateMuxerEngine();
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_NO_MEMORY, "Failed to create muxer engine");
    int32_t ret = muxerEngine_->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to init muxer engine");

    fd_ = fopen("frame.txt", "w");
    CHECK_AND_RETURN_RET_LOG(fd_ != nullptr, ret, "Failed to open file");
    return MSERR_OK;
}

int32_t MuxerServer::SetOutput(const std::string& path, const std::string& format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_IDEL && curState_ != MUXER_OUTPUT_SET) {
       MEDIA_LOGE("Failed to call SetOutput, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    MEDIA_LOGD("Current output path is: %{public}s, and the format is: %{public}s", path.c_str(), format.c_str());
    
    int32_t ret = muxerEngine_->SetOutput(path, format);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetOutput");
    curState_ = MUXER_OUTPUT_SET;
    return MSERR_OK;
}

int32_t MuxerServer::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_OUTPUT_SET && curState_ != MUXER_PARAMETER_SET) {
       MEDIA_LOGE("Failed to call SetLocation, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "Muxer engine does not exist");
    int32_t ret = muxerEngine_->SetLocation(latitude, longitude);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetLocation");
    curState_ = MUXER_PARAMETER_SET;
    return MSERR_OK;
}

int32_t MuxerServer::SetOrientationHint(int degrees)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_OUTPUT_SET && curState_ != MUXER_PARAMETER_SET) {
       MEDIA_LOGE("Failed to call SetOrientationHint, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "Muxer engine does not exist");
    int32_t ret = muxerEngine_->SetOrientationHint(degrees);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetOrientationHint");
    curState_ = MUXER_PARAMETER_SET;
    return MSERR_OK;
}

int32_t MuxerServer::AddTrack(const Format& trackFormat, int32_t &trackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_OUTPUT_SET && curState_ != MUXER_PARAMETER_SET) {
       MEDIA_LOGE("Failed to call AddTrack, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "Muxer engine does not exist");
    int32_t ret = muxerEngine_->AddTrack(trackFormat, trackId);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call AddTrack");
    curState_ = MUXER_PARAMETER_SET;
    trackNum_ += 1;
    return MSERR_OK;
}

int32_t MuxerServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_PARAMETER_SET || trackNum_ == 0) {
       MEDIA_LOGE("Failed to call Start, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "Muxer engine does not exist");
    int32_t ret = muxerEngine_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Start");
    curState_ = MUXER_STARTED;
    return MSERR_OK;
}

int32_t MuxerServer::WriteTrackSample(std::shared_ptr<AVSharedMemory> data, const TrackSampleInfo& info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_STARTED && curState_ != MUXER_SAMPLE_WRITING) {
       MEDIA_LOGE("Failed to call Start, currentState is %{public}d", curState_);
       return MSERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "Muxer engine does not exist");
    int32_t ret = muxerEngine_->WriteTrackSample(data, info);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteTrackSample");
    fwrite(data->GetBase(), data->GetSize(), 1, fd_);
    curState_ = MUXER_SAMPLE_WRITING;
    return MSERR_OK;
}

int32_t MuxerServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerEngine_ != nullptr, MSERR_INVALID_OPERATION, "Muxer engine does not exist");
    int32_t ret = muxerEngine_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Stop");
    curState_ = MUXER_IDEL;
    return MSERR_OK;
}

void MuxerServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (curState_ != MUXER_IDEL) {
        Stop();
    }
    fclose(fd_);
    muxerEngine_ = nullptr;
}
}  // namespace Media
}  // namespace OHOS