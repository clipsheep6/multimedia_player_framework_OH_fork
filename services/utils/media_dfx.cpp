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

#include <media_dfx.h>
#include <unistd.h>
#include "securec.h"
#include "hitrace_meter.h"
#include "hitrace/tracechain.h"
#include "ipc_skeleton.h"
#include "media_utils.h"
#include "hitrace/tracechain.h"
#include "common/log.h"
#include "common/media_core.h"
#include "meta/any.h"
#include <cstdint>

namespace {
    constexpr uint32_t MAX_STRING_SIZE = 256;
    constexpr int64_t HOURS_BETWEEN_REPORTS = 4;
    constexpr int64_t MAX_MAP_SIZE = 100;

    std::mutex collectMut_;
    std::mutex reportMut_;
    std::map<OHOS::Media::CallType,
        std::map<int32_t, std::list<std::pair<uint64_t, std::shared_ptr<OHOS::Media::Meta>>>>> mediaInfoMap_;
    std::map<OHOS::Media::CallType,
        std::map<int32_t, std::list<std::pair<uint64_t, std::shared_ptr<OHOS::Media::Meta>>>>> reportMediaInfoMap_;
    std::map<uint64_t, std::pair<OHOS::Media::CallType, int32_t>> idMap_;
    std::chrono::system_clock::time_point currentTime_ = std::chrono::system_clock::now();
    bool g_reachMaxMapSize {false};

    bool CollectReportMediaInfo(uint64_t instanceId)
    {
        OHOS::Media::CallType ct;
        int32_t uid;
        std::pair<uint64_t, std::shared_ptr<OHOS::Media::Meta>> metaAppIdPair;
        {
            std::lock_guard<std::mutex> lock(collectMut_);
            MEDIA_LOG_I("CollectReportMediaInfo, instanceId is : %{public}" PRIu64, instanceId);
            auto idMapIt = idMap_.find(instanceId);
            if (idMapIt == idMap_.end()) {
                MEDIA_LOG_W("Not found instanceId in idMap, instanceId is : %{public}" PRIu64, instanceId);
                return false;
            }
            ct = idMapIt->second.first;
            uid = idMapIt->second.second;
            idMap_.erase(idMapIt);
            auto ctUidToMediaInfo = mediaInfoMap_.find(ct);
            if (ctUidToMediaInfo == mediaInfoMap_.end()) {
                MEDIA_LOG_W("Not found calltype, calltype is : %{public}d", static_cast<OHOS::Media::CallType>(ct));
                return false;
            }
            auto uidToMediaInfo = ctUidToMediaInfo->second.find(uid);
            if (uidToMediaInfo == ctUidToMediaInfo->second.end()) {
                MEDIA_LOG_W("Not found uid in mediaInfoMap_, uid is : %{public}" PRId32, uid);
                return false;
            }
            auto& instanceList = uidToMediaInfo->second;
            for (const auto& instancePair : instanceList) {
                if (instancePair.first == instanceId) {
                    metaAppIdPair = instancePair;
                    instanceList.remove(instancePair);
                    break;
                }
            }
        }
        std::lock_guard<std::mutex> lock(reportMut_);
        auto reportCtUidToMediaInfo  = reportMediaInfoMap_.find(ct);
        if (reportCtUidToMediaInfo != reportMediaInfoMap_.end()) {
            auto it = reportCtUidToMediaInfo->second.find(uid);
            if (it != reportCtUidToMediaInfo->second.end()) {
                it->second.push_back(metaAppIdPair);
            } else {
                reportCtUidToMediaInfo->second[uid].push_back(metaAppIdPair);
            }
        } else {
            reportMediaInfoMap_[ct][uid].push_back(metaAppIdPair);
        }
        g_reachMaxMapSize = (reportMediaInfoMap_[ct].size() >= MAX_MAP_SIZE);
        return true;
    }

    int32_t StatisticsEventReport()
    {
        MEDIA_LOG_I("StatisticsEventReport.");
        if (reportMediaInfoMap_.empty()) {
            MEDIA_LOG_I("reportMediaInfoMap_ is empty, can't report");
            return OHOS::Media::MSERR_INVALID_OPERATION;
        }
        OHOS::Media::MediaEvent event;
        for (const auto &it : reportMediaInfoMap_) {
            event.CommonStatisicsEventWrite(it.first, OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC, it.second);
        }
        auto currentTime = std::chrono::system_clock::now();
        currentTime_ = currentTime;
        reportMediaInfoMap_.clear();
        return OHOS::Media::MSERR_OK;
    }
}

namespace OHOS {
namespace Media {
using namespace OHOS::HiviewDFX;
bool MediaEvent::CreateMsg(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char msg[MAX_STRING_SIZE] = {0};
    auto ret = vsnprintf_s(msg, sizeof(msg), sizeof(msg) - 1, format, args);
    va_end(args);
    msg_ = msg;
    return ret < 0 ? false : true;
}

void MediaEvent::EventWrite(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    std::string module)
{
    int32_t pid = getpid();
    uint32_t uid = getuid();
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
        "PID", pid,
        "UID", uid,
        "MODULE", module,
        "MSG", msg_);
}

void MediaEvent::EventWriteWithAppInfo(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    std::string module, std::string status, int32_t appUid, int32_t appPid)
{
    int32_t pid = getpid();
    uint32_t uid = getuid();
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
        "PID", pid,
        "UID", uid,
        "MODULE", module,
        "MSG", msg_,
        "APP_PID", appPid,
        "APP_UID", appUid,
        "STATUS", status);
}

void MediaEvent::EventWriteBundleName(std::string eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    std::string module, std::string status, int32_t appUid, int32_t appPid, std::string bundleName)
{
    int32_t pid = getpid();
    uint32_t uid = getuid();
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
                    "PID", pid,
                    "UID", uid,
                    "MODULE", module,
                    "MSG", msg_,
                    "APP_PID", appPid,
                    "APP_UID", appUid,
                    "STATUS", status,
                    "BUNDLE", bundleName);
}

void MediaEvent::SourceEventWrite(const std::string& eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    const std::string& appName, uint64_t instanceId, const std::string& callerType, int8_t sourceType,
    const std::string& sourceUrl, const std::string& errMsg)
{
    std::string instanceIdStr = std::to_string(instanceId);
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
                    "APP_NAME", appName,
                    "INSTANCE_ID", instanceIdStr,
                    "CALLER_TYPE", callerType,
                    "SOURCE_TYPE", sourceType,
                    "SOURCE_URI", sourceUrl,
                    "ERROR_MESG", errMsg);
}

void MediaEvent::RecordAudioEventWrite(const std::string& eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    const std::string& appName, uint64_t instanceId, int8_t sourceType, const std::string& errorMessage)
{
    std::string instanceIdStr = std::to_string(instanceId);
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
                    "APP_NAME", appName,
                    "INSTANCE_ID", instanceIdStr,
                    "AUDIO_SOURCE_TYPE", sourceType,
                    "ERROR_MESG", errorMessage);
}

void MediaEvent::ScreenCaptureEventWrite(const std::string& eventName, OHOS::HiviewDFX::HiSysEvent::EventType type,
    const std::string& appName, uint64_t instanceId, int8_t captureMode, int8_t dataMode, int32_t errorCode,
    const std::string& errorMessage)
{
    std::string instanceIdStr = std::to_string(instanceId);
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
                    "APP_NAME", appName,
                    "INSTANCE_ID", instanceIdStr,
                    "CAPTURE_MODE", captureMode,
                    "DATA_MODE", dataMode,
                    "ERROR_CODE", errorCode,
                    "ERROR_MESG", errorMessage);
}

void MediaEvent::CommonStatisicsEventWrite(CallType callType, OHOS::HiviewDFX::HiSysEvent::EventType type,
    const std::map<int32_t, std::list<std::pair<uint64_t, std::shared_ptr<Meta>>>>& infoMap)
{
    MEDIA_LOG_I("MediaEvent::CommonStatisicsEventWrite");
    if (infoMap.empty()) {
        MEDIA_LOG_I("Player infoMap is empty.");
        return;
    }
    std::vector<std::string> infoArr;
    for (const auto& kv : infoMap) {
        json jsonArray;
        json eventInfoJson;
        json mediaEvents;
        for (const auto& listPair : kv.second) {
            json metaInfoJson;
            ParseOneEvent(listPair, metaInfoJson);
            mediaEvents.push_back(metaInfoJson);
        }
        eventInfoJson["appName"] = GetClientBundleName(kv.first);
        eventInfoJson["mediaEvents"] = mediaEvents;
        jsonArray.push_back(eventInfoJson);
        infoArr.push_back(jsonArray.dump());
    }

    StatisicsHiSysEventWrite(callType, type, infoArr);
}

void MediaEvent::ParseOneEvent(const std::pair<uint64_t, std::shared_ptr<OHOS::Media::Meta>> &listPair,
    json& metaInfoJson)
{
    for (auto it = listPair.second->begin(); it != listPair.second->end(); ++it) {
        Any valueType = OHOS::Media::GetDefaultAnyValue(it->first);
        if (Any::IsSameTypeWith<int32_t>(valueType)) {
            int32_t intVal;
            if (listPair.second->GetData(it->first, intVal)) {
                metaInfoJson[it->first] = std::to_string(intVal);
            }
        } else if (Any::IsSameTypeWith<uint32_t>(valueType)) {
            uint32_t uintVal;
            if (listPair.second->GetData(it->first, uintVal)) {
                metaInfoJson[it->first] = std::to_string(uintVal);
            }
        } else if (Any::IsSameTypeWith<std::string>(valueType)) {
            metaInfoJson[it->first] = AnyCast<std::string>(it->second);
        } else if (Any::IsSameTypeWith<int8_t>(valueType)) {
            int8_t intVal;
            if (listPair.second->GetData(it->first, intVal)) {
                metaInfoJson[it->first] = std::to_string(intVal);
            }
        } else if (Any::IsSameTypeWith<bool>(valueType)) {
            bool isTrue;
            if (listPair.second->GetData(it->first, isTrue)) {
                metaInfoJson[it->first] = isTrue ? "true" : "false";
            }
        } else {
            MEDIA_LOG_I("not found type matched with it->first: %{public}s", it->first.c_str());
        }
    }
}

void MediaEvent::StatisicsHiSysEventWrite(CallType callType, OHOS::HiviewDFX::HiSysEvent::EventType type,
    const std::vector<std::string>& infoArr)
{
    MEDIA_LOG_I("MediaEvent::StatisicsHiSysEventWrite");
    std::string eventName;
    switch (callType) {
        case CallType::AVPLAYER:
            eventName = "PLAYER_COMMON_STATISTICS";
            break;
        case CallType::AVRECORDER:
            eventName = "RECORDER_STATISTICS";
            break;
        case CallType::SCREEN_CAPTRUER:
            eventName = "SCREEN_CAPTURE_STATISTICS";
            break;
        default:
            return;
    }
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::MULTI_MEDIA, eventName, type,
                    "EVENTS", infoArr);
}

void BehaviorEventWrite(std::string status, std::string module)
{
    MediaEvent event;
    if (event.CreateMsg("%s, current state is: %s", "state change", status.c_str())) {
        event.EventWrite("PLAYER_STATE", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR, module);
    }
}

void BehaviorEventWriteForScreenCapture(std::string status, std::string module, int32_t appUid, int32_t appPid)
{
    MediaEvent event;
    if (event.CreateMsg("%s, current state is: %s", "state change", status.c_str())) {
        event.EventWriteWithAppInfo("PLAYER_STATE", OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
            module, status, appUid, appPid);
    }
}

void StatisticEventWriteBundleName(std::string status, std::string module)
{
    MediaEvent event;
    int32_t appUid = IPCSkeleton::GetCallingUid();
    int32_t appPid = IPCSkeleton::GetCallingPid();
    std::string bundleName = GetClientBundleName(appUid);
    if (event.CreateMsg("%s is invoke %s", bundleName.c_str(), module.c_str())) {
        event.EventWriteBundleName("PLAYER_STATISTICS", OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
            module, status, appUid, appPid, bundleName);
    }
}

void FaultEventWrite(std::string msg, std::string module)
{
    MediaEvent event;
    if (event.CreateMsg("%s", msg.c_str())) {
        event.EventWrite("PLAYER_ERR", OHOS::HiviewDFX::HiSysEvent::EventType::FAULT, module);
    }
}

void FaultSourceEventWrite(const std::string& appName, uint64_t instanceId, const std::string& callerType,
    int8_t sourceType, const std::string& sourceUrl, const std::string& errorMessage)
{
    MediaEvent event;
    event.SourceEventWrite("SOURCE_FAILURE", OHOS::HiviewDFX::HiSysEvent::EventType::FAULT, appName, instanceId,
        callerType, sourceType, sourceUrl, errorMessage);
}

void FaultRecordAudioEventWrite(const std::string& appName, uint64_t instanceId, int8_t sourceType,
    const std::string& errorMessage)
{
    MediaEvent event;
    event.RecordAudioEventWrite("RECORD_AUDIO_FAILURE", OHOS::HiviewDFX::HiSysEvent::EventType::FAULT, appName,
        instanceId, sourceType, errorMessage);
}

void FaultScreenCaptureEventWrite(const std::string& appName, uint64_t instanceId, int8_t captureMode, int8_t dataMode,
    int32_t errorCode, const std::string& errorMessage)
{
    MediaEvent event;
    event.ScreenCaptureEventWrite("SCREEN_CAPTURE_FAILURE", OHOS::HiviewDFX::HiSysEvent::EventType::FAULT, appName,
        instanceId, captureMode, dataMode, errorCode, errorMessage);
}

int32_t CreateMediaInfo(CallType callType, int32_t uid, uint64_t instanceId)
{
    MEDIA_LOG_I("CreateMediaInfo uid is: %{public}" PRId32 " instanceId is: %{public}" PRIu64, uid, instanceId);
    std::lock_guard<std::mutex> lock(collectMut_);
    auto instanceIdMap = idMap_.find(instanceId);
    if (instanceIdMap != idMap_.end()) {
        MEDIA_LOG_I("instanceId already exists id idMap_");
        return MSERR_INVALID_VAL;
    } else {
        MEDIA_LOG_I("CreateMediaInfo not found instanceId in idMap_, add the instanceId to idMap_");
        std::pair<CallType, int32_t> insertToMapPair(callType, uid);
        idMap_[instanceId] = insertToMapPair;
    }
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::pair<uint64_t, std::shared_ptr<Meta>> metaAppIdPair(instanceId, meta);
    auto ctUidToMediaInfo = mediaInfoMap_.find(callType);
    if (ctUidToMediaInfo != mediaInfoMap_.end()) {
        auto it = ctUidToMediaInfo->second.find(uid);
        if (it != ctUidToMediaInfo->second.end()) {
            it->second.push_back(metaAppIdPair);
            MEDIA_LOG_I("CreateMediaInfo: Successfully inserted metaAppIdPair for uid ");
        } else {
            ctUidToMediaInfo->second[uid].push_back(metaAppIdPair);
            MEDIA_LOG_I("CreateMediaInfo: Successfully created new list for uid and inserted metaAppIdPair.");
        }
    } else {
        mediaInfoMap_[callType][uid].push_back(metaAppIdPair);
        MEDIA_LOG_I("CreateMediaInfo: Successfully created new list for callType and uid ");
    }
    return MSERR_OK;
}

int32_t AppendMediaInfo(const std::shared_ptr<Meta>& meta, uint64_t instanceId)
{
    MEDIA_LOG_I("AppendMediaInfo.");
    if (meta == nullptr || meta->Empty()) {
        MEDIA_LOG_I("Insert meta is empty.");
        return MSERR_INVALID_OPERATION;
    }
    std::lock_guard<std::mutex> lock(collectMut_);
    auto idMapIt = idMap_.find(instanceId);
    if (idMapIt == idMap_.end()) {
        MEDIA_LOG_I("Not found instanceId when append meta, instanceId is : %{public}" PRIu64, instanceId);
        return MSERR_INVALID_VAL;
    }
    CallType ct = idMapIt->second.first;
    int32_t uid = idMapIt->second.second;
    auto ctUidToMediaInfo = mediaInfoMap_.find(ct);
    if (ctUidToMediaInfo == mediaInfoMap_.end()) {
        MEDIA_LOG_I("Not found calltype when append meta, calltype is : %{public}d", static_cast<CallType>(ct));
        return MSERR_INVALID_OPERATION;
    }
    auto it = ctUidToMediaInfo->second.find(uid);
    if (it == ctUidToMediaInfo->second.end()) {
        MEDIA_LOG_I("Not found uid when append meta, uid is : %{public}" PRId32, uid);
        return MSERR_INVALID_OPERATION;
    }
    auto& instanceList = it->second;
    for (const auto& instancePair : instanceList) {
        if (instancePair.first == instanceId) {
            auto arg = meta->begin();
            while (arg != meta->end()) {
                instancePair.second->SetData(arg->first, arg->second);
                ++arg;
            }
            break;
        }
    }
    return MSERR_OK;
}

int32_t ReportMediaInfo(uint64_t instanceId)
{
    MEDIA_LOG_I("Report.");
    MEDIA_LOG_I("Delete media info instanceId is: %{public}" PRIu64, instanceId);
    if (!CollectReportMediaInfo(instanceId)) {
        MEDIA_LOG_I("Collect media info fail.");
        return MSERR_INVALID_OPERATION;
    }
    std::lock_guard<std::mutex> lock(reportMut_);
    if (g_reachMaxMapSize) {
        MEDIA_LOG_I("Event data size exceeds 100, report the event");
        g_reachMaxMapSize = false;
        return StatisticsEventReport();
    }
    auto currentTime = std::chrono::system_clock::now();
    auto diff = currentTime - currentTime_;
    auto hour = std::chrono::duration_cast<std::chrono::hours>(diff).count();
    if (hour >= HOURS_BETWEEN_REPORTS) {
        MEDIA_LOG_I("Over 4 hours, report the event");
        return StatisticsEventReport();
    }
    return MSERR_OK;
}

MediaTrace::MediaTrace(const std::string &funcName)
{
    StartTrace(HITRACE_TAG_ZMEDIA, funcName);
    isSync_ = true;
}

void MediaTrace::TraceBegin(const std::string &funcName, int32_t taskId)
{
    StartAsyncTrace(HITRACE_TAG_ZMEDIA, funcName, taskId);
}

void MediaTrace::TraceEnd(const std::string &funcName, int32_t taskId)
{
    FinishAsyncTrace(HITRACE_TAG_ZMEDIA, funcName, taskId);
}

void MediaTrace::CounterTrace(const std::string &varName, int32_t val)
{
    CountTrace(HITRACE_TAG_ZMEDIA, varName, val);
}

MediaTrace::~MediaTrace()
{
    if (isSync_) {
        FinishTrace(HITRACE_TAG_ZMEDIA);
    }
}
} // namespace Media
} // namespace OHOS
