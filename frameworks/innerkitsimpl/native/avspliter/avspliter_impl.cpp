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

#include "avspliter_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVSpliterImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVSpliter> AVSpliterFactory::CreateAVSpliter()
{
    auto avspliter = std::make_shared<AVSpliterImpl>();
    int32_t ret = avspliter->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "avspliter init failed");
    return avspliter;
}

AVSpliterImpl::AVSpliterImpl()
{

}

AVSpliterImpl::~AVSpliterImpl()
{

}

int32_t AVSpliterImpl::Init()
{
    return MSERR_OK;
}

int32_t AVSpliterImpl::SetSource(const std::string &uri, TrackSelectMode mode)
{
    (void)uri;
    (void)mode;
    return MSERR_OK;
}

int32_t AVSpliterImpl::SetSource(std::shared_ptr<IMediaDataSource> dataSource, TrackSelectMode mode)
{
    (void)dataSource;
    (void)mode;
    return MSERR_OK;
}

int32_t AVSpliterImpl::GetContainerDescription(MediaDescription &desc)
{
    (void)desc;
    return MSERR_OK;
}

int32_t AVSpliterImpl::GetTrackDescription(int32_t trackIdx, MediaDescription &desc)
{
    (void)trackIdx;
    (void)desc;
    return MSERR_OK;
}

int32_t AVSpliterImpl::SelectTrack(int32_t trackIdx)
{
    (void)trackIdx;
    return MSERR_OK;
}

int32_t AVSpliterImpl::UnSelectTrack(int32_t trackIdx)
{
    (void)trackIdx;
    return MSERR_OK;
}

int32_t AVSpliterImpl::ReadTrackSample(std::shared_ptr<AVMemory> buffer, TrackSampleInfo &info)
{
    (void)buffer;
    (void)info;
    return MSERR_OK;
}

int32_t AVSpliterImpl::Seek(int64_t timeUs, AVSpliterSeekMode mode)
{
    (void)timeUs;
    (void)mode;
    return MSERR_OK;
}

int32_t AVSpliterImpl::GetCacheState(int64_t &durationUs, bool &endOfStream)
{
    (void)durationUs;
    (void)endOfStream;
    return MSERR_OK;
}

void AVSpliterImpl::Release()
{
}
}
}