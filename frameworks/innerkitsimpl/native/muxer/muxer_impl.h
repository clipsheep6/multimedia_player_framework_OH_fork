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

#ifndef MUXER_IMPL_H
#define MUXER_IMPL_H

#include "muxer.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class MuxerImpl : public Muxer {
public:
    MuxerImpl();
    ~MuxerImpl();

    int32_t Init();
    int32_t SetOutput(const std::string &path, const std::string &format) override;
    int32_t SetLocation(float latitude, float longtitude) override;
    int32_t SetOrientationHint(int degrees) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackIdx) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo &info) override;
    int32_t Stop() override;
    void Release() override;

    DISALLOW_COPY_AND_MOVE(MuxerImpl);
};
}
}

#endif