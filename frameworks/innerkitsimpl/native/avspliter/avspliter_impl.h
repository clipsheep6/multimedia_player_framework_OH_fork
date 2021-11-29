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

#ifndef AVSPLITER_IMPL_H
#define AVSPLITER_IMPL_H

#include "avspliter.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVSpliterImpl : public AVSpliter {
public:
    AVSpliterImpl();
    ~AVSpliterImpl();

    int32_t Init();
    int32_t SetSource(const std::string &uri, TrackSelectMode mode) override;
    int32_t SetSource(std::shared_ptr<IMediaDataSource> dataSource, TrackSelectMode mode) override;
    int32_t GetContainerDescription(MediaDescription &desc) override;
    int32_t GetTrackDescription(int32_t trackIdx, MediaDescription &desc) override;
    int32_t SelectTrack(int32_t trackIdx) override;
    int32_t UnSelectTrack(int32_t trackIdx) override;
    int32_t ReadTrackSample(std::shared_ptr<AVMemory> buffer, TrackSampleInfo &info) override;
    int32_t Seek(int64_t timeUs, AVSpliterSeekMode mode) override;
    int32_t GetCacheState(int64_t &durationUs, bool &endOfStream) override;
    void Release() override;

    DISALLOW_COPY_AND_MOVE(AVSpliterImpl);
};
}
}

#endif
