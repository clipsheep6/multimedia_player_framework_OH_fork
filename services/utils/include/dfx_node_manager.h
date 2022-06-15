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

#ifndef DFX_NODE_MANAGER_H
#define DFX_NODE_MANAGER_H

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <sstream>
#include <mutex>
#include "task_queue.h"

namespace {
const std::string PLAYER_SERVICE = "PlayerService";
const std::string CODEC_SERVICE = "CodecService";
const std::string CODEC_BIN = "CodecBin";
const std::string HARDWARE_VIDEO_DECODER = "HardWareVideoDecoder";
const std::string HARDWARE_VIDEO_ENCODER = "HardWareVideoEncoder";
const std::string SINK = "SINK";
const std::string SOURCE = "SOURCE";
const std::string AVSHMEM_POOL = "AvshmemPool";
const std::string SURFACE_POOL = "SurfacePool";
const std::string CONSUMER_SURFACE_POOL = "ConsumerSurfacePool";
}

namespace OHOS {
namespace Media {
struct __attribute__((visibility("default"))) DfxNode : public std::enable_shared_from_this<DfxNode> {
public:
    DfxNode() noexcept {};
    explicit DfxNode(const std::string &name) noexcept : name_(name) {};

    template <typename T>
    void UpdateValTag(std::string name, T value) {
        std::unique_lock<std::mutex> lock(valMutex_);
        if (valMap_.find(name) == valMap_.end()) {
            valMap_[name] = std::list<std::string>();
        }
        std::stringstream stream;
        stream << value;
        std::string strValue;
        stream >> strValue;
        valMap_[name].push_back(strValue);
        if (valMap_[name].size() > VAL_TAG_SIZE) {
            valMap_[name].pop_front();
        }
    }

    void UpdateFuncTag(const std::string &funcTag, bool insert);
    void UpdateClassTag(void *thiz, const std::string &classTag, bool insert);
    std::shared_ptr<DfxNode> GetChildNode(const std::string &name);
    void DumpInfo(std::string &dumpString);
private:
    std::weak_ptr<DfxNode> parent;
    std::vector<std::shared_ptr<DfxNode>> childs;
    std::unordered_map<std::string, std::list<std::string>> valMap_;
    std::unordered_map<void *, std::string> classMap_;
    std::list<std::string> funcTags_;
    std::unordered_set<std::string> funcSet_;
    std::string name_;
    const uint32_t FUNC_TAG_SIZE = 20;
    const uint32_t VAL_TAG_SIZE = 10;
    std::mutex valMutex_;
    std::mutex funcMutex_;
    std::mutex classMutex_;
    std::mutex childMutex_;
};

class __attribute__((visibility("default"))) DfxNodeManager {
public:
    static DfxNodeManager &GetInstance();
    std::shared_ptr<DfxNode> CreateDfxNode(std::string name);
    std::shared_ptr<DfxNode> CreateChildDfxNode(const std::shared_ptr<DfxNode> &parentNode, std::string name);
    void SaveErrorDfxNode(const std::shared_ptr<DfxNode> &node);
    void SaveFinishDfxNode(const std::shared_ptr<DfxNode> &node);
    void DumpErrorDfxNode(std::string &str);
    void DumpFinishDfxNode(std::string &str);
    void DumpDfxNode(const std::shared_ptr<DfxNode> &node, std::string &str);
private:
    DfxNodeManager();
    ~DfxNodeManager();
    void GetDfxLevel();
    void CleanTask();
    std::list<std::shared_ptr<DfxNode>> errorNodeVec_;
    std::list<std::shared_ptr<DfxNode>> finishNodeVec_;
    std::mutex mutex_;
    int32_t level_;
    TaskQueue cleanNodeTask_;
    std::condition_variable cond_;
};

class DfxClassHelper {
public:
    void Init(void *thiz, const std::string& className, const std::shared_ptr<DfxNode> &node)
    {
        className_ = className;
        node_ = node;
        thiz_ = thiz;
        if (node_) {
            node_->UpdateClassTag(thiz_, className_, true);
        }
    }
    void DeInit()
    {
        if (node_) {
            node_->UpdateClassTag(thiz_, className_, false);
            node_ = nullptr;
        }
    }
    ~DfxClassHelper()
    {
        if (node_) {
            node_->UpdateClassTag(thiz_, className_, false);
        }
    }
private:
    std::string className_ = "";
    std::shared_ptr<DfxNode> node_ = nullptr;
    void *thiz_ = nullptr;
};

class DfxFuncHelper {
public:
    DfxFuncHelper(const std::string& func, const std::shared_ptr<DfxNode> &node) : func_(func), node_(node)
    {
        if (node_) {
            node_->UpdateFuncTag(func_, true);
        }
    }
    ~DfxFuncHelper()
    {
        if (node_) {
            node_->UpdateFuncTag(func_, false);
        }
    }
private:
    std::string func_ = ""; 
    std::shared_ptr<DfxNode> node_ = nullptr;   
};

template <typename T>
class DfxValHelper {
public:
    constexpr DfxValHelper() noexcept {};
    DfxValHelper(T &value) noexcept : value_(value) {};
    DfxValHelper(T &&value) noexcept : value_(value) {};
    DfxValHelper(const DfxValHelper<T> &dfx) noexcept {
        node_ = dfx.GetNode();
        name_ = dfx.GetName();
        value_ = dfx.GetValue();
    };

    inline std::shared_ptr<DfxNode> GetNode() const
    {
        return node_;
    }

    inline std::string GetName() const
    {
        return name_;
    }

    inline void Init(const std::shared_ptr<DfxNode> &node, const std::string &name)
    {
        node_ = node;
        name_ = name;
    }

    inline void operator=(const T &newValue)
    {
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, newValue);
        }
        value_ = newValue;
    }

    inline DfxValHelper<T> operator++()
    {
        ++value_;
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, value_);
        }
        return *this;
    }

    inline DfxValHelper<T> operator++(int32_t)
    {
        DfxValHelper<T> old = *this;
        value_++;
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, value_);
        }
        return old;
    }

    inline DfxValHelper<T> operator--()
    {
        --value_;
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, value_);
        }
        return *this;
    }

    inline DfxValHelper<T> operator--(int32_t)
    {
        DfxValHelper<T> old = *this;
        value_--;
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, value_);
        }
        return old;
    }

    DfxValHelper<T> operator-=(const T &other)
    {
        this->value_ -= other;
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, value_);
        }
        return *this;
    }

    DfxValHelper<T> operator+=(const T &other)
    {
        this->value_ += other;
        if (node_ != nullptr) {
            node_->UpdateValTag(name_, value_);
        }
        return *this;
    }

    inline T GetValue() const {
        return value_;
    };

private:
    T value_;
    std::shared_ptr<DfxNode> node_ = nullptr;
    std::string name_ = "";
};

template<typename T>
inline bool operator==(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() == right.GetValue();
}

template<typename T>
inline bool operator!=(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() != right.GetValue();
}

template<typename T>
inline bool operator<(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() < right.GetValue();
}

template<typename T>
inline bool operator<=(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() <= right.GetValue();
}

template<typename T>
inline bool operator>(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() > right.GetValue();
}

template<typename T>
inline bool operator>=(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() >= right.GetValue();
}

template<typename T>
inline T operator-(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() - right.GetValue();
}

template<typename T>
inline T operator+(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() + right.GetValue();
}

template<typename T>
inline T operator*(const DfxValHelper<T> &left, const DfxValHelper<T> &right)
{
    return left.GetValue() * right.GetValue();
}

template<typename T>
inline bool operator==(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() == right;
}

template<typename T>
inline bool operator!=(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() != right;
}

template<typename T>
inline bool operator<(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() < right;
}

template<typename T>
inline bool operator<=(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() <= right;
}

template<typename T>
inline bool operator>(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() > right;
}

template<typename T>
inline bool operator>=(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() >= right;
}

template<typename T>
inline T operator-(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() - right;
}

template<typename T>
inline T operator+(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() + right;
}

template<typename T>
inline T operator*(const DfxValHelper<T> &left, const T &right)
{
    return left.GetValue() * right;
}

template<typename T>
inline bool operator==(const T &left, const DfxValHelper<T> &right)
{
    return left == right.GetValue();
}

template<typename T>
inline bool operator!=(const T &left, const DfxValHelper<T> &right)
{
    return left != right.GetValue();
}

template<typename T>
inline bool operator<(const T &left, const DfxValHelper<T> &right)
{
    return left < right.GetValue();
}

template<typename T>
inline bool operator<=(const T &left, const DfxValHelper<T> &right)
{
    return left <= right.GetValue();
}

template<typename T>
inline bool operator>(const T &left, const DfxValHelper<T> &right)
{
    return left > right.GetValue();
}

template<typename T>
inline bool operator>=(const T &left, const DfxValHelper<T> &right)
{
    return left >= right.GetValue();
}

template<typename T>
inline T operator-(const T &left, const DfxValHelper<T> &right)
{
    return left - right.GetValue();
}

template<typename T>
inline T operator+(const T &left, const DfxValHelper<T> &right)
{
    return left + right.GetValue();
}

template<typename T>
inline T operator*(const T &left, const DfxValHelper<T> &right)
{
    return left * right.GetValue();
}
} // namespace Media
} // namespace OHOS

#endif