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
    void ReadTrackInfoByteStream();
    void WriteTrackSampleByteStream();
    void AddTrackVideo();
    void DoNext();
    std::shared_ptr<AVMuxer> avmuxer_;
    int32_t width_ = 480;
    int32_t height_ = 640;
    int32_t frameRate_ = 30;
};
}  // namespace Media
}  // namespace OHOS
#endif  // AVMUXER_DEMO_H