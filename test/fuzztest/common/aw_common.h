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

#ifndef AWCOMMON_H
#define AWCOMMON_H

#include <string>
#include "window.h"
#include "recorder.h"
namespace OHOS {
namespace Media {
#define CHECK_INSTANCE_AND_RETURN_RET(cond, ret, ...)           \
    do {                                                        \
        if (cond == nullptr) {                                  \
            cout << cond << "is nullptr" << endl;               \
            return ret;                                         \
        }                                                       \
    } while (0)

#define CHECK_BOOL_AND_RETURN_RET(cond, ret, ...)               \
    do {                                                        \
        if (!(cond)) {                                          \
            return ret;                                         \
        }                                                       \
    } while (0)

#define CHECK_STATE_AND_RETURN_RET(cond, ret, ...)              \
    do {                                                        \
        if (cond != 0) {                                        \
            return ret;                                         \
        }                                                       \
    } while (0)
constexpr int32_t WAITTING_TIME = 2;
namespace PlayerTestParam {
    int32_t WriteDataToFile(const std::string &path, const std::uint8_t *data, std::size_t size);
    int32_t ProduceRandomNumberCrypt(void);
}
}
}
#endif