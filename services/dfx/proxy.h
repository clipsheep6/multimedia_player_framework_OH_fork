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
#ifndef PROXY_H
#define PROXY_H

#include "player_xcollie.h"

namespace OHOS {
namespace Media {
class Proxy {
public:
    static Proxy &GetInstance() 
    {
        static Proxy proxy;
        return proxy;
    }
    template <typename T, typename R, typename... Args>
    typename std::enable_if<!std::is_same<R, void>::value, R>::type Execute(
        const std::unique_ptr<T> &obj, R (T::*p)(Args...) const, std::string name,
        Args&&... args) {
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog(name);
        R ret = (*obj.*p)(std::forward<Args>(args)...);
        PlayerXCollie::GetInstance().CancelTimer(id);
        return ret;
    }

    template <typename T, typename R, typename... Args>
    typename std::enable_if<std::is_same<R, void>::value, void>::type Execute(
        const std::unique_ptr<T> &obj, R (T::*p)(Args...) const, std::string name,
        Args&&... args) {
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog(name);
        (*obj.*p)(std::forward<Args>(args)...);
        PlayerXCollie::GetInstance().CancelTimer(id);
    }

    template <typename T, typename R, typename... Args>
    typename std::enable_if<!std::is_same<R, void>::value, R>::type Execute(
        const std::unique_ptr<T> &obj, R (T::*p)(Args...), std::string name,
        Args&&... args) {
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog(name);
        R ret = (*obj.*p)(std::forward<Args>(args)...);
        PlayerXCollie::GetInstance().CancelTimer(id);
        return ret;
    }

    template <typename T, typename R, typename... Args>
    typename std::enable_if<std::is_same<R, void>::value, void>::type Execute(
        const std::unique_ptr<T> &obj, R (T::*p)(Args...), std::string name,
        Args&&... args) {
        int32_t id = PlayerXCollie::GetInstance().SetTimerByLog(name);
        (*obj.*p)(std::forward<Args>(args)...);
        PlayerXCollie::GetInstance().CancelTimer(id);
    }
};
} // namespace Media
} // namespace OHOS
#endif