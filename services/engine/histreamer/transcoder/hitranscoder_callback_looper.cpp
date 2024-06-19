/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "CallbackLooper"

#include "hitranscoder_callback_looper.h"
#include <utility>
#include "common/log.h"
#include "osal/task/autolock.h"
#include "osal/utils/steady_clock.h"

namespace OHOS {
namespace Media {
namespace {
constexpr int32_t WHAT_NONE = 0;
constexpr int32_t WHAT_MEDIA_PROGRESS = 1;
constexpr int32_t WHAT_INFO = 2;
constexpr int32_t WHAT_ERROR = 3;

constexpr int32_t TUPLE_POS_0 = 0;
constexpr int32_t TUPLE_POS_1 = 1;
}
HiTransCoderCallbackLooper::HiTransCoderCallbackLooper()
{
}

HiTransCoderCallbackLooper::~HiTransCoderCallbackLooper()
{
    Stop();
}

bool HiTransCoderCallbackLooper::IsStarted()
{
    return taskStarted_;
}

void HiTransCoderCallbackLooper::Stop()
{
    if (taskStarted_) {
        task_->Stop();
        taskStarted_ = false;
    }
}

void HiTransCoderCallbackLooper::StartWithTransCoderEngineObs(const std::weak_ptr<ITransCoderEngineObs>& obs)
{
    MEDIA_LOG_I("StartWithTransCoderEngineObs");
    OHOS::Media::AutoLock lock(loopMutex_);
    obs_ = obs;
    if (!taskStarted_) {
        task_->Start();
        taskStarted_ = true;
        MEDIA_LOG_I("start callback looper");
    }
}
void HiTransCoderCallbackLooper::SetTransCoderEngine(ITransCoderEngine* engine, std::string transCoderId)
{
    OHOS::Media::AutoLock lock(loopMutex_);
    transCoderEngine_ = engine;
    task_ = std::make_unique<Task>("callbackThread", transCoderId, TaskType::GLOBAL, TaskPriority::NORMAL, false);
}

void HiTransCoderCallbackLooper::StartReportMediaProgress(int64_t updateIntervalMs)
{
    MEDIA_LOG_I("StartReportMediaProgress");
    reportProgressIntervalMs_ = updateIntervalMs;
    if (reportMediaProgress_) { // already set
        return;
    }
    reportMediaProgress_ = true;
    Enqueue(std::make_shared<Event>(WHAT_MEDIA_PROGRESS, SteadyClock::GetCurrentTimeMs(), Any()));
}

void HiTransCoderCallbackLooper::ManualReportMediaProgressOnce()
{
    Enqueue(std::make_shared<Event>(WHAT_MEDIA_PROGRESS, SteadyClock::GetCurrentTimeMs(), Any()));
}

void HiTransCoderCallbackLooper::StopReportMediaProgress()
{
    MEDIA_LOG_I("StopReportMediaProgress");
    reportMediaProgress_ = false;
}

void HiTransCoderCallbackLooper::DoReportCompletedTime()
{
    OHOS::Media::AutoLock lock(loopMutex_);
    auto obs = obs_.lock();
    if (obs) {
        Format format;
        int32_t currentPositionMs;
        if (transCoderEngine_->GetDuration(currentPositionMs) == 0) {
            MEDIA_LOG_D("EVENT_AUDIO_PROGRESS completed position updated: " PUBLIC_LOG_D32, currentPositionMs);
            obs->OnInfo(TransCoderOnInfoType::INFO_TYPE_PROGRESS_UPDATE, currentPositionMs);
        } else {
            MEDIA_LOG_W("get transCoder engine current time error");
        }
    }
}

void HiTransCoderCallbackLooper::DoReportMediaProgress()
{
    OHOS::Media::AutoLock lock(loopMutex_);
    auto obs = obs_.lock();
    if (obs && !isDropMediaProgress_) {
        Format format;
        int32_t currentPositionMs;
        int32_t durationMs;
        if (transCoderEngine_->GetCurrentTime(currentPositionMs) == 0 &&
            transCoderEngine_->GetDuration(durationMs) == 0) {
            int32_t progress = currentPositionMs * 100 / durationMs;
            MEDIA_LOG_D("EVENT_AUDIO_PROGRESS position updated: " PUBLIC_LOG_D32, progress);
            obs->OnInfo(TransCoderOnInfoType::INFO_TYPE_PROGRESS_UPDATE, progress);
        } else {
            MEDIA_LOG_W("get transcoder engine current time error");
        }
    }
    isDropMediaProgress_ = false;
    if (reportMediaProgress_) {
        Enqueue(std::make_shared<Event>(WHAT_MEDIA_PROGRESS,
            SteadyClock::GetCurrentTimeMs() + reportProgressIntervalMs_, Any()));
    }
}

void HiTransCoderCallbackLooper::OnError(TransCoderErrorType errorType, int32_t errorCode)
{
    Enqueue(std::make_shared<HiTransCoderCallbackLooper::Event>(WHAT_ERROR, SteadyClock::GetCurrentTimeMs(),
    std::make_pair(errorType, errorCode)));
}

void HiTransCoderCallbackLooper::DoReportError(const Any &error)
{
    OHOS::Media::AutoLock lock(loopMutex_);
    auto obs = obs_.lock();
    if (obs != nullptr) {
        auto ptr = AnyCast<std::pair<TransCoderErrorType, int32_t>>(&error);
        MEDIA_LOG_E("Report error, error type: " PUBLIC_LOG_D32 " error value: " PUBLIC_LOG_D32,
            static_cast<int32_t>(ptr->first), static_cast<int32_t>(ptr->second));
    }
}

void HiTransCoderCallbackLooper::OnInfo(TransCoderOnInfoType type, int32_t extra)
{
    Enqueue(std::make_shared<HiTransCoderCallbackLooper::Event>(WHAT_INFO, SteadyClock::GetCurrentTimeMs(),
        std::make_tuple(type, extra)));
}

void HiTransCoderCallbackLooper::DoReportInfo(const Any& info)
{
    auto obs = obs_.lock();
    if (obs != nullptr) {
        auto ptr = AnyCast<std::tuple<TransCoderOnInfoType, int32_t>>(&info);
        MEDIA_LOG_I("Report info, info type: " PUBLIC_LOG_D32 " info value: " PUBLIC_LOG_D32,
            static_cast<int32_t>(std::get<TUPLE_POS_0>(*ptr)), static_cast<int32_t>(std::get<TUPLE_POS_1>(*ptr)));
        obs->OnInfo(std::get<TUPLE_POS_0>(*ptr), std::get<TUPLE_POS_1>(*ptr));
    }
}

void HiTransCoderCallbackLooper::LoopOnce(const std::shared_ptr<HiTransCoderCallbackLooper::Event>& item)
{
    switch (item->what) {
        case WHAT_MEDIA_PROGRESS:
            DoReportMediaProgress();
            break;
        case WHAT_INFO:
            DoReportInfo(item->detail);
            break;
        case WHAT_ERROR:
            DoReportError(item->detail);
            break;
        default:
            break;
    }
}

void HiTransCoderCallbackLooper::Enqueue(const std::shared_ptr<HiTransCoderCallbackLooper::Event>& event)
{
    if (event->what == WHAT_NONE) {
        MEDIA_LOG_I("invalid event");
    }
    int64_t delayUs = (event->whenMs - SteadyClock::GetCurrentTimeMs()) * 1000;
    task_->SubmitJob([this, event]() {
            LoopOnce(event);
        }, delayUs);
}
}  // namespace Media
}  // namespace OHOS