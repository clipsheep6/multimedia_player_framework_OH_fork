/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <media_dfx.h>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDFX"};
}

namespace OHOS {
namespace Media {
void MediaDFX::EventReport(std::string eventName, EventType type, std::string msg)
{
    int32_t pid = getpid();
    uint32_t uid = getuid();
    OHOS::HiviewDFX::HiSysEvent::Write("MEDIA", eventName, type,
        "PID", pid,
        "UID", uid,
        "PROCESS_NAME", "meida server"
        "MSG", msg)
}
} // namespace Media
} // namespace OHOS