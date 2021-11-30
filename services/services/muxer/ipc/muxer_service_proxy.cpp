#include "muxer_service_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"
#include "media_parcel.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerServiceProxy"};
}

namespace OHOS {
namespace Media {
MuxerServiceProxy::MuxerServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardMuxerService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerServiceProxy::~MuxerServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MuxerServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(DESTROY, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call DestroyStub, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::SetOutput(const std::string& path, const std::string& format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteString(path);
    (void)data.WriteString(format);
    int error = Remote()->SendRequest(SET_OUTPUT, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call SetOutput, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::SetLocation(float latitude, float longitude)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteFloat(latitude);
    (void)data.WriteFloat(longitude);
    int error = Remote()->SendRequest(SET_LOCATION, data, reply, option); 
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call SetLocation, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::SetOrientationHint(int degrees)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteInt32(degrees);
    int error = Remote()->SendRequest(SET_ORIENTATION_HINT, data, reply, option); 
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call SetOrientationHint, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::AddTrack(const MediaDescription& trackDesc, int32_t& trackId)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)MediaParcel::Marshalling(data, trackDesc);
    int error = Remote()->SendRequest(ADD_TRACK, data, reply, option); 
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call AddTrack, error: %{public}d", error);
    trackId = reply.ReadInt32();
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(START, data, reply, option); 
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call Start, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo& sampleInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    AVShMemIPCStatic::WriteToParcel(sampleData, data);
    (void)data.WriteInt64(sampleInfo.timeUs);
    (void)data.WriteInt32(sampleInfo.size);
    (void)data.WriteInt32(sampleInfo.offset);
    (void)data.WriteInt32(sampleInfo.flags);
    (void)data.WriteInt32(sampleInfo.trackIdx);
    int error = Remote()->SendRequest(WRITE_TRACK_SAMPLE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call WriteTrackSample, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t MuxerServiceProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(STOP, data, reply, option); 
    CHECK_AND_RETURN_RET_LOG(error == MSERR_OK, error, "Failed to call Stop, error: %{public}d", error);
    return reply.ReadInt32();
}

void MuxerServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RELEASE, data, reply, option); 
    CHECK_AND_RETURN_LOG(error == MSERR_OK, "Failed to call Release, error: %{public}d", error);
}
}  // namespace Media
}  // namespace OHOS