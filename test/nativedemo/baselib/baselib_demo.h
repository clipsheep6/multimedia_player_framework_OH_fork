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

#ifndef BASELIB_DEMO_H
#define BASELIB_DEMO_H

#include <climits>
#include <thread>
#include <queue>
#include <string>
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class BaseLibDemo : public NoCopyable {
public:
    BaseLibDemo() = default;
    ~BaseLibDemo() = default;
    void RunCase();
private:
    void RunMemoryAllocation();
    void RunMemorySet();
    void RunMemoryCopy();
    void RunMultiThreadMem();
    void GenerateRandList(int32_t *r, const int32_t &len, const int32_t &left, const int32_t &right);
    std::unique_ptr<std::thread> memsetsThread_;
    std::unique_ptr<std::thread> memcpysThread_;
    std::unique_ptr<std::thread> mallocThread_;
    int32_t epoch_ = 2000; // default run 2000 epoch
    int32_t runTimesPerEpoch_ = 1000; // every epoch run 1000 times base lib function
};
}  // namespace Media
}  // namespace OHOS
#endif  // BASELIB_DEMO_H