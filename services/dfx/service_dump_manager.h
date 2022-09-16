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
#ifndef SERVICE_DUMP_MANAGER_H
#define SERVICE_DUMP_MANAGER_H
#include <functional>
#include <map>
#include <unordered_set>
#include <string>

namespace OHOS {
namespace Media {
using DfxDumper = std::function<int32_t(int32_t)>;
class __attribute__((visibility("default"))) ServiceDumpManager {
public:
    static ServiceDumpManager &GetInstance();
    void RegisterDfxDumper(std::u16string key, DfxDumper dump);
    int32_t Dump(int32_t fd, std::unordered_set<std::u16string> args);
private:
    ServiceDumpManager() = default;
    ~ServiceDumpManager() = default;
    std::map<std::u16string, DfxDumper> dfxDumper_;
};
} // namespace Media
} // namespace OHOS

#endif