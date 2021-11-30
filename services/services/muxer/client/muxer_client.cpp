#include "muxer_client.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerClient"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<MuxerClient> MuxerClient::Create(const sptr<IStandardMuxerService>& ipcProxy)
{
    std::shared_ptr<MuxerClient> muxerClient = std::make_shared<MuxerClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(muxerClient != nullptr, nullptr, "Failed to create muxer client");
    return muxerClient;
}

MuxerClient::MuxerClient(const sptr<IStandardMuxerService>& ipcProxy)
    : muxerProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerClient::~MuxerClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (muxerProxy_ != nullptr) {
        (void)muxerProxy_->DestroyStub();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MuxerClient::MediaServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    muxerProxy_ = nullptr;
}

int32_t MuxerClient::SetOutput(const std::string& path, const std::string& format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->SetOutput(path, format);
}

int32_t MuxerClient::SetLocation(float latitude, float longitude)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->SetLocation(latitude, longitude);
}

int32_t MuxerClient::SetOrientationHint(int degrees)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->SetOrientationHint(degrees);
}

int32_t MuxerClient::AddTrack(const MediaDescription& trackDesc, int32_t& trackId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->AddTrack(trackDesc, trackId);
}

int32_t MuxerClient::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->Start();
}

int32_t MuxerClient::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->WriteTrackSample(sampleData, sampleInfo);
}

int32_t MuxerClient::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxerProxy_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerProxy_->Stop();
}

void MuxerClient::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(muxerProxy_ != nullptr, "Muxer Service does not exist");
    muxerProxy_->Release();
}
}  // namespace Media
}  // namespace OHOS