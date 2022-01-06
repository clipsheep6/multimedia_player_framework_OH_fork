#ifndef AVMUXER_SERVICE_PROXY_H
#define AVMUXER_SERVICE_PROXY_H

#include "i_standard_avmuxer_service.h"

namespace OHOS {
namespace Media {
class AVMuxerServiceProxy : public IRemoteProxy<IStandardAVMuxerService> {
public:
    explicit AVMuxerServiceProxy(const sptr<IRemoteObject>& impl);
    virtual ~AVMuxerServiceProxy();
    DISALLOW_COPY_AND_MOVE(AVMuxerServiceProxy);

    std::vector<std::string> GetAVMuxerFormatList() override;
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
    static inline BrokerDelegator<AVMuxerServiceProxy> delegator_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_SERVICE_PROXY_H