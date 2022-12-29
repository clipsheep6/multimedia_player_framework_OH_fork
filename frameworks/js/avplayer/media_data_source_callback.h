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

#ifndef MEDIA_DATA_SOURCE_CALLBACK_H_
#define MEDIA_DATA_SOURCE_CALLBACK_H_

#include "media_data_source.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "common_napi.h"

namespace OHOS {
namespace Media {

const std::string READAT_CALLBACK_NAME = "readAt";

class MediaDataSourceCallback : public IMediaDataSource, public NoCopyable {
public:
    MediaDataSourceCallback(napi_env env, int64_t fileSize);
    virtual ~MediaDataSourceCallback();
    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1) override;
    int32_t GetSize(int64_t &size) override;
    void SaveCallbackReference(const std::string &name, std::weak_ptr<AutoRef> ref);
    void ClearCallbackReference();
private:
    struct MediaDataSourceJsCallback {
        std::weak_ptr<AutoRef> callback;
        std::string callbackName = "unknown";
        std::shared_ptr<AVSharedMemory> memory;
        uint32_t length;
        int64_t pos;
        int32_t size;
    };
    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::map<std::string, std::weak_ptr<AutoRef>> refMap_;
    int64_t size_ = -1;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DATA_SOURCE_CALLBACK_H_
