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

#ifndef I_MEDIA_PROFILE_SERVICE_H
#define I_MEDIA_PROFILE_SERVICE_H

#include "recorder_profiles.h"
#include "avsharedmemory.h"

namespace OHOS {
namespace Media {
class IRecorderProfilesService {
public:
    virtual ~IRecorderProfilesService() = default;
    virtual bool IsAudioRecoderConfigSupported(const RecorderProfilesData &profile) = 0;
    virtual bool HasVideoRecorderProfile(int32_t sourceId, int32_t qualityLevel) = 0;
    virtual std::vector<RecorderProfilesData> GetAudioRecorderCapsInfo() = 0;
    virtual std::vector<RecorderProfilesData> GetVideoRecorderCapsInfo() = 0;
    virtual RecorderProfilesData GetVideoRecorderProfileInfo(int32_t sourceId, int32_t qualityLevel) = 0;
};
}  // namespace Media
}  // namespace OHOS
#endif