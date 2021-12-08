#include "muxer_service_stub.h"
#include "media_server_manager.h"
#include "media_errors.h"
#include "media_log.h"
#include "avsharedmemory_ipc.h"
#include "media_parcel.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerServiceStub"};
}

namespace OHOS {
namespace Media {
sptr<MuxerServiceStub> MuxerServiceStub::Create()
{
    sptr<MuxerServiceStub> muxerStub = new(std::nothrow) MuxerServiceStub();
    CHECK_AND_RETURN_RET_LOG(muxerStub != nullptr, nullptr, "Failed to create muxer service stub");

    int32_t ret = muxerStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Failed to init MuxerServiceStub");
    return muxerStub;
}

MuxerServiceStub::MuxerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerServiceStub::~MuxerServiceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MuxerServiceStub::Init()
{
    muxerServer_ = MuxerServer::Create();
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Failed to create muxer server");

    muxerFuncs_[SET_OUTPUT] = &MuxerServiceStub::SetOutput;
    muxerFuncs_[SET_LOCATION] = &MuxerServiceStub::SetLocation;
    muxerFuncs_[SET_ORIENTATION_HINT] = &MuxerServiceStub::SetOrientationHint;
    muxerFuncs_[ADD_TRACK] = &MuxerServiceStub::AddTrack;
    muxerFuncs_[START] = &MuxerServiceStub::Start;
    muxerFuncs_[WRITE_TRACK_SAMPLE] = &MuxerServiceStub::WriteTrackSample;
    muxerFuncs_[STOP] = &MuxerServiceStub::Stop;
    muxerFuncs_[RELEASE] = &MuxerServiceStub::Release;
    muxerFuncs_[DESTROY] = &MuxerServiceStub::DestroyStub;
    return MSERR_OK;
}

int32_t MuxerServiceStub::DestroyStub()
{
    muxerServer_ = nullptr;
    MediaServerManager::GetInstance().DestroyStubObject(MediaServerManager::MUXER, AsObject());
    return MSERR_OK;
}

int MuxerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option)
{
    MEDIA_LOGI("Stub: OnRemoteRequest of code: %{public}u is received", code);
    auto itFunc = muxerFuncs_.find(code);
    if (itFunc != muxerFuncs_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            int32_t ret = (this->*memberFunc)(data, reply);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call memberFunc");
            return MSERR_OK;
        }
    }
    MEDIA_LOGW("Failed to find corresponding function");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t MuxerServiceStub::SetOutput(const std::string& path, const std::string& format)
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->SetOutput(path, format);
}

int32_t MuxerServiceStub::SetLocation(float latitude, float longitude)
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->SetLocation(latitude, longitude);
}

int32_t MuxerServiceStub::SetOrientationHint(int degrees)
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->SetOrientationHint(degrees);
}

int32_t MuxerServiceStub::AddTrack(const MediaDescription& trackDesc, int32_t& trackId)
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->AddTrack(trackDesc, trackId);
}

int32_t MuxerServiceStub::Start()
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->Start();
}

int32_t MuxerServiceStub::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo& sampleInfo)
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->WriteTrackSample(sampleData, sampleInfo);
}

int32_t MuxerServiceStub::Stop()
{
    CHECK_AND_RETURN_RET_LOG(muxerServer_ != nullptr, MSERR_NO_MEMORY, "Muxer Service does not exist");
    return muxerServer_->Stop();
}

void MuxerServiceStub::Release()
{
    CHECK_AND_RETURN_LOG(muxerServer_ != nullptr, "Muxer Service does not exist");
    muxerServer_->Release();
}

int32_t MuxerServiceStub::SetOutput(MessageParcel& data, MessageParcel& reply)
{
    std::string path = data.ReadString();
    std::string format = data.ReadString();
    reply.WriteInt32(SetOutput(path, format));
    return MSERR_OK;
}

int32_t MuxerServiceStub::SetLocation(MessageParcel& data, MessageParcel& reply)
{
    float latitude = data.ReadFloat();
    float longitude = data.ReadFloat();
    reply.WriteInt32(SetLocation(latitude, longitude));
    return MSERR_OK;
}

int32_t MuxerServiceStub::SetOrientationHint(MessageParcel& data, MessageParcel& reply)
{
    int32_t degrees = data.ReadInt32();
    reply.WriteInt32(SetOrientationHint(degrees));
    return MSERR_OK;
}

int32_t MuxerServiceStub::AddTrack(MessageParcel& data, MessageParcel& reply)
{
    MediaDescription trackDesc;
    (void)MediaParcel::Unmarshalling(data, trackDesc);
    int32_t trackId;
    int32_t ret = AddTrack(trackDesc, trackId);
    reply.WriteInt32(trackId);
    reply.WriteInt32(ret);
    return MSERR_OK;
}

int32_t MuxerServiceStub::Start(MessageParcel& data, MessageParcel& reply)
{
    reply.WriteInt32(Start());
    return MSERR_OK;
}

int32_t MuxerServiceStub::WriteTrackSample(MessageParcel& data, MessageParcel& reply)
{
    std::shared_ptr<AVSharedMemory> sampleData = ReadAVSharedMemoryFromParcel(data);
    TrackSampleInfo sampleInfo = {{data.ReadInt64(), data.ReadInt32(), data.ReadInt32(),
        static_cast<FrameFlags>(data.ReadInt32())}, data.ReadInt32()};
    reply.WriteInt32(WriteTrackSample(sampleData, sampleInfo));
    return MSERR_OK;
}

int32_t MuxerServiceStub::Stop(MessageParcel& data, MessageParcel& reply)
{
    reply.WriteInt32(Stop());
    return MSERR_OK;
}

int32_t MuxerServiceStub::Release(MessageParcel& data, MessageParcel& reply)
{
    Release();
    return MSERR_OK;
}

int32_t MuxerServiceStub::DestroyStub(MessageParcel& data, MessageParcel& reply)
{
    reply.WriteInt32(DestroyStub());
    return MSERR_OK;
}
}  // namespace Media
}  // namespace OHOS