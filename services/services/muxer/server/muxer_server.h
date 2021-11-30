#ifndef MUXER_SERVER_H
#define MUXER_SERVER_H

#include <mutex>
#include "i_muxer_service.h"
#include "i_muxer_engine.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
enum MuxerStates : int32_t {
    MUXER_IDEL = 0,
    MUXER_OUTPUT_SET,
    MUXER_PARAMETER_SET,
    MUXER_STARTED,
    MUXER_SAMPLE_WRITING,
};
    
class MuxerServer : public IMuxerService {
public:
    static std::shared_ptr<IMuxerService> Create();
    MuxerServer();
    ~MuxerServer();
    DISALLOW_COPY_AND_MOVE(MuxerServer);

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
    std::shared_ptr<IMuxerEngine> muxerEngine_ = nullptr;
    MuxerStates curState_ = MUXER_IDEL;
    uint32_t trackNum_ = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif