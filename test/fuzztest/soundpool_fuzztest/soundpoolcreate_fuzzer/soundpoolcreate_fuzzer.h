/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef SOUNDPOOLCREATE_FUZZER
#define SOUNDPOOLCREATE_FUZZER

#include <fcntl.h>
#include <securec.h>
#include <unistd.h>
#include <cstdint>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include "test_soundpool.h"

#define FUZZ_PROJECT_NAME "soundpoolcreate_fuzzer"

namespace OHOS {
namespace Media {
class SoundPoolCreateFuzzer : public TestSoundPool {
public:
    SoundPoolCreateFuzzer();
    ~SoundPoolCreateFuzzer();
    bool FuzzSoundPoolCreate(uint8_t *data, size_t size);
    bool FuzzSoundPoolCreateFlags(uint8_t *data, size_t size);
    static const int32_t waitTime3 = 3;
};
} // namespace Media
bool FuzzTestSoundPoolCreate(uint8_t *data, size_t size);
bool FuzzTestSoundPoolCreateFlags(uint8_t *data, size_t size);
} // namespace OHOS
#endif