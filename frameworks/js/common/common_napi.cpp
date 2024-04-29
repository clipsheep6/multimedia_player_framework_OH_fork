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

#include "common_napi.h"
#include <climits>
#include "avcodec_list.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CommonNapi"};
}

namespace OHOS {
namespace Media {
std::string CommonNapi::GetStringArgument(napi_env env, napi_value value, size_t maxLength)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < maxLength) {
        char *buffer = static_cast<char *>(malloc((bufLength + 1) * sizeof(char)));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            MEDIA_LOGD("argument = %{public}s", buffer);
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

bool CommonNapi::CheckValueType(napi_env env, napi_value arg, napi_valuetype type)
{
    napi_valuetype valueType = napi_undefined;
    if (arg != nullptr && napi_typeof(env, arg, &valueType) == napi_ok && valueType == type) {
        return true;
    }
    return false;
}

bool CommonNapi::CheckhasNamedProperty(napi_env env, napi_value arg, std::string type)
{
    bool exist = false;
    napi_status napiStatus = napi_has_named_property(env, arg, type.c_str(), &exist);
    return exist && (napiStatus == napi_ok);
}

bool CommonNapi::GetPropertyInt32(napi_env env, napi_value configObj, const std::string &type, int32_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status napiStatus = napi_has_named_property(env, configObj, type.c_str(), &exist);
    CHECK_AND_RETURN_RET_LOG(napiStatus == napi_ok && exist, false, "can not find %{public}s property", type.c_str());
    CHECK_AND_RETURN_RET_LOG(napi_get_named_property(env, configObj, type.c_str(), &item) == napi_ok, false,
        "get %{public}s property fail", type.c_str());
    CHECK_AND_RETURN_RET_LOG(napi_get_value_int32(env, item, &result) == napi_ok, false,
        "get %{public}s property value fail", type.c_str());
    return true;
}

bool CommonNapi::GetPropertyUint32(napi_env env, napi_value configObj, const std::string &type, uint32_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && exist, false, "can not find %{public}s property", type.c_str());
    CHECK_AND_RETURN_RET_LOG(napi_get_named_property(env, configObj, type.c_str(), &item) == napi_ok, false,
        "get %{public}s property fail", type.c_str());

    CHECK_AND_RETURN_RET_LOG(napi_get_value_uint32(env, item, &result) == napi_ok, false,
        "get %{public}s property value fail", type.c_str());
    return true;
}

bool CommonNapi::GetPropertyInt64(napi_env env, napi_value configObj, const std::string &type, int64_t &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return false;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return false;
    }

    if (napi_get_value_int64(env, item, &result) != napi_ok) {
        MEDIA_LOGE("get %{public}s property value fail", type.c_str());
        return false;
    }
    return true;
}

bool CommonNapi::GetPropertyDouble(napi_env env, napi_value configObj, const std::string &type, double &result)
{
    napi_value item = nullptr;
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return false;
    }

    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return false;
    }

    if (napi_get_value_double(env, item, &result) != napi_ok) {
        MEDIA_LOGE("get %{public}s property value fail", type.c_str());
        return false;
    }
    return true;
}

std::string CommonNapi::GetPropertyString(napi_env env, napi_value configObj, const std::string &type)
{
    std::string invalid = "";
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return invalid;
    }

    napi_value item = nullptr;
    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return invalid;
    }

    return GetStringArgument(env, item);
}

napi_status CommonNapi::GetPropertyRecord(napi_env env, napi_value configObj, Meta &meta, std::string type)
{
    bool exist = false;
    napi_value in = nullptr;
    napi_valuetype valueType = napi_undefined;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return napi_invalid_arg;
    }
    if (napi_get_named_property(env, configObj, type.c_str(), &in) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return napi_invalid_arg;
    }
    status = napi_typeof(env, in, &valueType);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "get valueType failed");
    CHECK_AND_RETURN_RET_LOG(valueType != napi_undefined, napi_ok, "PropertyRecord undefined");
    CHECK_AND_RETURN_RET_LOG(valueType == napi_object, napi_invalid_arg, "invalid arguments");

    napi_value dataList = nullptr;
    uint32_t count = 0;
    status = napi_get_property_names(env, in, &dataList);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "get property names failed");
    status = napi_get_array_length(env, dataList, &count);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && count <= MAX_COUNT,
        napi_invalid_arg, "get length failed or more than 500");

    napi_value jsKey = nullptr;
    napi_value jsValue = nullptr;
    for (uint32_t i = 0; i < count; i++) {
        status = napi_get_element(env, dataList, i, &jsKey);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "get element Key failed");
        status = napi_typeof(env, jsKey, &valueType);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "get valueType failed");
        CHECK_AND_RETURN_RET_LOG(valueType == napi_string, napi_invalid_arg, "key not supported type");
        std::string strKey = GetStringArgument(env, jsKey, CUSTOM_MAX_LENGTH);
        CHECK_AND_RETURN_RET_LOG(strKey != "", napi_invalid_arg, "key not supported");

        status = napi_get_named_property(env, in, strKey.c_str(), &jsValue);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "get property value failed");
        status = napi_typeof(env, jsValue, &valueType);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status, "get valueType failed");

        CHECK_AND_RETURN_RET_LOG(valueType == napi_string, napi_invalid_arg, "value not supported type");
        std::string strValue = GetStringArgument(env, jsValue, CUSTOM_MAX_LENGTH);
        CHECK_AND_RETURN_RET_LOG(!strValue.empty(), napi_invalid_arg, "get value failed");
        meta.SetData(strKey, strValue);
    }
    return napi_ok;
}

bool CommonNapi::GetFdArgument(napi_env env, napi_value value, AVFileDescriptor &rawFd)
{
    CHECK_AND_RETURN_RET(GetPropertyInt32(env, value, "fd", rawFd.fd) == true, false);

    if (!GetPropertyInt64(env, value, "offset", rawFd.offset)) {
        rawFd.offset = 0; // use default value
    }

    if (!GetPropertyInt64(env, value, "length", rawFd.length)) {
        rawFd.length = -1; // -1 means use default value
    }

    MEDIA_LOGD("get fd argument, fd = %{public}d, offset = %{public}" PRIi64 ", size = %{public}" PRIi64 "",
        rawFd.fd, rawFd.offset, rawFd.length);

    return true;
}

bool CommonNapi::GetPropertyMap(napi_env env, napi_value value, std::map<std::string, std::string>& map)
{
    napi_value jsProNameList = nullptr;
    uint32_t jsProCount = 0;
    napi_status status = napi_get_property_names(env, value, &jsProNameList);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "get property name failed");
    status = napi_get_array_length(env, jsProNameList, &jsProCount);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "get subKeys length failed");

    napi_value jsProName = nullptr;
    napi_value jsProValue = nullptr;
    for (uint32_t i = 0; i < jsProCount; i++) {
        status = napi_get_element(env, jsProNameList, i, &jsProName);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "get sub key failed");
        std::string strProName = GetStringArgument(env, jsProName);

        status = napi_get_named_property(env, value, strProName.c_str(), &jsProValue);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "get sub value failed");
        std::string strProValue = GetStringArgument(env, jsProValue);

        map.emplace(strProName, strProValue);
    }

    return true;
}

bool CommonNapi::GetPlayStrategy(napi_env env, napi_value value, AVPlayStrategyTmp &playStrategy)
{
    if (!GetPropertyUint32(env, value, "preferredWidth", playStrategy.preferredWidth)) {
        playStrategy.preferredWidth = 0; // use default value
    }
    if (!GetPropertyUint32(env, value, "preferredHeight", playStrategy.preferredHeight)) {
        playStrategy.preferredHeight = 0; // use default value
    }
    if (!GetPropertyUint32(env, value, "preferredBufferDuration", playStrategy.preferredBufferDuration)) {
        playStrategy.preferredBufferDuration = 0; // use default value
    }
    if (!GetPropertyBool(env, value, "preferredHdr", playStrategy.preferredHdr)) {
        playStrategy.preferredHdr = 0; // use default value
    }
    return true;
}

napi_status CommonNapi::FillErrorArgs(napi_env env, int32_t errCode, const napi_value &args)
{
    napi_value codeStr = nullptr;
    napi_status status = napi_create_string_utf8(env, "code", NAPI_AUTO_LENGTH, &codeStr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && codeStr != nullptr, napi_invalid_arg, "create code str fail");

    napi_value errCodeVal = nullptr;
    int32_t errCodeInt = errCode;
    status = napi_create_int32(env, errCodeInt, &errCodeVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && errCodeVal != nullptr, napi_invalid_arg,
        "create error code number val fail");

    status = napi_set_property(env, args, codeStr, errCodeVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, napi_invalid_arg, "set error code property fail");

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, "name", NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && nameStr != nullptr, napi_invalid_arg, "create name str fail");

    napi_value errNameVal = nullptr;
    status = napi_create_string_utf8(env, "BusinessError", NAPI_AUTO_LENGTH, &errNameVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && errNameVal != nullptr, napi_invalid_arg,
        "create BusinessError str fail");

    status = napi_set_property(env, args, nameStr, errNameVal);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, napi_invalid_arg, "set error name property fail");
    return napi_ok;
}

napi_status CommonNapi::CreateError(napi_env env, int32_t errCode, const std::string &errMsg, napi_value &errVal)
{
    napi_get_undefined(env, &errVal);

    napi_value msgValStr = nullptr;
    napi_status nstatus = napi_create_string_utf8(env, errMsg.c_str(), NAPI_AUTO_LENGTH, &msgValStr);
    if (nstatus != napi_ok || msgValStr == nullptr) {
        MEDIA_LOGE("create error message str fail");
        return napi_invalid_arg;
    }

    nstatus = napi_create_error(env, nullptr, msgValStr, &errVal);
    if (nstatus != napi_ok || errVal == nullptr) {
        MEDIA_LOGE("create error fail");
        return napi_invalid_arg;
    }

    napi_value codeStr = nullptr;
    nstatus = napi_create_string_utf8(env, "code", NAPI_AUTO_LENGTH, &codeStr);
    if (nstatus != napi_ok || codeStr == nullptr) {
        MEDIA_LOGE("create code str fail");
        return napi_invalid_arg;
    }

    napi_value errCodeVal = nullptr;
    nstatus = napi_create_int32(env, errCode, &errCodeVal);
    if (nstatus != napi_ok || errCodeVal == nullptr) {
        MEDIA_LOGE("create error code number val fail");
        return napi_invalid_arg;
    }

    nstatus = napi_set_property(env, errVal, codeStr, errCodeVal);
    if (nstatus != napi_ok) {
        MEDIA_LOGE("set error code property fail");
        return napi_invalid_arg;
    }

    napi_value nameStr = nullptr;
    nstatus = napi_create_string_utf8(env, "name", NAPI_AUTO_LENGTH, &nameStr);
    if (nstatus != napi_ok || nameStr == nullptr) {
        MEDIA_LOGE("create name str fail");
        return napi_invalid_arg;
    }

    napi_value errNameVal = nullptr;
    nstatus = napi_create_string_utf8(env, "BusinessError", NAPI_AUTO_LENGTH, &errNameVal);
    if (nstatus != napi_ok || errNameVal == nullptr) {
        MEDIA_LOGE("create BusinessError str fail");
        return napi_invalid_arg;
    }

    nstatus = napi_set_property(env, errVal, nameStr, errNameVal);
    if (nstatus != napi_ok) {
        MEDIA_LOGE("set error name property fail");
        return napi_invalid_arg;
    }

    return napi_ok;
}

napi_ref CommonNapi::CreateReference(napi_env env, napi_value arg)
{
    napi_ref ref = nullptr;
    napi_valuetype valueType = napi_undefined;
    if (arg != nullptr && napi_typeof(env, arg, &valueType) == napi_ok && valueType == napi_function) {
        MEDIA_LOGD("napi_create_reference");
        napi_create_reference(env, arg, 1, &ref);
    }
    return ref;
}

napi_deferred CommonNapi::CreatePromise(napi_env env, napi_ref ref, napi_value &result)
{
    napi_deferred deferred = nullptr;
    if (ref == nullptr) {
        MEDIA_LOGD("napi_create_promise");
        napi_create_promise(env, &deferred, &result);
    }
    return deferred;
}

bool CommonNapi::SetPropertyByValueType(napi_env env, napi_value &obj, std::shared_ptr<Meta> &meta, std::string key)
{
    CHECK_AND_RETURN_RET(obj != nullptr && meta != nullptr, false);
    CHECK_AND_RETURN_RET(meta->Find(key) != meta->end(), false);

    bool ret = true;
    AnyValueType type = meta->GetValueType(key);
    if (type == AnyValueType::STRING) {
        std::string sValue;
        ret = meta->GetData(key, sValue);
        CHECK_AND_RETURN_RET_LOG(ret, ret, "GetData failed, key %{public}s", key.c_str());
        ret = CommonNapi::SetPropertyString(env, obj, key, sValue);
        CHECK_AND_RETURN_RET_LOG(ret, ret, "SetPropertyString failed, key %{public}s", key.c_str());
    } else if (type == AnyValueType::INT32_T) {
        int32_t value;
        ret = meta->GetData(key, value);
        CHECK_AND_RETURN_RET_LOG(ret, ret, "GetData failed, key %{public}s", key.c_str());
        ret = CommonNapi::SetPropertyInt32(env, obj, key, value);
        CHECK_AND_RETURN_RET_LOG(ret, ret, "SetPropertyString failed, key %{public}s", key.c_str());
    } else if (type == AnyValueType::FLOAT) {
        float dValue;
        ret = meta->GetData(key, dValue);
        CHECK_AND_RETURN_RET_LOG(ret, ret, "GetData failed, key %{public}s", key.c_str());
        ret = CommonNapi::SetPropertyDouble(env, obj, key, dValue);
        CHECK_AND_RETURN_RET_LOG(ret, ret, "SetPropertyString failed, key %{public}s", key.c_str());
    } else {
        MEDIA_LOGE("not supported value type");
    }
    return true;
}

bool CommonNapi::AddRangeProperty(napi_env env, napi_value obj, const std::string &name, int32_t min, int32_t max)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value range = nullptr;
    napi_status status = napi_create_object(env, &range);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    CHECK_AND_RETURN_RET(SetPropertyInt32(env, range, "min", min) == true, false);
    CHECK_AND_RETURN_RET(SetPropertyInt32(env, range, "max", max) == true, false);

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, nameStr, range);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    return true;
}

bool CommonNapi::AddArrayProperty(napi_env env, napi_value obj, const std::string &name,
    const std::vector<int32_t> &vec)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value array = nullptr;
    napi_status status = napi_create_array_with_length(env, vec.size(), &array);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    for (uint32_t i = 0; i < vec.size(); i++) {
        napi_value number = nullptr;
        (void)napi_create_int32(env, vec.at(i), &number);
        (void)napi_set_element(env, array, i, number);
    }

    napi_value nameStr = nullptr;
    status = napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &nameStr);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, nameStr, array);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    return true;
}

bool CommonNapi::AddArrayInt(napi_env env, napi_value &array, const std::vector<int32_t> &vec)
{
    if (vec.size() == 0) {
        return false;
    }

    napi_status status = napi_create_array_with_length(env, vec.size(), &array);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    for (uint32_t i = 0; i < vec.size(); i++) {
        napi_value number = nullptr;
        (void)napi_create_int32(env, vec.at(i), &number);
        (void)napi_set_element(env, array, i, number);
    }

    return true;
}

bool CommonNapi::SetPropertyInt32(napi_env env, napi_value &obj, const std::string &key, int32_t value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_int32(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "failed to set property");

    return true;
}

bool CommonNapi::SetPropertyDouble(napi_env env, napi_value &obj, const std::string &key, double value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_double(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "failed to set property");

    return true;
}

bool CommonNapi::SetPropertyBool(napi_env env, napi_value &obj, const std::string &key, bool value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_get_boolean(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "failed to set property");

    return true;
}

bool CommonNapi::SetPropertyString(napi_env env, napi_value &obj, const std::string &key, const std::string &value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "failed to set property");

    return true;
}

napi_value CommonNapi::CreateFormatBuffer(napi_env env, Format &format)
{
    napi_value buffer = nullptr;
    int32_t intValue = 0;
    std::string strValue;
    napi_status status = napi_create_object(env, &buffer);
    CHECK_AND_RETURN_RET(status == napi_ok, nullptr);

    for (auto &iter : format.GetFormatMap()) {
        switch (format.GetValueType(std::string_view(iter.first))) {
            case FORMAT_TYPE_INT32:
                if (format.GetIntValue(iter.first, intValue)) {
                    CHECK_AND_RETURN_RET(SetPropertyInt32(env, buffer, iter.first, intValue) == true, nullptr);
                }
                break;
            case FORMAT_TYPE_INT64:
                int64_t longValue;
                if (format.GetLongValue(iter.first, longValue) &&
                    longValue >= INT32_MIN && longValue <= INT32_MAX) {
                    intValue = static_cast<int32_t>(longValue);
                    CHECK_AND_RETURN_RET(SetPropertyInt32(env, buffer, iter.first, intValue) == true, nullptr);
                }
                break;
            case FORMAT_TYPE_DOUBLE:
                double doubleValue;
                if (format.GetDoubleValue(iter.first, doubleValue) &&
                    doubleValue >= INT32_MIN && doubleValue <= INT32_MAX) {
                    intValue = static_cast<int32_t>(doubleValue);
                    CHECK_AND_RETURN_RET(SetPropertyInt32(env, buffer, iter.first, intValue) == true, nullptr);
                }
                break;
            case FORMAT_TYPE_STRING:
                if (format.GetStringValue(iter.first, strValue)) {
                    CHECK_AND_RETURN_RET(SetPropertyString(env, buffer, iter.first, strValue) == true, nullptr);
                }
                break;
            default:
                MEDIA_LOGE("format key: %{public}s", iter.first.c_str());
                break;
        }
    }

    return buffer;
}

bool CommonNapi::CreateFormatBufferByRef(napi_env env, Format &format, napi_value &result)
{
    int32_t intValue = 0;
    std::string strValue = "";
    napi_status status = napi_create_object(env, &result);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    for (auto &iter : format.GetFormatMap()) {
        switch (format.GetValueType(std::string_view(iter.first))) {
            case FORMAT_TYPE_INT32:
                if (format.GetIntValue(iter.first, intValue)) {
                    (void)SetPropertyInt32(env, result, iter.first, intValue);
                }
                break;
            case FORMAT_TYPE_STRING:
                if (format.GetStringValue(iter.first, strValue)) {
                    (void)SetPropertyString(env, result, iter.first, strValue);
                }
                break;
            default:
                MEDIA_LOGE("format key: %{public}s", iter.first.c_str());
                break;
        }
    }

    return true;
}

bool CommonNapi::AddNumberPropInt32(napi_env env, napi_value obj, const std::string &key, int32_t value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_int32(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "Failed to set property");

    return true;
}

bool CommonNapi::AddNumberPropInt64(napi_env env, napi_value obj, const std::string &key, int64_t value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_int64(env, value, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "Failed to set property");

    return true;
}

napi_status MediaJsResultStringVector::GetJsResult(napi_env env, napi_value &result)
{
    napi_status status;
    size_t size = value_.size();
    napi_create_array_with_length(env, size, &result);
    for (unsigned int i = 0; i < size; ++i) {
        std::string format = value_[i];
        napi_value value = nullptr;
        status = napi_create_string_utf8(env, format.c_str(), NAPI_AUTO_LENGTH, &value);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status,
            "Failed to call napi_create_string_utf8, with element %{public}u", i);
        status = napi_set_element(env, result, i, value);
        CHECK_AND_RETURN_RET_LOG(status == napi_ok, status,
            "Failed to call napi_set_element, with element %{public}u", i);
    }
    return napi_ok;
}

napi_status MediaJsResultArray::GetJsResult(napi_env env, napi_value &result)
{
    // create Description
    napi_status status = napi_create_array(env, &result);
    if (status != napi_ok) {
        return napi_cancelled;
    }

    auto vecSize = value_.size();
    for (size_t index = 0; index < vecSize; ++index) {
        napi_value description = nullptr;
        description = CommonNapi::CreateFormatBuffer(env, value_[index]);
        if (description == nullptr || napi_set_element(env, result, index, description) != napi_ok) {
            return napi_cancelled;
        }
    }
    return napi_ok;
}

MediaAsyncContext::MediaAsyncContext(napi_env env)
    : env_(env)
{
    MEDIA_LOGD("MediaAsyncContext Create 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

MediaAsyncContext::~MediaAsyncContext()
{
    MEDIA_LOGD("MediaAsyncContext Destroy 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

void MediaAsyncContext::SignError(int32_t code, const std::string &message, bool del)
{
    errMessage = message;
    errCode = code;
    errFlag = true;
    delFlag = del;
    MEDIA_LOGE("SignError: %{public}s", message.c_str());
}

void MediaAsyncContext::CompleteCallback(napi_env env, napi_status status, void *data)
{
    MEDIA_LOGD("CompleteCallback In");
    auto asyncContext = reinterpret_cast<MediaAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "asyncContext is nullptr!");

    std::string memoryTag = asyncContext->memoryTagHead + asyncContext->memoryTagTail;
    MEDIA_LOGD("MediaAsyncContext Create 0x%{public}06" PRIXPTR " memoryTag = %{public}s",
        FAKE_POINTER(data), memoryTag.c_str());

    if (status != napi_ok) {
        asyncContext->SignError(MSERR_EXT_UNKNOWN, "napi_create_async_work status != napi_ok");
    }

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    napi_value args[2] = { nullptr };
    napi_get_undefined(env, &args[0]);
    napi_get_undefined(env, &args[1]);
    if (asyncContext->errFlag) {
        MEDIA_LOGD("async callback failed");
        (void)CommonNapi::CreateError(env, asyncContext->errCode, asyncContext->errMessage, result);
        args[0] = result;
    } else {
        MEDIA_LOGD("async callback success");
        if (asyncContext->JsResult != nullptr) {
            asyncContext->JsResult->GetJsResult(env, result);
            CheckCtorResult(env, result, asyncContext, args[0]);
        }
        if (!asyncContext->errFlag) {
            args[1] = result;
        }
    }

    Callback(env, asyncContext, args);
    napi_delete_async_work(env, asyncContext->work);

    if (asyncContext->delFlag) {
        delete asyncContext;
        asyncContext = nullptr;
    }
}

void MediaAsyncContext::Callback(napi_env env, const MediaAsyncContext *context, const napi_value *args)
{
    if (context->deferred) {
        if (context->errFlag) {
            MEDIA_LOGE("promise napi_reject_deferred");
            napi_reject_deferred(env, context->deferred, args[0]);
        } else {
            MEDIA_LOGD("promise napi_resolve_deferred");
            napi_resolve_deferred(env, context->deferred, args[1]);
        }
    } else if (context->callbackRef != nullptr) {
        MEDIA_LOGD("callback napi_call_function");
        napi_value callback = nullptr;
        napi_get_reference_value(env, context->callbackRef, &callback);
        CHECK_AND_RETURN_LOG(callback != nullptr, "callback is nullptr!");
        constexpr size_t argCount = 2;
        napi_value retVal;
        napi_get_undefined(env, &retVal);
        napi_call_function(env, nullptr, callback, argCount, args, &retVal);
        napi_delete_reference(env, context->callbackRef);
    } else {
        MEDIA_LOGE("invalid promise and callback");
    }
}

void MediaAsyncContext::CheckCtorResult(napi_env env, napi_value &result, MediaAsyncContext *ctx, napi_value &args)
{
    CHECK_AND_RETURN(ctx != nullptr);
    if (ctx->ctorFlag) {
        void *instance = nullptr;
        if (napi_unwrap(env, result, reinterpret_cast<void **>(&instance)) != napi_ok || instance == nullptr) {
            MEDIA_LOGE("Failed to create instance");
            ctx->errFlag = true;
            (void)CommonNapi::CreateError(env, MSERR_EXT_API9_NO_MEMORY,
                "The instance or memory has reached the upper limit, please recycle background playback", result);
            args = result;
        }
    }
}

bool CommonNapi::AddStringProperty(napi_env env, napi_value obj, const std::string &key, const std::string &value)
{
    CHECK_AND_RETURN_RET(obj != nullptr, false);

    napi_value keyNapi = nullptr;
    napi_status status = napi_create_string_utf8(env, key.c_str(), NAPI_AUTO_LENGTH, &keyNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    napi_value valueNapi = nullptr;
    status = napi_create_string_utf8(env, value.c_str(), NAPI_AUTO_LENGTH, &valueNapi);
    CHECK_AND_RETURN_RET(status == napi_ok, false);

    status = napi_set_property(env, obj, keyNapi, valueNapi);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, false, "Failed to set property");

    return true;
}

bool CommonNapi::GetPropertyBool(napi_env env, napi_value configObj, const std::string &type, bool &result)
{
    bool exist = false;
    napi_status status = napi_has_named_property(env, configObj, type.c_str(), &exist);
    if (status != napi_ok || !exist) {
        MEDIA_LOGE("can not find %{public}s property", type.c_str());
        return false;
    }
    napi_value item = nullptr;
    if (napi_get_named_property(env, configObj, type.c_str(), &item) != napi_ok) {
        MEDIA_LOGE("get %{public}s property fail", type.c_str());
        return false;
    }
    if (napi_get_value_bool(env, item, &result) != napi_ok) {
        MEDIA_LOGE("get %{public}s property value fail", type.c_str());
        return false;
    }
    return true;
}

void CommonNapi::ConvertDeviceInfoToAudioDeviceDescriptor(
    sptr<AudioStandard::AudioDeviceDescriptor> audioDeviceDescriptor, const AudioStandard::DeviceInfo &deviceInfo)
{
    CHECK_AND_RETURN_LOG(audioDeviceDescriptor != nullptr, "audioDeviceDescriptor is nullptr");
    audioDeviceDescriptor->deviceRole_ = deviceInfo.deviceRole;
    audioDeviceDescriptor->deviceType_ = deviceInfo.deviceType;
    audioDeviceDescriptor->deviceId_ = deviceInfo.deviceId;
    audioDeviceDescriptor->channelMasks_ = deviceInfo.channelMasks;
    audioDeviceDescriptor->channelIndexMasks_ = deviceInfo.channelIndexMasks;
    audioDeviceDescriptor->deviceName_ = deviceInfo.deviceName;
    audioDeviceDescriptor->macAddress_ = deviceInfo.macAddress;
    audioDeviceDescriptor->interruptGroupId_ = deviceInfo.interruptGroupId;
    audioDeviceDescriptor->volumeGroupId_ = deviceInfo.volumeGroupId;
    audioDeviceDescriptor->networkId_ = deviceInfo.networkId;
    audioDeviceDescriptor->displayName_ = deviceInfo.displayName;
    audioDeviceDescriptor->audioStreamInfo_.samplingRate = deviceInfo.audioStreamInfo.samplingRate;
    audioDeviceDescriptor->audioStreamInfo_.encoding = deviceInfo.audioStreamInfo.encoding;
    audioDeviceDescriptor->audioStreamInfo_.format = deviceInfo.audioStreamInfo.format;
    audioDeviceDescriptor->audioStreamInfo_.channels = deviceInfo.audioStreamInfo.channels;
}

napi_status CommonNapi::SetDeviceDescriptor(const napi_env &env, const AudioStandard::AudioDeviceDescriptor &deviceInfo,
    napi_value &result)
{
    (void)napi_create_object(env, &result);
    SetPropertyInt32(env, result, "deviceRole", static_cast<int32_t>(deviceInfo.deviceRole_));
    SetPropertyInt32(env, result, "deviceType", static_cast<int32_t>(deviceInfo.deviceType_));
    SetPropertyInt32(env, result, "id", static_cast<int32_t>(deviceInfo.deviceId_));
    SetPropertyString(env, result, "name", deviceInfo.deviceName_);
    SetPropertyString(env, result, "address", deviceInfo.macAddress_);
    SetPropertyString(env, result, "networkId", deviceInfo.networkId_);
    SetPropertyString(env, result, "displayName", deviceInfo.displayName_);
    SetPropertyInt32(env, result, "interruptGroupId", static_cast<int32_t>(deviceInfo.interruptGroupId_));
    SetPropertyInt32(env, result, "volumeGroupId", static_cast<int32_t>(deviceInfo.volumeGroupId_));

    napi_value value = nullptr;
    napi_value sampleRates;
    size_t size = deviceInfo.audioStreamInfo_.samplingRate.size();
    napi_create_array_with_length(env, size, &sampleRates);
    size_t count = 0;
    for (const auto &samplingRate : deviceInfo.audioStreamInfo_.samplingRate) {
        napi_create_int32(env, samplingRate, &value);
        napi_set_element(env, sampleRates, count, value);
        count++;
    }
    napi_set_named_property(env, result, "sampleRates", sampleRates);

    napi_value channelCounts;
    size = deviceInfo.audioStreamInfo_.channels.size();
    napi_create_array_with_length(env, size, &channelCounts);
    count = 0;
    for (const auto &channels : deviceInfo.audioStreamInfo_.channels) {
        napi_create_int32(env, channels, &value);
        napi_set_element(env, channelCounts, count, value);
        count++;
    }
    napi_set_named_property(env, result, "channelCounts", channelCounts);

    std::vector<int32_t> channelMasks_;
    channelMasks_.push_back(deviceInfo.channelMasks_);
    AddArrayProperty(env, result, "channelMasks", channelMasks_);

    std::vector<int32_t> channelIndexMasks_;
    channelIndexMasks_.push_back(deviceInfo.channelIndexMasks_);
    AddArrayProperty(env, result, "channelIndexMasks", channelIndexMasks_);

    std::vector<int32_t> encoding;
    encoding.push_back(deviceInfo.audioStreamInfo_.encoding);
    AddArrayProperty(env, result, "encodingTypes", encoding);

    return napi_ok;
}

napi_status CommonNapi::SetDeviceDescriptors(const napi_env &env,
    const std::vector<sptr<AudioStandard::AudioDeviceDescriptor>> &deviceDescriptors, napi_value &result)
{
    napi_status status = napi_create_array_with_length(env, deviceDescriptors.size(), &result);
    for (size_t i = 0; i < deviceDescriptors.size(); i++) {
        if (deviceDescriptors[i] != nullptr) {
            napi_value valueParam = nullptr;
            SetDeviceDescriptor(env, deviceDescriptors[i], valueParam);
            napi_set_element(env, result, i, valueParam);
        }
    }
    return status;
}

napi_status CommonNapi::SetValueDeviceInfo(const napi_env &env, const AudioStandard::DeviceInfo &deviceInfo,
    napi_value &result)
{
    std::vector<sptr<AudioStandard::AudioDeviceDescriptor>> deviceDescriptors;
    sptr<AudioStandard::AudioDeviceDescriptor> audioDeviceDescriptor =
        new(std::nothrow) AudioStandard::AudioDeviceDescriptor();
    CHECK_AND_RETURN_RET_LOG(audioDeviceDescriptor != nullptr, napi_generic_failure,
        "audioDeviceDescriptor malloc failed");
    ConvertDeviceInfoToAudioDeviceDescriptor(audioDeviceDescriptor, deviceInfo);
    deviceDescriptors.push_back(std::move(audioDeviceDescriptor));
    SetDeviceDescriptors(env, deviceDescriptors, result);
    return napi_ok;
}
} // namespace Media
} // namespace OHOS