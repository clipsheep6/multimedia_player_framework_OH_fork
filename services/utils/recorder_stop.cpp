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

#include "recorder_stop.h"

namespace OHOS {
namespace Media {
RecorderStop* RecorderStop::instance_;
std::mutex RecorderStop::mutex_;

RecorderStop* RecorderStop::GetInstance() {
    if (instance_ == nullptr) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (instance_ == nullptr) {
            instance_ = new RecorderStop();
        }
    }
    return instance_;
}

void RecorderStop::RegisterStopCallback(std::shared_ptr<StopCallback> stopCallback) {
    std::unique_lock<std::mutex> lock(mutex_);
    stopCallbacks_.push_back(stopCallback);
    stopCallbackNum_++;
}

void RecorderStop::NotifyStop(int64_t stopTime) {
    std::unique_lock<std::mutex> lock(mutex_);
    for (int i = 0; i < stopCallbacks_.size(); i++) {
        stopCallbacks_[i]->OnStop(stopTime);
    }
}

void RecorderStop::RegisterStopDoneCallback(std::shared_ptr<StopDoneCallback> stopDoneCallback) {
    std::unique_lock<std::mutex> lock(mutex_);
    stopDoneCallbacks_.push_back(stopDoneCallback);
}

void RecorderStop::NotifyStopDone() {
    std::unique_lock<std::mutex> lock(mutex_);
    stopCallbackNum_--;
    if (stopCallbackNum_ == 0) {
        for (int i = 0; i < stopDoneCallbacks_.size(); i++) {
            stopDoneCallbacks_[i]->OnStopDone();
        }
    }
}

void RecorderStop::SetEncoder(bool isEncoder) {
    std::unique_lock<std::mutex> lock(mutex_);
    isEncoder_ = true;
}

bool RecorderStop::GetEncoder() {
    std::unique_lock<std::mutex> lock(mutex_);
    return isEncoder_;
}

void RecorderStop::Release() {
    stopCallbacks_.clear();
    stopDoneCallbacks_.clear();
}

RecorderStop::RecorderStop() {
    isEncoder_ = false;
    stopCallbackNum_ = 0;
}

RecorderStop::~RecorderStop() {
}

} // namespace Media
} // namespace OHOS
