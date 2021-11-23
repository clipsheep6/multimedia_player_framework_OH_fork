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

#include "demuxer_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
DemuxerImpl::DemuxerImpl()
{

}

DemuxerImpl::~DemuxerImpl()
{

}

int32_t DemuxerImpl::SetSource(const std::string &uri, TrackSelectMode mode)
{
    (void)uri;
    (void)mode;
    return MSERR_OK;
}

int32_t DemuxerImpl::SetSource(std::shared_ptr<IMediaDataSource> dataSource, TrackSelectMode mode)
{
    (void)dataSource;
    (void)mode;
    return MSERR_OK;
}

int32_t DemuxerImpl::GetContainerDescription(MediaDescription &desc)
{
    (void)desc;
    return MSERR_OK;
}

int32_t DemuxerImpl::GetTrackDescription(int32_t trackIdx, MediaDescription &desc)
{
    (void)trackIdx;
    (void)desc;
    return MSERR_OK;
}

int32_t DemuxerImpl::SelectTrack(int32_t trackIdx)
{
    (void)trackIdx;
    return MSERR_OK;
}

int32_t DemuxerImpl::UnSelectTrack(int32_t trackIdx)
{
    (void)trackIdx;
    return MSERR_OK;
}

int32_t DemuxerImpl::ReadTrackSample(std::shared_ptr<AVMemory> buffer, TrackSampleInfo &info)
{
    (void)buffer;
    (void)info;
    return MSERR_OK;
}

int32_t DemuxerImpl::Seek(int64_t timeUs, DemuxerSeekMode mode)
{
    (void)timeUs;
    (void)mode;
    return MSERR_OK;
}

int32_t DemuxerImpl::GetCacheState(int64_t &durationUs, bool &endOfStream)
{
    (void)durationUs;
    (void)endOfStream;
    return MSERR_OK;
}

void DemuxerImpl::Release()
{
}
}
}