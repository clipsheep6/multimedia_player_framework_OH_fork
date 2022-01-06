#ifndef AVMUXER_SERVER_H
#define AVMUXER_SERVER_H

#include <mutex>
#include "i_avmuxer_service.h"
#include "i_avmuxer_engine.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
enum AVMuxerStates : int32_t {
    AVMUXER_IDEL = 0,
    AVMUXER_OUTPUT_SET,
    AVMUXER_PARAMETER_SET,
    AVMUXER_STARTED,
    AVMUXER_SAMPLE_WRITING,
};
    
class AVMuxerServer : public IAVMuxerService {
public:
    static std::shared_ptr<IAVMuxerService> Create();
    AVMuxerServer();
    ~AVMuxerServer();
    DISALLOW_COPY_AND_MOVE(AVMuxerServer);

    std::vector<std::string> GetMuxerFormatList() override;
    int32_t SetOutput(const std::string& path, const std::string& format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetOrientationHint(int degrees) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
    void Release() override;
private:
    int32_t Init();
    std::mutex mutex_;
    std::shared_ptr<IAVMuxerEngine> avmuxerEngine_ = nullptr;
    AVMuxerStates curState_ = AVMUXER_IDEL;
    uint32_t trackNum_ = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif