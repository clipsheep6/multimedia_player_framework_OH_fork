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
    void ReadTrackInfo();
    void WriteTrackSample();
    void DoNext();
    std::shared_ptr<Muxer> muxer_;
    const char* url_ = "/data/media/test.mp4";
    AVFormatContext* fmt_ctx_ = nullptr;
    int32_t width_;
    int32_t height_;
    int32_t index_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // MUXER_DEMO_H