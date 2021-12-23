#ifndef AVMUXER_CLIENT_H
#define AVMUXER_CLIENT_H

#include "i_avmuxer_service.h"
#include "i_standard_avmuxer_service.h"

namespace OHOS {
namespace Media {
class AVMuxerClient : public IAVMuxerService {
public:
    static std::shared_ptr<AVMuxerClient> Create(const sptr<IStandardAVMuxerService>& ipcProxy);
    explicit AVMuxerClient(const sptr<IStandardAVMuxerService>& ipcProxy);
    ~AVMuxerClient();
    DISALLOW_COPY_AND_MOVE(AVMuxerClient);

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
    sptr<IStandardAVMuxerService> avmuxerProxy_ = nullptr;
    // FILE *fp;
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_CLIENT_H