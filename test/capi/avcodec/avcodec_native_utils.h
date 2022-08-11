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


#ifndef AVCODEC_NATIVE_UTILS_H
#define AVCODEC_NATIVE_UTILS_H

#include <queue>
#include "native_avmemory.h"

namespace OHOS {
namespace Media {
__attribute__((visibility("default"))) void ClearIntQueue(std::queue<uint32_t> &q);
__attribute__((visibility("default"))) void ClearBufferQueue(std::queue<AVMemory *> &q);
__attribute__((visibility("default"))) void ClearBoolQueue(std::queue<bool> &q);
}
}
#endif // AVCODEC_NATIVE_UTILS_H