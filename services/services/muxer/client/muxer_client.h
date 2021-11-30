#ifndef MUXER_CLIENT_H
#define MUXER_CLIENT_H

#include "i_muxer_service.h"
#include "i_standard_muxer_service.h"

namespace OHOS {
namespace Media {
class MuxerClient : public IMuxerService {
public:
    static std::shared_ptr<MuxerClient> Create(const sptr<IStandardMuxerService>& ipcProxy);
    explicit MuxerClient(const sptr<IStandardMuxerService>& ipcProxy);
    ~MuxerClient();
    DISALLOW_COPY_AND_MOVE(MuxerClient);

    int32_t SetOutput(const std::string& path, const std::string& format) override;
    int32_t SetLocation(float latitude, float longitude) override;
	int32_t SetOrientationHint(int degrees) override;
    int32_t AddTrack(const MediaDescription& trackDesc, int32_t& trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
    void Release() override;

    void MediaServerDied();
private:
    std::mutex mutex_;
    sptr<IStandardMuxerService> muxerProxy_ = nullptr;
};
}  // namespace Media
}  // namespace OHOS
#endif  // MUXER_CLIENT_H