#ifndef AVMUXER_SERVICE_STUB_H
#define AVMUXER_SERVICE_STUB_H

#include "i_standard_avmuxer_service.h"
#include "avmuxer_server.h"

namespace OHOS {
namespace Media {
class AVMuxerServiceStub : public IRemoteStub<IStandardAVMuxerService> {
public:
    static sptr<AVMuxerServiceStub> Create();
    virtual ~AVMuxerServiceStub();
    DISALLOW_COPY_AND_MOVE(AVMuxerServiceStub);
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
    using AVMuxerStubFunc = int32_t(AVMuxerServiceStub::*)(MessageParcel& data, MessageParcel& reply);

    std::vector<std::string> GetMuxerFormatList() override;
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
    AVMuxerServiceStub();
    int32_t Init();
    int32_t GetMuxerFormatList(MessageParcel& data, MessageParcel& reply);
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
    std::shared_ptr<IAVMuxerService> avmuxerServer_ = nullptr;
    std::map<uint32_t, AVMuxerStubFunc> avmuxerFuncs_;
};
}  // Media
}  // OHOS
#endif