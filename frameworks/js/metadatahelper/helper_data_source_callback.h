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

#ifndef HELPER_DATA_SOURCE_CALLBACK_H
#define HELPER_DATA_SOURCE_CALLBACK_H

#include <mutex>
#include <uv.h>
#include "common/media_data_source.h"
#include "common_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
const std::string HELPER_READAT_CALLBACK_NAME = "readAt";

struct HelperDataSourceJsCallback {
    HelperDataSourceJsCallback(const std::string &callbackName, const std::shared_ptr<AVSharedMemory> &mem,
        uint32_t length, int64_t pos)
        :callbackName_(callbackName), memory_(mem), length_(length), pos_(pos) {
    }
    ~HelperDataSourceJsCallback();
    void WaitResult();
    std::weak_ptr<AutoRef> callback_;
    std::string callbackName_;
    std::shared_ptr<AVSharedMemory> memory_;
    uint32_t length_;
    int64_t pos_;
    int32_t readSize_ = 0;
    std::mutex mutexCond_;
    std::condition_variable cond_;
    bool setResult_ = false;
    bool isExit_ = false;
};

struct HelperDataSourceJsCallbackWraper {
    std::weak_ptr<HelperDataSourceJsCallback> cb_;
};

class HelperDataSourceCallback : public IMediaDataSource, public NoCopyable {
public:
    HelperDataSourceCallback(napi_env env, int64_t fileSize);
    virtual ~HelperDataSourceCallback();
    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1) override;
    int32_t GetSize(int64_t &size) override;
    void SaveCallbackReference(const std::string &name, std::shared_ptr<AutoRef> ref);
    int32_t GetCallback(const std::string &name, napi_value *callback);
    void ClearCallbackReference();
    static bool AddNapiValueProp(napi_env env, napi_value obj, const std::string &key, napi_value value);

    // This interface has been deprecated
    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
    // This interface has been deprecated
    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem) override;
private:
    int32_t UvWork(uv_loop_s *loop, uv_work_t *work);
    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::map<std::string, std::shared_ptr<AutoRef>> refMap_;
    int64_t size_ = -1;
    std::shared_ptr<HelperDataSourceJsCallback> cb_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // HELPER_DATA_SOURCE_CALLBACK_H