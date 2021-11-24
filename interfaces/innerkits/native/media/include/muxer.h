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

#ifndef MUXER_H
#define MUXER_H

#include <string>
#include <memory>
#include <vector>
#include "avmemory.h"
#include "media_types.h"
#include "media_descriptions.h"

namespace OHOS {
namespace Media {
class Muxer {
public:
    virtual ~Muxer() = default;

    static std::vector<std::string> GetSupportedFormats();

    virtual int32_t SetOutput(const std::string &path, const std::string &format);
    virtual int32_t SetLocation(float latitude, float longtitude) = 0;
    virtual int32_t SetOrientationHint(int degrees) = 0;
    virtual int32_t AddTrack(const MediaDescriptions &trackDesc, int32_t &trackId) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo &info) = 0;
    virtual int32_t Stop() = 0;
    virtual void Release() = 0;
};

class __attribute__((visibility("default"))) MuxerFactory {
public:
    static std::shared_ptr<Muxer> CreateMuxer();
private:
    MuxerFactory() = default;
    ~MuxerFactory() = default;
};
}
}
#endif
