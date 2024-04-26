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

#ifndef COMMON_NAPI_H
#define COMMON_NAPI_H

#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "meta/format.h"
#include "meta/meta.h"
#include "av_common.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "media_core.h"
#include "audio_info.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace Media {
struct AVFileDescriptor;
struct AVPlayStrategyTmp;
struct AVDataSrcDescriptor;
class AVMediaSourceTmp;
/**
 * customInfo max count
*/
constexpr uint32_t MAX_COUNT = 500;
/**
 * NOTE: use on AVRecorderConfig.metadata.customInfo
*/
constexpr uint32_t CUSTOM_MAX_LENGTH = 1001;
class CommonNapi {
public:
    CommonNapi() = delete;
    ~CommonNapi() = delete;
    static std::string GetStringArgument(napi_env env, napi_value value, size_t maxLength = PATH_MAX);
    static bool CheckValueType(napi_env env, napi_value arg, napi_valuetype type);
    static bool CheckhasNamedProperty(napi_env env, napi_value arg, std::string type);
    static bool GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result);
    static bool GetPropertyUint32(napi_env env, napi_value configObj, const std::string &type, uint32_t &result);
    static bool GetPropertyInt64(napi_env env, napi_value configObj, const std::string &type, int64_t &result);
    static bool GetPropertyDouble(napi_env env, napi_value configObj, const std::string &type, double &result);
    static std::string GetPropertyString(napi_env env, napi_value configObj, const std::string &type);
    // support Record<string, string>
    static napi_status GetPropertyRecord(napi_env env, napi_value in, Meta &meta, std::string type);
    static bool GetPropertyMap(napi_env env, napi_value value, std::map<std::string, std::string>& map);
    static bool GetFdArgument(napi_env env, napi_value value, AVFileDescriptor &rawFd);
    static bool GetPlayStrategy(napi_env env, napi_value value, AVPlayStrategyTmp &playStrategy);
    static napi_status FillErrorArgs(napi_env env, int32_t errCode, const napi_value &args);
    static napi_status CreateError(napi_env env, int32_t errCode, const std::string &errMsg, napi_value &errVal);
    static napi_ref CreateReference(napi_env env, napi_value arg);
    static napi_deferred CreatePromise(napi_env env, napi_ref ref, napi_value &result);
    static bool SetPropertyByValueType(napi_env env, napi_value &obj, std::shared_ptr<Meta> &meta, std::string key);
    static bool SetPropertyInt32(napi_env env, napi_value &obj, const std::string &key, int32_t value);
    static bool SetPropertyDouble(napi_env env, napi_value &obj, const std::string &key, double value);
    static bool SetPropertyBool(napi_env env, napi_value &obj, const std::string &key, bool value);
    static bool SetPropertyString(napi_env env, napi_value &obj, const std::string &key, const std::string &value);
    static napi_value CreateFormatBuffer(napi_env env, Format &format);
    static bool CreateFormatBufferByRef(napi_env env, Format &format, napi_value &result);
    static bool AddRangeProperty(napi_env env, napi_value obj, const std::string &name, int32_t min, int32_t max);
    static bool AddArrayProperty(napi_env env, napi_value obj, const std::string &name,
        const std::vector<int32_t> &vec);
    static bool AddNumberPropInt32(napi_env env, napi_value obj, const std::string &key, int32_t value);
    static bool AddNumberPropInt64(napi_env env, napi_value obj, const std::string &key, int64_t value);
    static bool AddArrayInt(napi_env env, napi_value &array, const std::vector<int32_t> &vec);
    static bool AddStringProperty(napi_env env, napi_value obj, const std::string &key, const std::string &value);
    static bool GetPropertyBool(napi_env env, napi_value configObj, const std::string &type, bool &result);

    static void ConvertDeviceInfoToAudioDeviceDescriptor(
        sptr<AudioStandard::AudioDeviceDescriptor> audioDeviceDescriptor, const AudioStandard::DeviceInfo &deviceInfo);
    static napi_status SetValueDeviceInfo(const napi_env &env, const AudioStandard::DeviceInfo &deviceInfo,
        napi_value &result);
    static napi_status SetDeviceDescriptor(const napi_env &env, const AudioStandard::AudioDeviceDescriptor &deviceInfo,
        napi_value &result);
    static napi_status SetDeviceDescriptors(const napi_env &env,
        const std::vector<sptr<AudioStandard::AudioDeviceDescriptor>> &deviceDescriptors, napi_value &result);
};

class MediaJsResult {
public:
    virtual ~MediaJsResult() = default;
    virtual napi_status GetJsResult(napi_env env, napi_value &result) = 0;
};

class MediaJsResultBoolean : public MediaJsResult {
public:
    explicit MediaJsResultBoolean(bool value)
        : value_(value)
    {
    }
    ~MediaJsResultBoolean() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        return napi_get_boolean(env, value_, &result);
    }
private:
    bool value_;
};

class MediaJsResultInt : public MediaJsResult {
public:
    explicit MediaJsResultInt(int32_t value)
        : value_(value)
    {
    }
    ~MediaJsResultInt() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        return napi_create_int32(env, value_, &result);
    }
private:
    int32_t value_;
};

class MediaJsResultString : public MediaJsResult {
public:
    explicit MediaJsResultString(const std::string &value)
        : value_(value)
    {
    }
    ~MediaJsResultString() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        return napi_create_string_utf8(env, value_.c_str(), NAPI_AUTO_LENGTH, &result);
    }

private:
    std::string value_;
};

class MediaJsResultStringVector : public MediaJsResult {
public:
    explicit MediaJsResultStringVector(const std::vector<std::string> &value)
        : value_(value)
    {
    }
    ~MediaJsResultStringVector() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<std::string> value_;
};

class MediaJsResultArray : public MediaJsResult {
public:
    explicit MediaJsResultArray(const std::vector<Format> &value)
        : value_(value)
    {
    }
    ~MediaJsResultArray() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override;

private:
    std::vector<Format> value_;
};

class MediaJsResultRange : public MediaJsResult {
public:
    explicit MediaJsResultRange(int32_t min, int32_t max)
        : min_(min),
          max_(max)
    {
    }
    ~MediaJsResultRange() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        napi_status status = napi_create_object(env, &result);
        if (status != napi_ok) {
            return status;
        }

        if (!CommonNapi::SetPropertyInt32(env, result, "min", min_)) {
            return napi_invalid_arg;
        }

        if (!CommonNapi::SetPropertyInt32(env, result, "max", max_)) {
            return napi_invalid_arg;
        }

        return napi_ok;
    }
private:
    int32_t min_;
    int32_t max_;
};

class MediaJsResultInstance : public MediaJsResult {
public:
    explicit MediaJsResultInstance(const napi_ref &constructor)
        : constructor_(constructor)
    {
    }
    ~MediaJsResultInstance() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        napi_value constructor = nullptr;
        napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
        if (ret != napi_ok || constructor == nullptr) {
            return ret;
        }
        return napi_new_instance(env, constructor, 0, nullptr, &result);
    }

private:
    napi_ref constructor_;
};

class AVCodecJsResultCtor : public MediaJsResult {
public:
    AVCodecJsResultCtor(const napi_ref &constructor, int32_t isMimeType, const std::string &name)
        : constructor_(constructor),
          isMimeType_(isMimeType),
          name_(name)
    {
    }
    ~AVCodecJsResultCtor() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        napi_value constructor = nullptr;
        napi_status ret = napi_get_reference_value(env, constructor_, &constructor);
        if (ret != napi_ok || constructor == nullptr) {
            return ret;
        }

        napi_value args[2] = { nullptr };
        ret = napi_create_string_utf8(env, name_.c_str(), NAPI_AUTO_LENGTH, &args[0]);
        if (ret != napi_ok) {
            return ret;
        }

        ret = napi_create_int32(env, isMimeType_, &args[1]);
        if (ret != napi_ok) {
            return ret;
        }

        return napi_new_instance(env, constructor, 2, args, &result); // The number of parameters is 2
    }

private:
    napi_ref constructor_;
    int32_t isMimeType_ = 0;
    std::string name_ = "";
};

class AVCodecJsResultFormat : public MediaJsResult {
public:
    explicit AVCodecJsResultFormat(const Format &format)
        : format_(format)
    {
    }
    ~AVCodecJsResultFormat() = default;
    napi_status GetJsResult(napi_env env, napi_value &result) override
    {
        (void)CommonNapi::CreateFormatBufferByRef(env, format_, result);
        return napi_ok;
    }

private:
    Format format_;
};

struct MediaAsyncContext {
    explicit MediaAsyncContext(napi_env env);
    virtual ~MediaAsyncContext();
    static void CompleteCallback(napi_env env, napi_status status, void *data);
    static void Callback(napi_env env, const MediaAsyncContext *context, const napi_value *args);
    static void CheckCtorResult(napi_env env, napi_value &result, MediaAsyncContext *ctx, napi_value &args);
    void SignError(int32_t code, const std::string &message, bool del = true);
    std::string memoryTagHead = "safe";
    napi_env env_;
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callbackRef = nullptr;
    std::unique_ptr<MediaJsResult> JsResult;
    bool errFlag = false;
    int32_t errCode = 0;
    std::string errMessage = "";
    bool delFlag = true;
    bool ctorFlag = false;
    std::string memoryTagTail = "memory";
};

struct AutoRef {
    AutoRef(napi_env env, napi_ref cb)
        : env_(env), cb_(cb)
    {
    }
    ~AutoRef()
    {
        if (env_ != nullptr && cb_ != nullptr) {
            (void)napi_delete_reference(env_, cb_);
        }
    }
    napi_env env_;
    napi_ref cb_;
};

struct AVDataSrcDescriptor {
    int64_t fileSize = 0;
    napi_value callback = nullptr;
};

class AVMediaSourceTmp {
public:
    AVMediaSourceTmp() = default;
    ~AVMediaSourceTmp()
    {
        header.clear();
    }
    std::map<std::string, std::string> header;
    std::string url {0};
};

struct AVPlayStrategyTmp {
    uint32_t preferredWidth;
    uint32_t preferredHeight;
    uint32_t preferredBufferDuration;
    bool preferredHdr;
};

template<typename T>
class ObjectRefMap {
public:
    static std::mutex allObjLock;
    static std::map<T*, uint32_t> refMap;
    static void Insert(T *obj);
    static void Erase(T *obj);
    static T *IncreaseRef(T *obj);
    static void DecreaseRef(T *obj);

    ObjectRefMap(T *obj);
    ~ObjectRefMap();
    T *GetPtr();

private:
    T *obj_ = nullptr;
};

template <typename T>
std::mutex ObjectRefMap<T>::allObjLock;

template <typename T>
std::map<T *, uint32_t> ObjectRefMap<T>::refMap;

template <typename T>
void ObjectRefMap<T>::Insert(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    refMap[obj] = 1;
}

template <typename T>
void ObjectRefMap<T>::Erase(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    auto it = refMap.find(obj);
    if (it != refMap.end()) {
        refMap.erase(it);
    }
}

template <typename T>
T *ObjectRefMap<T>::IncreaseRef(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    if (refMap.count(obj)) {
        refMap[obj]++;
        return obj;
    } else {
        return nullptr;
    }
}

template <typename T>
void ObjectRefMap<T>::DecreaseRef(T *obj)
{
    std::lock_guard<std::mutex> lock(allObjLock);
    if (refMap.count(obj) && --refMap[obj] == 0) {
        refMap.erase(obj);
        delete obj;
        obj = nullptr;
    }
}

template <typename T>
ObjectRefMap<T>::ObjectRefMap(T *obj)
{
    if (obj != nullptr) {
        obj_ = ObjectRefMap::IncreaseRef(obj);
    }
}

template <typename T>
ObjectRefMap<T>::~ObjectRefMap()
{
    if (obj_ != nullptr) {
        ObjectRefMap::DecreaseRef(obj_);
    }
}

template <typename T>
T *ObjectRefMap<T>::GetPtr()
{
    return obj_;
}

} // namespace Media
} // namespace OHOS
#endif // COMMON_NAPI_H
