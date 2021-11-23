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

#ifndef AVCODEC_LIST_H
#define AVCODEC_LIST_H

#include <vector>
#include <avcodec_info.h>
#include <format.h>

namespace OHOS {
namespace Media {
__attribute__((visibility("default"))) std::string FindVideoDecoder(const Format &format);
__attribute__((visibility("default"))) std::string FindVideoEncoder(const Format &format);
__attribute__((visibility("default"))) std::string FindAudioDecoder(const Format &format);
__attribute__((visibility("default"))) std::string FindAudioEncoder(const Format &format);
__attribute__((visibility("default"))) std::vector<std::shared_ptr<AVCodecInfo>> GetCodecInfos();
} // Media
} // OHOS
#endif // AVCODEC_LIST_H