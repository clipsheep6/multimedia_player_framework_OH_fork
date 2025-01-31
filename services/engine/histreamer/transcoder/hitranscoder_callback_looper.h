/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef HITRANSCODER_CALLBACKER_LOOPER_H
#define HITRANSCODER_CALLBACKER_LOOPER_H

#include <list>
#include <utility>
#include "osal/task/task.h"
#include "i_transcoder_engine.h"
#include "meta/any.h"
#include "osal/utils/steady_clock.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"

namespace OHOS {
namespace Media {
class HiTransCoderCallbackLooper : public ITransCoderEngineObs {
public:
    explicit HiTransCoderCallbackLooper();
    ~HiTransCoderCallbackLooper() override;
    bool IsStarted();
    void Stop();
    void StartWithTransCoderEngineObs(const std::weak_ptr<ITransCoderEngineObs>& obs);
    void SetTransCoderEngine(ITransCoderEngine* engine, std::string transCoderId);
    void StartReportMediaProgress(int64_t updateIntervalMs = 1000);
    void StopReportMediaProgress();
    void ManualReportMediaProgressOnce();
    void OnError(TransCoderErrorType errorType, int32_t errorCode) override;
    void OnInfo(TransCoderOnInfoType type, int32_t extra) override;
    void DoReportCompletedTime();

private:
    void DoReportMediaProgress();
    void DoReportInfo(const Any& info);
    void DoReportError(const Any& error);

    struct Event {
        Event(int32_t inWhat, int64_t inWhenMs, Any inAny): what(inWhat), whenMs(inWhenMs),
            detail(std::move(inAny)) {}
        int32_t what {0};
        int64_t whenMs {INT64_MAX};
        Any detail;
    };
    class EventQueue {
    public:
        void Enqueue(const std::shared_ptr<Event>& event);
        std::shared_ptr<Event> Next();
        void RemoveMediaProgressEvent();
        void Quit();
    private:
        OHOS::Media::Mutex queueMutex_ {};
        OHOS::Media::ConditionVariable queueHeadUpdatedCond_ {};
        std::list<std::shared_ptr<Event>> queue_ {};
        bool quit_ {false};
    };

    void LoopOnce(const std::shared_ptr<HiTransCoderCallbackLooper::Event>& item);
    void Enqueue(const std::shared_ptr<Event>& event);

    std::unique_ptr<OHOS::Media::Task> task_;
    OHOS::Media::Mutex loopMutex_ {};
    bool taskStarted_ {false};
    ITransCoderEngine* transCoderEngine_ {};
    std::weak_ptr<ITransCoderEngineObs> obs_ {};
    bool reportMediaProgress_ {false};
    bool isDropMediaProgress_ {false};
    int64_t reportProgressIntervalMs_ {100}; // default interval is 100 ms
};
}  // namespace Media
}  // namespace OHOS
#endif // HITRANSCODER_CALLBACKER_LOOPER_H
