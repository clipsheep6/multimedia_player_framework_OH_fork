#ifndef I_STANDARD_MUXER_SERVICE_H
#define I_STANDARD_MUXER_SERVICE_H

#include "i_muxer_service.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

namespace OHOS {
namespace Media {
class IStandardMuxerService : public IRemoteBroker {
public:
    virtual ~IStandardMuxerService() = default;
    static std::vector<std::string> GetSupportedFormats();
    virtual int32_t SetOutput(const std::string& path, const std::string& format) = 0;
    virtual int32_t SetLocation(float latitude, float longitude) = 0;
	virtual int32_t SetOrientationHint(int degrees) = 0;
    virtual int32_t AddTrack(const MediaDescription& trackDesc, int32_t& trackId) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo& sampleInfo) = 0;
    virtual int32_t Stop() = 0;
    virtual void Release() = 0;
    virtual int32_t DestroyStub() = 0;

    enum MuxerServiceMsg {
        SET_OUTPUT = 0,
        SET_LOCATION,
        SET_ORIENTATION_HINT,
        ADD_TRACK,
        START,
        WRITE_TRACK_SAMPLE,
        STOP,
        RELEASE,
        DESTROY,
    };

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardMuxerServiceq1a");
};
}  // namespace Media
}  // namespace OHOS
#endif  // I_STANDARD_MUXER_SERVICE_H