/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef MEDIA_DATA_SOURCE_CALLBACK_H_
#define MEDIA_DATA_SOURCE_CALLBACK_H_

#include <mutex>
#include "media_data_source.h"
#include "common_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace Media {
const std::string READAT_CALLBACK_NAME = "readAt";

class MediaDataSourceCallback : public IMediaDataSource, public NoCopyable {
public:
    MediaDataSourceCallback(napi_env env, int64_t fileSize);
    virtual ~MediaDataSourceCallback();
    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1) override;
    int32_t GetSize(int64_t &size) override;
    void SaveCallbackReference(const std::string &name, std::shared_ptr<AutoRef> ref);
    void ClearCallbackReference();
private:
    struct MediaDataSourceJsCallback {
        MediaDataSourceJsCallback(const std::string &callbackName, const std::shared_ptr<AVSharedMemory> &mem,
            uint32_t length, int64_t pos)
            :callbackName_(callbackName), memory_(mem), length_(length), pos_(pos)
        {
            readSize_ = nullptr;
        }
        ~MediaDataSourceJsCallback()
        {
            if (memory_ != nullptr) {
                memory_ = nullptr;
            }
        }
        std::shared_ptr<AutoRef> callback_;
        std::string callbackName_;
        std::shared_ptr<AVSharedMemory> memory_;
        uint32_t length_;
        int64_t pos_;
        napi_value readSize_;
        std::mutex mutexCond_;
        std::condition_variable cond_;
        void SetResult(napi_value val)
        {
            std::unique_lock<std::mutex> lock(mutexCond_);
            readSize_ = val;
            cond_.notify_all();
        }
        void WaitResult()
        {
            std::unique_lock<std::mutex> lock(mutexCond_);
            if (readSize_ == nullptr) {
                cond_.wait(lock, [this]() { return readSize_ != nullptr; });
            }
        }
    };
    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> refMap_;
    int64_t size_ = -1;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DATA_SOURCE_CALLBACK_H_
