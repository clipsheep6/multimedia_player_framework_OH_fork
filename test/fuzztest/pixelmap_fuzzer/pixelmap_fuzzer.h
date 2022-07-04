/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef PIXELMAP_FUZZER
#define PIXELMAP_FUZZER

#define FUZZ_PROJECT_NAME "pixelmap_fuzzer"
#include "test_player.h"
#include "test_metadata.h"

namespace OHOS {
namespace Media {
bool FuzzTestPixelMap(uint8_t* data, size_t size);
class PixelMapFuzzer : public TestMetadata {
public:
    PixelMapFuzzer();
    ~PixelMapFuzzer();
    bool FuzzPixelMap(uint8_t* data, size_t size);
};
}
}
#endif

