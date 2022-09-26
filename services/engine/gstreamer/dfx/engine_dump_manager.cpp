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

#include "engine_dump_manager.h"
#include <unistd.h>
#include "service_dump_manager.h"
#include "gmemdfxdump.h"
#include "media_errors.h"

namespace OHOS {
namespace Media {
EngineDumpManager &EngineDumpManager::GetInstance()
{
    static EngineDumpManager EngineDumpManager;
    return EngineDumpManager;
}

void EngineDumpManager::Init()
{
    auto MemInfoDump = std::bind(&EngineDumpManager::DumpGlibMemInfo, this, std::placeholders::_1);
    std::u16string keyMem = u"glibmem";
    ServiceDumpManager::GetInstance().RegisterDfxDumper(keyMem, MemInfoDump);

    auto MemPoolInfoDump = std::bind(&EngineDumpManager::DumpGlibMemPoolInfo, this, std::placeholders::_1);
    std::u16string keyPool = u"glibpool";
    ServiceDumpManager::GetInstance().RegisterDfxDumper(keyPool, MemPoolInfoDump);

    std::string dumpString;
    GetGMemDump(dumpString);
}

int32_t EngineDumpManager::DumpGlibMemInfo(int32_t fd)
{
    std::string dumpString;
    GetGMemDump(dumpString);
    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
        dumpString.clear();
    }
    return MSERR_OK;
}

int32_t EngineDumpManager::DumpGlibMemPoolInfo(int32_t fd)
{
    std::string dumpString;
    GetGMemPoolDump(dumpString);
    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
        dumpString.clear();
    }
    return MSERR_OK;
}
} // namespace Media
} // namespace OHOS