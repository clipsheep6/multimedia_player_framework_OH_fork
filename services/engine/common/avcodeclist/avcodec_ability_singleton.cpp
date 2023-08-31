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

#include "avcodec_ability_singleton.h"
#include "avcodec_xml_parser.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecAbilitySingleton"};
}

namespace OHOS {
namespace Media {
AVCodecAbilitySingleton& AVCodecAbilitySingleton::GetInstance()
{
    static AVCodecAbilitySingleton instance;
    bool ret = instance.ParseCodecXml();
    CHECK_AND_RETURN_RET_LOG(ret == true, instance, "ParseCodecXml failed");
    return instance;
}

AVCodecAbilitySingleton::AVCodecAbilitySingleton()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecAbilitySingleton::~AVCodecAbilitySingleton()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool AVCodecAbilitySingleton::ParseCodecXml()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isParsered_) {
        return true;
    }
    AVCodecXmlParser xmlParser;

    CHECK_AND_RETURN_RET_LOG(xmlParser.LoadConfiguration() == true, false, "AVCodecList LoadConfiguration failed");
    CHECK_AND_RETURN_RET_LOG(xmlParser.Parse() == true, false, "AVCodecList Parse failed");
    std::vector<CapabilityData> data = xmlParser.GetCapabilityDataArray();
    capabilityDataArray_.insert(capabilityDataArray_.end(), data.begin(), data.end());
    isParsered_ = true;
    return true;
}

bool AVCodecAbilitySingleton::RegisterHdiCapability(const std::vector<CapabilityData> &registerCapabilityDataArray)
{
    std::lock_guard<std::mutex> lock(mutex_);
    capabilityDataArray_.insert(capabilityDataArray_.begin(), registerCapabilityDataArray.begin(),
        registerCapabilityDataArray.end());
    hdiCapLen_ = registerCapabilityDataArray.size();
    MEDIA_LOGD("RegisterHdiCapability success");
    return true;
}

std::vector<CapabilityData> AVCodecAbilitySingleton::GetCapabilityDataArray()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return capabilityDataArray_;
}
} // namespace Media
} // namespace OHOS
