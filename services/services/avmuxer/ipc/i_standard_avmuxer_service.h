#ifndef I_STANDARD_AVMUXER_SERVICE_H
#define I_STANDARD_AVMUXER_SERVICE_H

#include "i_avmuxer_service.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

namespace OHOS {
namespace Media {
class IStandardAVMuxerService : public IRemoteBroker {
public:
    virtual ~IStandardAVMuxerService() = default;
    virtual std::vector<std::string> GetAVMuxerFormatList() = 0;
    virtual int32_t SetOutput(const std::string& path, const std::string& format) = 0;
    virtual int32_t SetLocation(float latitude, float longitude) = 0;
    virtual int32_t SetOrientationHint(int degrees) = 0;
    virtual int32_t AddTrack(const MediaDescription& trackDesc, int32_t& trackId) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo& sampleInfo) = 0;
    virtual int32_t Stop() = 0;
    virtual void Release() = 0;
    virtual int32_t DestroyStub() = 0;

    enum AVMuxerServiceMsg {
        GET_MUXER_FORMAT_LIST = 0,
        SET_OUTPUT,
        SET_LOCATION,
        SET_ORIENTATION_HINT,
        ADD_TRACK,
        START,
        WRITE_TRACK_SAMPLE,
        STOP,
        RELEASE,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVMuxerServiceq1a");
};
}  // namespace Media
}  // namespace OHOS
#endif  // I_STANDARD_AVMUXER_SERVICE_H