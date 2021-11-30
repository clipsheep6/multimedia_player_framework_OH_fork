#ifndef MUXER_SERVICE_STUB_H
#define MUXER_SERVICE_STUB_H

#include "i_standard_muxer_service.h"
#include "muxer_server.h"

namespace OHOS {
namespace Media {
class MuxerServiceStub : public IRemoteStub<IStandardMuxerService> {
public:
    static sptr<MuxerServiceStub> Create();
    virtual ~MuxerServiceStub();
    DISALLOW_COPY_AND_MOVE(MuxerServiceStub);
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
    using MuxerStubFunc = int32_t(MuxerServiceStub::*)(MessageParcel& data, MessageParcel& reply);

    int32_t SetOutput(const std::string& path, const std::string& format) override;
    int32_t SetLocation(float latitude, float longitude) override;
	int32_t SetOrientationHint(int degrees) override;
    int32_t AddTrack(const MediaDescription& trackDesc, int32_t& trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo& sampleInfo) override;
    int32_t Stop() override;
    void Release() override;
    int32_t DestroyStub() override;
private:
    MuxerServiceStub();
    int32_t Init();
    std::vector<std::string> GetSupportedFormats(MessageParcel& data, MessageParcel& reply);
    int32_t SetOutput(MessageParcel& data, MessageParcel& reply);
    int32_t SetLocation(MessageParcel& data, MessageParcel& reply);
	int32_t SetOrientationHint(MessageParcel& data, MessageParcel& reply);
    int32_t AddTrack(MessageParcel& data, MessageParcel& reply);
    int32_t Start(MessageParcel& data, MessageParcel& reply);
    int32_t WriteTrackSample(MessageParcel& data, MessageParcel& reply);
    int32_t Stop(MessageParcel& data, MessageParcel& reply);
    int32_t Release(MessageParcel& data, MessageParcel& reply);
    int32_t DestroyStub(MessageParcel& data, MessageParcel& reply);

    std::mutex mutex_;
    std::shared_ptr<IMuxerService> muxerServer_ = nullptr;
    std::map<uint32_t, MuxerStubFunc> muxerFuncs_;
};
}  // Media
}  // OHOS
#endif