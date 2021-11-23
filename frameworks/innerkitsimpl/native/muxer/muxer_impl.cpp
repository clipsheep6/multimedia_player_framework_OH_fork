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

#include "muxer_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace OHOS {
namespace Media {
MuxerImpl::MuxerImpl()
{

}

MuxerImpl::~MuxerImpl()
{

}

int32_t MuxerImpl::SetOutput(const std::string &path, const std::string &format)
{
    (void)path;
    (void)format;
    return MSERR_OK;
}

int32_t MuxerImpl::SetLocation(float latitude, float longtitude)
{
    (void)latitude;
    (void)longtitude;
    return MSERR_OK;
}

int32_t MuxerImpl::SetOrientationHint(int degrees)
{
    (void)degrees;
    return MSERR_OK;
}

int32_t MuxerImpl::AddTrack(const MediaDescription &trackDesc, int32_t &trackIdx)
{
    (void)trackDesc;
    (void)trackIdx;
    return MSERR_OK;
}

int32_t MuxerImpl::Start()
{
    return MSERR_OK;
}

int32_t MuxerImpl::WriteTrackSample(std::shared_ptr<AVMemory> sampleData, const TrackSampleInfo &info)
{
    (void)sampleData;
    (void)info;
    return MSERR_OK;
}

int32_t MuxerImpl::Stop()
{
    return MSERR_OK;
}

void MuxerImpl::Release()
{
}

}
}