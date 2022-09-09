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

#ifndef RECORDERSTOP_FUZZER
#define RECORDERSTOP_FUZZER

#include <cstdint>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include "test_recorder.h"

#define FUZZ_PROJECT_NAME "recorderstop_fuzzer"

namespace OHOS {
namespace Media {
class RecorderStopFuzzer : public TestRecorder {
public:
    RecorderStopFuzzer();
    ~RecorderStopFuzzer();
    bool FuzzRecorderStop(uint8_t *data, size_t size);
};
}
bool FuzzTestRecorderStop(uint8_t *data, size_t size);
}
#endif