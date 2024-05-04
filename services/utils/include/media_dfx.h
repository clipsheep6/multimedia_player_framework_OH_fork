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

#ifndef MEDIA_DFX_H
#define MEDIA_DFX_H

#include <cstring>
#include <list>
#include <memory>
#include <map>
#include <string>
#include <refbase.h>
#include "nocopyable.h"
#include "hisysevent.h"
#include "meta/meta.h"
#include "nlohmann/json.hpp"
#include <chrono>
#include <mutex>

namespace OHOS {
namespace Media {
using json = nlohmann::json;
enum CallType {
    AVPLAYER,
    AVRECORDER,
    METADATA_RETRIEVER,
    IMAGE_GENERATER,
    AVDEMUXER,
    AVMUXER,
    VIDEO_DECODER,
    VIDEO_ENCODER,
    AUDIO_DECODER,
    AUDIO_ENCODER,
    SOUNDPOOL,
    SCREEN_CAPTRUER,
    AVTRANSCODER
};
class __attribute__((visibility("default"))) MediaEvent : public NoCopyable {
public:
    MediaEvent() = default;
    ~MediaEvent() = default;
    bool CreateMsg(const char *format, ...) __attribute__((__format__(printf, 2, 3)));
    void EventWrite(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
        std::string module);
    void EventWriteWithAppInfo(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
        std::string module, std::string status, int32_t appUid, int32_t appPid);
    void EventWriteBundleName(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
        std::string module, std::string status, int32_t appUid, int32_t appPid, std::string bundleName);
    void SourceEventWrite(const std::string& eventName, OHOS::HiviewDFX::HiSysEvent::EventType type, const std::string&
        appName, uint64_t instanceId, const std::string& callerType, int8_t sourceType, const std::string& sourceUrl,
        const std::string& errMsg);
    void RecordAudioEventWrite(const std::string& eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
        const std::string& appName, uint64_t instanceId, int8_t sourceType, const std::string& errorMessage);
    void ScreenCaptureEventWrite(const std::string& eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
        const std::string& appName, uint64_t instanceId, int8_t captureMode, int8_t dataMode, int32_t errorCode,
        const std::string& errMsg);
    void CommonStatisicsEventWrite(CallType callType, OHOS::HiviewDFX::HiSysEvent::EventType type,
        const std::map<int32_t, std::list<std::pair<uint64_t, std::shared_ptr<Meta>>>>& infoMap);
private:
    void StatisicsHiSysEventWrite(CallType callType, OHOS::HiviewDFX::HiSysEvent::EventType type,
        const std::vector<std::string>& infoArr);
    void ParseOneEvent(const std::pair<uint64_t, std::shared_ptr<OHOS::Media::Meta>> &listPair, json& metaInfoJson);
    std::string msg_;
};


__attribute__((visibility("default"))) void BehaviorEventWrite(std::string status, std::string module);
__attribute__((visibility("default"))) void BehaviorEventWriteForScreenCapture(std::string status,
    std::string module, int32_t appUid, int32_t appPid);
__attribute__((visibility("default"))) void StatisticEventWriteBundleName(std::string status,
    std::string module);
__attribute__((visibility("default"))) void FaultEventWrite(std::string msg, std::string module);
__attribute__((visibility("default"))) void FaultSourceEventWrite(const std::string& appName, uint64_t instanceId,
    const std::string& callerType, int8_t sourceType, const std::string& sourceUrl, const std::string& errorMessage);
__attribute__((visibility("default"))) void FaultRecordAudioEventWrite(const std::string& appName, uint64_t instanceId,
    int8_t sourceType, const std::string& errorMessage);
__attribute__((visibility("default"))) void FaultScreenCaptureEventWrite(const std::string& appName,
    uint64_t instanceId, int8_t captureMode, int8_t dataMode, int32_t errorCode, const std::string& errorMessage);
__attribute__((visibility("default"))) int32_t CreateMediaInfo(CallType callType, int32_t uid, uint64_t instanceId);
__attribute__((visibility("default"))) int32_t AppendMediaInfo(const std::shared_ptr<Meta>& meta, uint64_t instanceId);
__attribute__((visibility("default"))) int32_t ReportMediaInfo(uint64_t instanceId);

class __attribute__((visibility("default"))) MediaTrace : public NoCopyable {
public:
    explicit MediaTrace(const std::string &funcName);
    static void TraceBegin(const std::string &funcName, int32_t taskId);
    static void TraceEnd(const std::string &funcName, int32_t taskId);
    static void CounterTrace(const std::string &varName, int32_t val);
    ~MediaTrace();
private:
    bool isSync_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DFX_H