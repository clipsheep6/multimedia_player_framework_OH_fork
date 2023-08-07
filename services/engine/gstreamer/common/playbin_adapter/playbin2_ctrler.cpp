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

#include "playbin2_ctrler.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBin2Ctrler"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IPlayBinCtrler> IPlayBinCtrler::Create(
    PlayBinKind kind, const PlayBinCreateParam &createParam)
{
    CHECK_AND_RETURN_RET_LOG(kind == PlayBinKind::PLAYBIN2, nullptr, "kind is't PLAYBIN2");

    auto inst = std::make_shared<PlayBin2Ctrler>(createParam);
    int32_t ret = inst->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "create playbin2ctrler failed");
    return inst;
}

PlayBin2Ctrler::~PlayBin2Ctrler()
{
    MEDIA_LOGD("enter dtor");
}

int32_t PlayBin2Ctrler::OnInit()
{
    playbin_ = reinterpret_cast<GstPipeline *>(gst_element_factory_make("playbin", "playbin"));
    CHECK_AND_RETURN_RET_LOG(playbin_ != nullptr, MSERR_UNKNOWN, "create playbin failed");
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS