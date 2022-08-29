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
const int32_t DELAY_TIME = 2000;
const std::string FREEZER_IMPL_NAME = "FreezerImpl";
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
    return DelayedSingleton<FreezerImpl>::GetInstance();
}

bool FreezerImpl::GetFreezerService()
{
    if (freezerService_) {
        MEDIA_LOGD("freezerService_ is already exist");
        return false;
    }
    freezerService_ = MediaServiceFactory::GetInstance().CreateFreezerService();
    if (!freezerService_) {
        return false;
    }

    if (!recipient_) {
        recipient_ = new (std::nothrow) FreezerImplDeathRecipient();
    }
    freezerServiceRunner_ = AppExecFwk::EventRunner::Create(FREEZER_IMPL_NAME);
    if (!freezerServiceRunner_) {
        MEDIA_LOGE("FreezerImpl runner create failed!");
        return false;
    }
    freezerServiceHandler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(freezerServiceRunner_);
    if (!freezerServiceHandler_) {
        MEDIA_LOGE("FreezerImpl handler create failed!");
        return false;
    }
    return true;
}

FreezerImpl::FreezerImpl()
{
    MEDIA_LOGD("FreezerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

FreezerImpl::~FreezerImpl()
{
    freezerService_ = nullptr;
    recipient_ = nullptr;
    MEDIA_LOGD("FreezerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t FreezerImpl::ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze)
{
    MEDIA_LOGW("KPI-TRACE: FreezerImpl ProxyApp in(pidSet, isFreeze)");
    if (!freezerService_ && !GetFreezerService()) {
        return MSERR_INVALID_OPERATION;
    }
    return freezerService_->ProxyApp(pidSet, isFreeze);
}

int32_t FreezerImpl::ResetAll()
{
    MEDIA_LOGW("KPI-TRACE: FreezerImpl ResetAll in()");
    if (!freezerService_ && !GetFreezerService()) {
        return MSERR_INVALID_OPERATION;
    }
    return freezerService_->ResetAll();
}

void FreezerImpl::FreezerImplDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    if (object == nullptr) {
        MEDIA_LOGE("remote object is null.");
        return;
    }
    DelayedSingleton<FreezerImpl>::GetInstance()->freezerService_ = nullptr;
    DelayedSingleton<FreezerImpl>::GetInstance()->freezerServiceHandler_->PostTask([this, &object]() {
            this->OnServiceDiedInner(object);
        },
        DELAY_TIME);
}

void FreezerImpl::FreezerImplDeathRecipient::OnServiceDiedInner(const wptr<IRemoteObject> &object)
{
    while (!DelayedSingleton<FreezerImpl>::GetInstance()->GetFreezerService()) { }
}
} // namespace Media
} // namespace OHOS