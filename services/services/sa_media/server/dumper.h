/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef DUMPER_H
#define DUMPER_H

#include <functional>
#include "iremote_object.h"

namespace OHOS {
namespace Media {
class Dumper {
public:
    using DumperEntry = std::function<int32_t(int32_t)>;
    Dumper(pid_t pid, pid_t uid, DumperEntry entry, sptr<IRemoteObject> object) : pid_(pid), uid_(uid),
        entry_(entry), remoteObject_(object)
    {

    }
// private:
    pid_t pid_;
    pid_t uid_;
    DumperEntry entry_;
    sptr<IRemoteObject> remoteObject_;
};
}
}
#endif