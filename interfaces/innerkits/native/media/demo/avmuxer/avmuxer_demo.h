#ifndef AVMUXER_DEMO_H
#define AVMUXER_DEMO_H

extern "C" {
    #include "libavformat/avformat.h"
    #include "libavutil/avutil.h"
}
#include "avmuxer.h"

namespace OHOS {
namespace Media {
class AVMuxerDemo {
public:
    AVMuxerDemo() = default;
    ~AVMuxerDemo() = default;
    DISALLOW_COPY_AND_MOVE(AVMuxerDemo);
    void RunCase();
private:
    void ReadTrackInfo();
    void WriteTrackSample();
    void DoNext();
    std::shared_ptr<AVMuxer> avmuxer_;
    const char* url_ = "/data/media/test.mp4";
    AVFormatContext* fmt_ctx_ = nullptr;
    int32_t width_;
    int32_t height_;
    int32_t index_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_DEMO_H