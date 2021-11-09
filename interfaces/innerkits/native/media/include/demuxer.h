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

#ifndef DEMUXER_H
#define DEMUXER_H

#include <string>
#include "avmemory.h"
#include "media_types.h"
#include "media_data_source.h"
#include "media_descriptions.h"

namespace OHOS {
namespace Media {
enum TrackSelectMode : uint8_t {
    TRACK_TIME_SYNC,
    TRACK_TIME_INDEPENDENT,
};

enum DemuxerSeekMode : uint8_t {
    DEMUXER_SEEK_PREV_SYNC = 0,
    DEMUXER_SEEK_NEXT_SYNC = 1,
    DEMUXER_SEEK_CLOSEST_SYNC = 2,
};

class Demuxer {
public:
    virtual ~Demuxer() = default;

    virtual int32_t SetSource(const std::string &uri, TrackSelectMode mode) = 0;
    virtual int32_t SetSource(std::shared_ptr<IMediaDataSource> dataSource, TrackSelectMode mode) = 0;
    virtual void GetContainerDescriptions(MediaDescriptions &desc) = 0;
    virtual int32_t GetTrackDescriptions(int32_t trackId, MediaDescriptions &desc) = 0;
    virtual int32_t SelectTrack(int32_t trackId) = 0;
    virtual int32_t UnSelectTrack(int32_t trackId) = 0;
    virtual int32_t ReadTrackSample(std::shared_ptr<AVMemory> buffer, TrackSampleInfo &info) = 0;
    virtual int32_t Seek(int64_t timeUs, DemuxerSeekMode mode) = 0;
    virtual int32_t GetCacheState(int64_t &durationUs, bool &endOfStream) = 0;
    virtual void Release() = 0;
};

class __attribute__((visibility("default"))) DemuxerFactory {
public:
    static std::shared_ptr<Demuxer> CreateDemuxer();
private:
    DemuxerFactory() = default;
    ~DemuxerFactory() = default;
};
}
}

#endif
