/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "freezer_impl.h"

#include "freezer_client.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "FreezerImpl"};
}

namespace OHOS {
namespace Media {
const std::map<FreezerErrorType, std::string> FREEZER_ERRTYPE_INFOS = {
    {FREEZER_ERROR, "internal freezer error"},
    {FREEZER_ERROR_UNKNOWN, "unknown freezer error"},
    {FREEZER_ERROR_EXTEND_START, "freezer extend start error type"},
};

std::string FreezerErrorTypeToString(FreezerErrorType type)
{
    if (FREEZER_ERRTYPE_INFOS.count(type) != 0) {
        return FREEZER_ERRTYPE_INFOS.at(type);
    }

    if (type > FREEZER_ERROR_EXTEND_START) {
        return "extend error type:" + std::to_string(static_cast<int32_t>(type - FREEZER_ERROR_EXTEND_START));
    }

    return "invalid error type:" + std::to_string(static_cast<int32_t>(type));
}

std::shared_ptr<Freezer> FreezerFactory::CreateFreezer()
{
    std::shared_ptr<FreezerImpl> impl = std::make_shared<FreezerImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new FreezerImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init FreezerImpl");

    return impl;
}

int32_t FreezerImpl::Init()
{
    freezerService_ = MediaServiceFactory::GetInstance().CreateFreezerService();
    CHECK_AND_RETURN_RET_LOG(freezerService_ != nullptr, MSERR_UNKNOWN, "failed to create player service");
    return MSERR_OK;
}

FreezerImpl::FreezerImpl()
{
    MEDIA_LOGD("FreezerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

FreezerImpl::~FreezerImpl()
{
    freezerService_ = nullptr;
    MEDIA_LOGD("FreezerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t FreezerImpl::ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze)
{
    MEDIA_LOGW("KPI-TRACE: FreezerImpl ProxyApp in(pidSet, isFreeze)");
    return freezerService_->ProxyApp(pidSet, isFreeze);
}

int32_t FreezerImpl::ResetAll()
{
    MEDIA_LOGW("KPI-TRACE: FreezerImpl ResetAll in()");
    return freezerService_->ResetAll();
}
} // namespace Media
} // namespace OHOS