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

#include "hiplayer_callback_looper.h"
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
constexpr int32_t TUPLE_POS_2 = 2;
}
HiPlayerCallbackLooper::HiPlayerCallbackLooper() : task_("callbackThread", OHOS::Media::TaskPriority::NORMAL)
{
    task_.RegisterJob([this] {LoopOnce();});
}

HiPlayerCallbackLooper::~HiPlayerCallbackLooper()
{
    Stop();
}

bool HiPlayerCallbackLooper::IsStarted()
{
    return taskStarted_;
}

void HiPlayerCallbackLooper::Stop()
{
    if (taskStarted_) {
        eventQueue_.Quit();
        task_.Stop();
        taskStarted_ = false;
    }
}

void HiPlayerCallbackLooper::StartWithPlayerEngineObs(const std::weak_ptr<IPlayerEngineObs>& obs)
{
    OHOS::Media::AutoLock lock(loopMutex_);
    obs_ = obs;
    if (!taskStarted_) {
        task_.Start();
        taskStarted_ = true;
        MEDIA_LOG_I("start callback looper");
    }
}
void HiPlayerCallbackLooper::SetPlayEngine(IPlayerEngine* engine)
{
    OHOS::Media::AutoLock lock(loopMutex_);
    playerEngine_ = engine;
}

void HiPlayerCallbackLooper::StartReportMediaProgress(int64_t updateIntervalMs)
{
    MEDIA_LOG_I("StartReportMediaProgress");
    reportProgressIntervalMs_ = updateIntervalMs;
    if (reportMediaProgress_) { // already set
        return;
    }
    reportMediaProgress_ = true;
    eventQueue_.Enqueue(std::make_shared<Event>(WHAT_MEDIA_PROGRESS, SteadyClock::GetCurrentTimeMs(), Any()));
}

void HiPlayerCallbackLooper::ManualReportMediaProgressOnce()
{
    eventQueue_.Enqueue(std::make_shared<Event>(WHAT_MEDIA_PROGRESS, SteadyClock::GetCurrentTimeMs(), Any()));
}

void HiPlayerCallbackLooper::StopReportMediaProgress()
{
    MEDIA_LOG_I("StopReportMediaProgress");
    reportMediaProgress_ = false;
}

void HiPlayerCallbackLooper::DropMediaProgress()
{
    isDropMediaProgress_ = true;
    eventQueue_.RemoveMediaProgressEvent();
}

void HiPlayerCallbackLooper::DoReportCompletedTime()
{
    OHOS::Media::AutoLock lock(loopMutex_);
    auto obs = obs_.lock();
    if (obs) {
        Format format;
        int32_t currentPositionMs;
        if (playerEngine_->GetDuration(currentPositionMs) == 0) {
            MEDIA_LOG_D("EVENT_AUDIO_PROGRESS completed position updated: " PUBLIC_LOG_D32, currentPositionMs);
            obs->OnInfo(INFO_TYPE_POSITION_UPDATE, currentPositionMs, format);
        } else {
            MEDIA_LOG_W("get player engine current time error");
        }
    }
}

void HiPlayerCallbackLooper::DoReportMediaProgress()
{
    OHOS::Media::AutoLock lock(loopMutex_);
    auto obs = obs_.lock();
    if (obs && !isDropMediaProgress_) {
        Format format;
        int32_t currentPositionMs;
        if (playerEngine_->GetCurrentTime(currentPositionMs) == 0) {
            MEDIA_LOG_D("EVENT_AUDIO_PROGRESS position updated: " PUBLIC_LOG_D32, currentPositionMs);
            obs->OnInfo(INFO_TYPE_POSITION_UPDATE, currentPositionMs, format);
        } else {
            MEDIA_LOG_W("get player engine current time error");
        }
    }
    isDropMediaProgress_ = false;
    if (reportMediaProgress_) {
        eventQueue_.Enqueue(std::make_shared<Event>(WHAT_MEDIA_PROGRESS,
            SteadyClock::GetCurrentTimeMs() + reportProgressIntervalMs_, Any()));
    }
}

void HiPlayerCallbackLooper::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    eventQueue_.Enqueue(std::make_shared<HiPlayerCallbackLooper::Event>(WHAT_ERROR, SteadyClock::GetCurrentTimeMs(),
    std::make_pair(errorType, errorCode)));
}

void HiPlayerCallbackLooper::DoReportError(const Any &error)
{
    OHOS::Media::AutoLock lock(loopMutex_);
    auto obs = obs_.lock();
    if (obs != nullptr) {
        auto ptr = AnyCast<std::pair<PlayerErrorType, int32_t>>(&error);
        MEDIA_LOG_E("Report error, error type: " PUBLIC_LOG_D32 " error value: " PUBLIC_LOG_D32,
            static_cast<int32_t>(ptr->first), static_cast<int32_t>(ptr->second));
        obs->OnError(ptr->first, ptr->second);
    }
}

void HiPlayerCallbackLooper::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    eventQueue_.Enqueue(std::make_shared<HiPlayerCallbackLooper::Event>(WHAT_INFO, SteadyClock::GetCurrentTimeMs(),
        std::make_tuple(type, extra, infoBody)));
}

void HiPlayerCallbackLooper::DoReportInfo(const Any& info)
{
    auto obs = obs_.lock();
    if (obs != nullptr) {
        auto ptr = AnyCast<std::tuple<PlayerOnInfoType, int32_t, Format>>(&info);
        MEDIA_LOG_I("Report info, info type: " PUBLIC_LOG_D32 " info value: " PUBLIC_LOG_D32,
            static_cast<int32_t>(std::get<TUPLE_POS_0>(*ptr)), static_cast<int32_t>(std::get<TUPLE_POS_1>(*ptr)));
        obs->OnInfo(std::get<TUPLE_POS_0>(*ptr), std::get<TUPLE_POS_1>(*ptr), std::get<TUPLE_POS_2>(*ptr));
    }
}

void HiPlayerCallbackLooper::LoopOnce()
{
    auto item = eventQueue_.Next();
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

void HiPlayerCallbackLooper::EventQueue::Enqueue(const std::shared_ptr<HiPlayerCallbackLooper::Event>& event)
{
    if (!event) {
        return;
    }
    if (event->what == WHAT_NONE) {
        MEDIA_LOG_I("invalid event");
    }
    OHOS::Media::AutoLock lock(queueMutex_);
    if (quit_) {
        MEDIA_LOG_W("event already quit");
        return;
    }
    auto ite = queue_.begin();
    for (; ite != queue_.end(); ite++) {
        if ((*ite)->whenMs > event->whenMs) {
            break;
        }
    }
    auto pos = queue_.insert(ite, event);
    if (pos == queue_.begin()) {
        queueHeadUpdatedCond_.NotifyOne();
    }
}

std::shared_ptr<HiPlayerCallbackLooper::Event> HiPlayerCallbackLooper::EventQueue::Next()
{
    OHOS::Media::AutoLock lock(queueMutex_);
    // not empty
    while (queue_.empty() && !quit_) {
        queueHeadUpdatedCond_.Wait(lock);
    }

    do {
        if (quit_) {
            return std::make_shared<HiPlayerCallbackLooper::Event>(WHAT_NONE, 0, Any());
        }
        auto wakenAtTime = (*queue_.begin())->whenMs;
        auto leftTime = wakenAtTime - SteadyClock::GetCurrentTimeMs();
        if (leftTime <= 0) {
            auto first = *queue_.begin();
            queue_.erase(queue_.begin());
            return first;
        }
        queueHeadUpdatedCond_.WaitFor(lock, leftTime);
    } while (1);
}

void HiPlayerCallbackLooper::EventQueue::Quit()
{
    OHOS::Media::AutoLock lock(queueMutex_);
    quit_ = true;
    queueHeadUpdatedCond_.NotifyOne();
}

void HiPlayerCallbackLooper::EventQueue::RemoveMediaProgressEvent()
{
    OHOS::Media::AutoLock lock(queueMutex_);
    for (auto iter = queue_.begin(); iter != queue_.end(); iter++) {
        if ((*iter)->what == WHAT_MEDIA_PROGRESS) {
            (*iter)->what = WHAT_NONE;
        }
    }
}
}  // namespace Media
}  // namespace OHOS