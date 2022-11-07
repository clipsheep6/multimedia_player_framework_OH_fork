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

#include "map.h"

namespace OHOS {
namespace Media {
template<typename T, typename R>
void Map<T, R>::insert(const std::pair<T, R> &p)
{
    std::unique_lock<std::mutex> lock(mutex_);
    map_.insert(p);
}

template<typename T, typename R>
void Map<T, R>::erase(const T &key)
{
    std::unique_lock<std::mutex> lock(mutex_);
    map_.erase(key);
}

template<typename T, typename R>
void Map<T, R>::erase(const typename std::map<T, R>::iterator positon)
{
    std::unique_lock<std::mutex> lock(mutex_);
    map_.erase(positon);
}

template<typename T, typename R>
typename std::map<T, R>::iterator Map<T,R>::begin()
{
    return map_.begin();
}

template<typename T, typename R>
typename std::map<T, R>::iterator Map<T,R>::end()
{
    return map_.end();
}

template<typename T, typename R>
typename std::map<T, R>::iterator Map<T, R>::find(const T &key)
{
    return map_.find(key);
}

template<typename T, typename R>
size_t Map<T, R>::count(const T &key)
{
    return map_.count(key);
}


template<typename T, typename R>
size_t Map<T, R>::size()
{
    return map_.size();
}

template<typename T, typename R>
Map<T, R>::~Map()
{
    map_ = nullptr;
}

template<typename T, typename R>
R Map<T, R>::operator[](const T &key)
{
    return map_[key];
}
} // namespace Media
} // namespace OHOS