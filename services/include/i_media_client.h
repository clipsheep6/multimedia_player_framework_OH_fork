/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef I_MEDIA_CLIENT_H
#define I_MEDIA_CLIENT_H

#include <memory>

namespace OHOS {
namespace Media {
class IMediaClient {
public:
    virtual ~IMediaClient() = default;
    virtual std::shared_ptr<IMediaClient> CreateClient();
};

class __attribute__((visibility("default"))) MediaClientFactory {
public:
    /**
     * @brief IMediaClient singleton
     *
     * Create Recorder Client and Player Client Through the Media Client.
     *
     * @return Returns IMediaClient singleton;
     * @since 1.0
     * @version 1.0
     */
    static IMediaClient &GetInstance();
private:
    MediaClientFactory() = delete;
    ~MediaClientFactory() = delete;
};
} // namespace Media
} // namespace OHOS
#endif // I_MEDIA_SERVICE_H
