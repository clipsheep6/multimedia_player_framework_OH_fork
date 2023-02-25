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

#include "player_server_task_mgr.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerServerTaskMgr"};
}

namespace OHOS {
namespace Media {
PlayerServerTaskMgr::PlayerServerTaskMgr()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

PlayerServerTaskMgr::~PlayerServerTaskMgr()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    (void)Reset();
}

int32_t PlayerServerTaskMgr::Init()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isInited_) {
        return MSERR_OK;
    }

    taskThread_ = std::make_unique<TaskQueue>("PlayerEngine");
    int32_t ret = taskThread_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "task thread start failed");

    isInited_ = true;

    return MSERR_OK;
}

int32_t PlayerServerTaskMgr::EnqueueTask(const std::shared_ptr<ITaskHandler> &task, PlayerServerTaskType type,
    const std::string &taskName)
{
    int32_t ret = taskThread_->EnqueueTask(task);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "launch two phase task failed");
    currTwoPhaseTask_ = task;
    currTwoPhaseType_ = type;
    currTwoPhaseTaskName_ = taskName;
    MEDIA_LOGI("task[%{public}s] start", currTwoPhaseTaskName_.c_str());
    return MSERR_OK;
}

int32_t PlayerServerTaskMgr::LaunchTask(const std::shared_ptr<ITaskHandler> &task, PlayerServerTaskType type,
    const std::string &taskName, const std::shared_ptr<ITaskHandler> &cancelTask)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(isInited_, MSERR_INVALID_OPERATION, "not init");

    if (type == PlayerServerTaskType::SEEKING || type == PlayerServerTaskType::RATE_CHANGE) {
        if (currTwoPhaseTask_ == nullptr) {
            return EnqueueTask(task, type, taskName);
        }
        MEDIA_LOGI("current two phase task[%{public}s] is in processing, the new task[%{public}s]",
            currTwoPhaseTaskName_.c_str(), taskName.c_str());
        for (auto &item : pendingTwoPhaseTasks_)  {
            if (item.type == type) {
                item.type = PlayerServerTaskType::CANCEL_TASK;
                MEDIA_LOGI("replace old task");
            }
        }
        pendingTwoPhaseTasks_.push_back({ type, task, cancelTask, taskName });
    } else if (type == PlayerServerTaskType::STATE_CHANGE) {
        if (currTwoPhaseTask_ == nullptr) {
            return EnqueueTask(task, type, taskName);
        } else {
            MEDIA_LOGI("current two phase task[%{public}s] is in processing, the new task[%{public}s]",
                currTwoPhaseTaskName_.c_str(), taskName.c_str());
            pendingTwoPhaseTasks_.push_back({ type, task, nullptr, taskName });
        }
    }
    return MSERR_OK;
}

int32_t PlayerServerTaskMgr::MarkTaskDone(const std::string &taskName)
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(isInited_, MSERR_INVALID_OPERATION, "not init");

    MEDIA_LOGI("task[%{public}s] end", taskName.c_str());
    currTwoPhaseTask_ = nullptr;
    currTwoPhaseType_ = PlayerServerTaskType::BUTT;

    if (!pendingTwoPhaseTasks_.empty()) {
        auto item = pendingTwoPhaseTasks_.front();
        pendingTwoPhaseTasks_.pop_front();
        currTwoPhaseType_ = item.type;
        if (item.type == PlayerServerTaskType::CANCEL_TASK) {
            currTwoPhaseTask_ = item.cancelTask;
        } else {
            currTwoPhaseTask_ = item.task;
        }
        currTwoPhaseTaskName_ = item.taskName;

        CHECK_AND_RETURN_RET_LOG(currTwoPhaseTask_ != nullptr, MSERR_OK, "task is nullptr");
        int32_t ret = taskThread_->EnqueueTask(currTwoPhaseTask_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret,
            "execute the stack top task failed, type: %{public}hhu", item.type);

        MEDIA_LOGI("task[%{public}s] start", currTwoPhaseTaskName_.c_str());
    }
    return MSERR_OK;
}

PlayerServerTaskType PlayerServerTaskMgr::GetCurrTaskType()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return currTwoPhaseType_;
}

void PlayerServerTaskMgr::ClearAllTask()
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(isInited_, "not init");

    currTwoPhaseTask_ = nullptr;
    currTwoPhaseType_ = PlayerServerTaskType::BUTT;
    pendingTwoPhaseTasks_.clear();

    auto dummyTask = std::make_shared<TaskHandler<void>>([]() {
        MEDIA_LOGI("execute dummy task...");
    });
    (void)taskThread_->EnqueueTask(dummyTask, true, 0);
    MEDIA_LOGD("exit");
}

int32_t PlayerServerTaskMgr::Reset()
{
    MEDIA_LOGD("enter");
    std::unique_lock<std::mutex> lock(mutex_);

    currTwoPhaseTask_ = nullptr;
    currTwoPhaseType_ = PlayerServerTaskType::BUTT;
    pendingTwoPhaseTasks_.clear();
    isInited_ = false;

    if (taskThread_ != nullptr) {
        std::unique_ptr<TaskQueue> tmp;
        std::swap(tmp, taskThread_);

        lock.unlock();
        int32_t ret = tmp->Stop();
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "stop task thread failed");
        lock.lock();
    }

    MEDIA_LOGD("exit");
    return MSERR_OK;
}
}
}
