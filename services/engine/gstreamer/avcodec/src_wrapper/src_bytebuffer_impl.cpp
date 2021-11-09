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

#include "src_bytebuffer_impl.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "SrcBytebufferImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVSharedMemory> SrcBytebufferImpl::GetInputBuffer(uint32_t index)
{
    MEDIA_LOGD("GetInputBuffer");
    return nullptr;
}

int32_t SrcBytebufferImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info,
                                           AVCodecBufferType type, AVCodecBufferFlag flag)
{
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetParameter(const Format &format)
{
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCallback(const std::shared_ptr<SrcCallback> &callback)
{
    return MSERR_OK;
}

int32_t SrcBytebufferImpl::SetCaps(GstCaps *caps)
{
    return MSERR_OK;
}
} // Media
} // OHOS
