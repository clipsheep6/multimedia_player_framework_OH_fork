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

#ifndef FREEZER_H
#define FREEZER_H

#include <string>
#include <unordered_set>

namespace OHOS {
namespace Media {
class Freezer {
public:
    virtual ~Freezer() = default;
    virtual int32_t ProxyApp(const std::unordered_set<int32_t>& pidSet, const bool isFreeze) = 0;
    virtual int32_t ResetAll() = 0;
};

class FreezerFactory {
public:
    static std::shared_ptr<Freezer> CreateFreezer();

private:
    FreezerFactory() = default;
    ~FreezerFactory() = default;
};
} // namespace Media
} // namespace OHOS
#endif // FREEZER_H