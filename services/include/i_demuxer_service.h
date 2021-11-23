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

#ifndef I_DEMUXER_SERVICE_H
#define I_DEMUXER_SERVICE_H

#include <string>
#include <memory>
#include <vector>
#include "demuxer.h"
#include "avsharedmemory.h"
#include "media_types.h"
#include "media_description.h"
#include "media_data_source.h"

namespace OHOS {
namespace Media {
struct ReadSampleOption {
    int64_t startTimeUs;
    int32_t sampleCount;
    int64_t endTimeUs;
    DemuxerSeekMode seekMode;
};

class IDemuxerService {
public:
    virtual ~IDemuxerService() = default;

    virtual int32_t SetSource(const std::string &uri) = 0;
    virtual int32_t SetSource(std::shared_ptr<IMediaDataSource> dataSource) = 0;
    virtual int32_t GetContainerDescription(MediaDescription &desc) = 0;
    virtual int32_t GetTrackDescription(int32_t trackIdx, MediaDescription &desc) = 0;
    virtual int32_t SelectTrack(int32_t trackIdx) = 0;
    virtual int32_t UnSelectTrack(int32_t trackIdx) = 0;
    virtual int32_t ReadTrackSample(const ReadSampleOption &option,
        std::vector<std::shared_ptr<AVSharedMemory>> &sampleDatas, std::vector<TrackSampleInfo> &sampleInfos) = 0;
    virtual int32_t GetCacheState(int64_t &durationUs, bool &endOfStream) = 0;
    virtual void Release() = 0;
};
}
}

#endif