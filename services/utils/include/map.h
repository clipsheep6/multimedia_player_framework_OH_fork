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

#ifndef MAP_H
#define MAP_H

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
    void Insert(const std::pair<T, R> &p)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        map_.insert(p);
    }

    size_t Erase(const T &key)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return map_.erase(key);
    }

    const typename std::map<T, R>::iterator Erase(const typename std::map<T, R>::iterator positon)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return map_.erase(positon);
    }

    typename std::map<T, R>::iterator Begin()
    {
        return map_.begin();
    }

    typename std::map<T, R>::iterator End()
    {
        return map_.end();
    }

    typename std::map<T, R>::iterator Find(const T &key)
    {
        return map_.find(key);
    }
    
    size_t Count(const T &key)
    {
        return map_.count(key);
    }
    
    size_t Size()
    {
        return map_.size();
    }

    R& operator[](const T &key)
    {
        auto iter = map_.lower_bound(key);
        if (iter == map_.end()) {
            iter = map_.emplace_hint(iter, key, R{});
        }
        return iter->second;
    }

    R& operator[](T&& key)
    {
        auto iter = map_.lower_bound(key);
        if (iter == map_.end()) {
            iter = map_.emplace_hint(iter, key, R{});
        }
        return iter->second;
    }

    Map& operator=(Map&& map)
    {
        map_ = std::move(map);
        return *this;
    }

    Map& operator=(const Map& map)
    {
        map_ = map;
        return *this;
    }

    void Clear()
    {
        map_.clear();
    }
    
private:
    std::map<T, R> map_;
    std::mutex mutex_;
};
} // namespace Media
} // namespace OHOS
#endif