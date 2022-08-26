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

#ifndef FREEZER_IMPL_H
#define FREEZER_IMPL_H

#include "singleton.h"
#include "freezer.h"
#include "nocopyable.h"
#include "i_freezer_service.h"

namespace OHOS {
namespace Media {
class FreezerImpl : public Freezer, public NoCopyable {
    DECLARE_DELAYED_SINGLETON(FreezerImpl)
public:
    int32_t ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze) override;
    int32_t ResetAll() override;
};
} // namespace Media
} // namespace OHOS
#endif // FREEZER_IMPL_H