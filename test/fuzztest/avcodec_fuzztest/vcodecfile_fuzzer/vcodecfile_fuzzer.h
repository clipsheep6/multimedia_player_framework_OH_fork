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

#ifndef VCODECFILE_FUZZER_H
#define VCODECFILE_FUZZER_H

#define FUZZ_PROJECT_NAME "vcodecfile_fuzzer"
#include "vdec_mock.h"
#include "venc_mock.h"

namespace OHOS {
namespace Media {
bool FuzzVCodecFile(uint8_t *data, size_t size);

class VCodecFileFuzzer : public NoCopyable {
public:
    VCodecFileFuzzer();
    ~VCodecFileFuzzer();
    bool FuzzVideoFile(uint8_t *data, size_t size);
protected:
    std::shared_ptr<VDecMock> videoDec_ = nullptr;
    std::shared_ptr<VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<VEncMock> videoEnc_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
};
}
}
#endif

