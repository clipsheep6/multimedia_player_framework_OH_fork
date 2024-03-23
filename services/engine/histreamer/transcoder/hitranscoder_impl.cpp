/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy  of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "hitranscoder_impl.h"
#include "sync_fence.h"
#include <sys/syscall.h>

namespace OHOS {
namespace Media {
class TransCoderEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TransCoderEventReceiver(HiTransCoderImpl *hiTransCoderImpl)
    {
        hiTransCoderImpl_ = hiTransCoderImpl;
    }

    void OnEvent(const Event &event)
    {
        hiTransCoderImpl_->OnEvent(event);
    }

private:
    HiTransCoderImpl *hiTransCoderImpl_;
};

class TransCoderFilterCallback : public Pipeline::FilterCallback {
public:
    explicit TransCoderFilterCallback(HiTransCoderImpl *hiTransCoderImpl)
    {
        hiTransCoderImpl_ = hiTransCoderImpl;
    }

    void OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd,
        Pipeline::StreamType outType)
    {
        hiTransCoderImpl_->OnCallback(filter, cmd, outType);
    }

private:
    HiTransCoderImpl *hiTransCoderImpl_;
};

HiTransCoderImpl::HiTransCoderImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId, uint64_t appFullTokenId)
    : appUid_(appUid), appPid_(appPid), appTokenId_(appTokenId), appFullTokenId_(appFullTokenId)
{
    pipeline_ = std::make_shared<Pipeline::Pipeline>();
    (void)appUid_;
    (void)appPid_;
    (void)appTokenId_;
    (void)appFullTokenId_;
}

HiTransCoderImpl::~HiTransCoderImpl()
{
}

int32_t HiTransCoderImpl::Init()
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::SetOutputFormat(OutputFormatType format)
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::SetObs(const std::weak_ptr<ITransCoderEngineObs> &obs)
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::Configure(const TransCoderParam &transCoderParam)
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::Prepare()
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::Start()
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::Pause()
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::Resume()
{
    return (int32_t)Status::OK;
}

int32_t HiTransCoderImpl::Cancel()
{
    return (int32_t)Status::OK;
}

void HiTransCoderImpl::OnEvent(const Event &event)
{
    switch (event.type) {
        case EventType::EVENT_ERROR: {
            break;
        }
        default:
            break;
    }
}

void HiTransCoderImpl::OnCallback(std::shared_ptr<Pipeline::Filter> filter, const Pipeline::FilterCallBackCommand cmd,
    Pipeline::StreamType outType)
{
}
} // namespace MEDIA
} // namespace OHOS
