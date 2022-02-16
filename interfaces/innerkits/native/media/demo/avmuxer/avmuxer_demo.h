/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    void ReadTrackInfoAVC();
    void ReadTrackInfoByteStream();
    void WriteTrackSampleAVC();
    void WriteTrackSampleByteStream();
    void AddTrackVideo();
    void AddTrackAudio();
    void DoNext();
    std::shared_ptr<AVMuxer> avmuxer_;
    std::string url_ = "/data/media/test.mp4";
    AVFormatContext *fmt_ctx_ = nullptr;
    int32_t width_;
    int32_t height_;
    int32_t frameRate_;
    int32_t channels_;
    int32_t sampleRate_;
    int32_t videoIndex_;
    int32_t audioIndex_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_DEMO_H