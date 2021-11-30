#ifndef MUXER_SERVICE_PROXY_H
#define MUXER_SERVICE_PROXY_H

#include "i_standard_muxer_service.h"

namespace OHOS {
namespace Media {
class MuxerServiceProxy : public IRemoteProxy<IStandardMuxerService> {
public:
    explicit MuxerServiceProxy(const sptr<IRemoteObject>& impl);
    virtual ~MuxerServiceProxy();
    DISALLOW_COPY_AND_MOVE(MuxerServiceProxy);

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
    static inline BrokerDelegator<MuxerServiceProxy> delegator_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // MUXER_SERVICE_PROXY_H