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

#include <map>
#include <mutex>

namespace OHOS {
namespace Media {
template<typename T, typename R>
class Map {
public: 
    Map() = default;
    Map(const Map&) = default;
    Map(Map&&) = default;
    ~Map() = default;
    void insert(const std::pair<T, R> &p);
    void erase(const T &key);
    void erase(const typename std::map<T, R>::iterator positon);
    typename std::map<T, R>::iterator begin();
    typename std::map<T, R>::iterator end();
    typename std::map<T, R>::iterator find(const T &key);
    size_t count(const T &key);
    size_t size();
    R operator[](const T &key);
    void clear();
    
private:
    std::map<T, R> map_;
    
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS