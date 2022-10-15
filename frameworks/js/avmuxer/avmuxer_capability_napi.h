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

#ifndef AVMUXER_CAPABILITY_NAPI_H
#define AVMUXER_CAPABILITY_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
class AVMuxerCapabilityNapi {
public:
    static napi_value GetAVMuxerFormatList(napi_env env, napi_callback_info info);

private:
    AVMuxerCapabilityNapi() = delete;
    ~AVMuxerCapabilityNapi() = delete;
};
} // namespace Media
} // namespace OHOS
#endif // AVMUXER_CAPABILITY_NAPI_H