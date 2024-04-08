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
#ifndef MEDIA_PERMISSION_H
#define MEDIA_PERMISSION_H

namespace OHOS {
namespace Media {
class __attribute__((visibility("default"))) MediaPermission {
public:
    static int32_t CheckMicPermission();
    static int32_t CheckNetWorkPermission(int32_t appUid, int32_t appPid, uint32_t appTokenId);
};
} // namespace Media
} // namespace OHOS

#endif