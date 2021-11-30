#ifndef MUXER_DEMO_H
#define MUXER_DEMO_H

extern "C" {
    #include "libavformat/avformat.h"
    #include "libavutil/avutil.h"
}
#include "muxer.h"

namespace OHOS {
namespace Media {
class MuxerDemo {
public:
    MuxerDemo() = default;
    ~MuxerDemo() = default;
    DISALLOW_COPY_AND_MOVE(MuxerDemo);
    void RunCase();
private:
    void WriteTrackSample();
    void DoNext();
    std::shared_ptr<Muxer> muxer_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // MUXER_DEMO_H