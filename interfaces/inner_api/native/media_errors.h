/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef MEDIA_ERRORS_H
#define MEDIA_ERRORS_H

#include <map>
#include <string>
#include "errors.h"
#include "media_core.h"

namespace OHOS {
namespace Media {

__attribute__((visibility("default"))) std::string MSErrorToString(MediaServiceErrCode code);
__attribute__((visibility("default"))) std::string MSExtErrorToString(MediaServiceExtErrCode code);
__attribute__((visibility("default"))) std::string MSErrorToExtErrorString(MediaServiceErrCode code);
__attribute__((visibility("default"))) MediaServiceExtErrCode MSErrorToExtError(MediaServiceErrCode code);

__attribute__((visibility("default"))) std::string MSExtErrorAPI9ToString(MediaServiceExtErrCodeAPI9 code,
    const std::string& param1, const std::string& param2);
__attribute__((visibility("default"))) std::string MSErrorToExtErrorAPI9String(MediaServiceErrCode code,
    const std::string& param1, const std::string& param2);
__attribute__((visibility("default"))) MediaServiceExtErrCodeAPI9 MSErrorToExtErrorAPI9(MediaServiceErrCode code);

__attribute__((visibility("default"))) std::string MSExtAVErrorToString(MediaServiceExtErrCodeAPI9 code);
} // namespace Media
} // namespace OHOS
#endif // MEDIA_ERRORS_H
