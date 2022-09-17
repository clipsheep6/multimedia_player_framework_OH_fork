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

#include "service_dump_manager.h"
#include <unistd.h>
#include <locale>
#include <codecvt>
#include "media_errors.h"

namespace OHOS {
namespace Media {
ServiceDumpManager &ServiceDumpManager::GetInstance()
{
    static ServiceDumpManager ServiceDumpManager;
    return ServiceDumpManager;
}

void ServiceDumpManager::RegisterDfxDumper(std::u16string key, DfxDumper dump)
{
    dfxDumper_[key] = dump;
}

int32_t ServiceDumpManager::Dump(int32_t fd, std::unordered_set<std::u16string> args)
{
    std::string dumpString = "------------------MediaDfx------------------\n";
    for (auto iter : dfxDumper_) {
        if (args.find(static_cast<std::u16string>(iter.first)) != args.end()) {
            dumpString += "-----";
            dumpString += std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> {}.to_bytes(iter.first);
            dumpString += "-----\n";
            if (fd != -1) {
                write(fd, dumpString.c_str(), dumpString.size());
                dumpString.clear();
            }
            if (iter.second(fd) != MSERR_OK) {
                return MSERR_INVALID_OPERATION;
            }
        }
    }
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS