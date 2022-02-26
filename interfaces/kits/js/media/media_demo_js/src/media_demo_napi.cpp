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

#include "media_demo_napi.h"
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <time.h>
#include <random>
#include "media_log.h"
#include "display_type.h"
#include "securec.h"
#include "media_errors.h"
#include "media_surface.h"
#include "surface_utils.h"
#include "ui/rs_surface_node.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDemoNapi"};
}

namespace OHOS {
namespace Media {
napi_ref MediaDemoNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "MediaTest";

uint32_t surfaceCount = 0;
std::unordered_map<int32_t, sptr<Surface>> surfaceMap;

MediaDemoNapi::MediaDemoNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDemoNapi::~MediaDemoNapi()
{
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value MediaDemoNapi::Init(napi_env env, napi_value exports)
{
    napi_property_descriptor staticProperty[] = {
        DECLARE_NAPI_STATIC_FUNCTION("createMediaTest", CreateMediaTest),
    };
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("startStream", StartStream),
        DECLARE_NAPI_FUNCTION("closeStream", CloseStream),
        DECLARE_NAPI_FUNCTION("setResolution", SetResolution),
        DECLARE_NAPI_FUNCTION("setFrameCount", SetFrameCount),
        DECLARE_NAPI_FUNCTION("setFrameRate", SetFrameRate),
    };
    napi_value constructor = nullptr;
    napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define AudioRecorder class");
    status = napi_create_reference(env, constructor, 1, &constructor_);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");
    status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");
    status = napi_define_properties(env, exports, sizeof(staticProperty) / sizeof(staticProperty[0]), staticProperty);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static function");
    MEDIA_LOGD("Init success");
    return exports;
}

napi_value MediaDemoNapi::Constructor(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value jsThis = nullptr;
    size_t argCount = 0;
    napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        MEDIA_LOGE("Failed to retrieve details about the callback");
        return result;
    }

    MediaDemoNapi *mediatestNapi = new(std::nothrow) MediaDemoNapi();
    CHECK_AND_RETURN_RET_LOG(mediatestNapi != nullptr, nullptr, "No memory");

    mediatestNapi->env_ = env;

    status = napi_wrap(env, jsThis, reinterpret_cast<void *>(mediatestNapi),
        MediaDemoNapi::Destructor, nullptr, &(mediatestNapi->wrapper_));
    if (status != napi_ok) {
        napi_get_undefined(env, &result);
        delete mediatestNapi;
        MEDIA_LOGE("Failed to wrap native instance");
        return result;
    }

    MEDIA_LOGD("Constructor success");
    return jsThis;
}

void MediaDemoNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
    (void)env;
    (void)finalize;
    if (nativeObject != nullptr) {
        delete reinterpret_cast<MediaDemoNapi *>(nativeObject);
    }
    MEDIA_LOGD("Destructor success");
}

napi_value MediaDemoNapi::CreateMediaTest(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    napi_status status = napi_get_reference_value(env, constructor_, &constructor);
    if (status != napi_ok) {
        MEDIA_LOGE("Failed to get the representation of constructor object");
        napi_get_undefined(env, &result);
        return result;
    }
    status = napi_new_instance(env, constructor, 0, nullptr, &result);
    if (status != napi_ok) {
        MEDIA_LOGE("new instance fail");
        napi_get_undefined(env, &result);
        return result;
    }
    MEDIA_LOGD("CreateMediaTest success");
    return result;
}

void MediaDemoNapi::BufferLoop()
{
    MEDIA_LOGD("Enter BufferLoop");
    count_ = 0;
    pts_ = 0;
    color_ = 0xFF;
    while (count_ <= totalFrameCount_) {
        if (!isStart_.load()) {
            MEDIA_LOGD("Exit thread");
            break;
        }
        const uint32_t sleepTime = 1000000 / frameRate_;
        usleep(sleepTime);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t releaseFence;
        OHOS::BufferRequestConfig g_requestConfig = {
            .width = width_,
            .height = height_,
            .strideAlignment = 8,
            .format = PIXEL_FMT_YCRCB_420_SP,
            .usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
            .timeout = 0
        };
        OHOS::SurfaceError ret = producerSurface_->RequestBuffer(buffer, releaseFence, g_requestConfig);
        if (ret == OHOS::SURFACE_ERROR_NO_BUFFER) {
            MEDIA_LOGD("surface loop full, no buffer now");
            continue;
        }
        if ((ret != SURFACE_ERROR_OK) || (buffer == nullptr)) {
            MEDIA_LOGD("RequestBuffer failed");
            continue;
        }
        auto addr = static_cast<uint8_t *>(buffer->GetVirAddr());
        if (addr == nullptr) {
            MEDIA_LOGD("GetVirAddr failed");
            (void)producerSurface_->CancelBuffer(buffer);
            continue;
        }
        const uint32_t bufferSize = width_ * height_ * 3 / 2;
        char *tempBuffer = static_cast<char *>(malloc(sizeof(char) * bufferSize));
        if (tempBuffer == nullptr) {
            MEDIA_LOGE("malloc failed");
            (void)producerSurface_->CancelBuffer(buffer);
            continue;
        }
        memset(tempBuffer, color_, bufferSize - 1);

        srand((int32_t)time(0));
        for (uint32_t i = 0; i < bufferSize -1; i += 100) {
            if (i >= bufferSize - 1) {
                break;
            }
            tempBuffer[i] = (char)(rand() % 255);
        }

        color_ -= 3;
        if (color_ <= 0) {
            color_ = 0xFF;
        }
        errno_t mRet = memcpy_s(addr, buffer->GetSize(), tempBuffer, bufferSize);
        if (mRet != EOK) {
            (void)producerSurface_->CancelBuffer(buffer);
            free(tempBuffer);
            MEDIA_LOGE("memcpy_s failed");
            continue;
        }
        if (count_ == totalFrameCount_) {
            (void)buffer->ExtraSet("endOfStream", true);
            MEDIA_LOGD("EOS surface buffer");
        } else {
            (void)buffer->ExtraSet("endOfStream", false);
        }
        (void)buffer->ExtraSet("timeStamp", pts_);
        count_++;
        pts_ += 1000000000 / frameRate_;
        OHOS::BufferFlushConfig g_flushConfig = {
            .damage = {
                .x = 0,
                .y = 0,
                .w = width_,
                .h = height_
            },
            .timestamp = 0
        };
        (void)producerSurface_->FlushBuffer(buffer, -1, g_flushConfig);
        free(tempBuffer);
    }
}

bool MediaDemoNapi::StrToUint64(const std::string &str, uint64_t &value)
{
    if (str.empty() || !isdigit(str.front())) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    auto addr = str.data();
    auto result = strtou ll(addr, &end, 10); /* 10 means decimal */
    if ((end == addr) || (end[0] != '\0') || (errno == ERANGE)) {
        return false;
    }

    value = result;
    return true;
}

std::string MediaDemoNapi::GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
        CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "no memory");
        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

napi_value MediaDemoNapi::StartStream(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }
    MediaDemoNapi *test = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&test));
    std::string surfaceID = "";
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_string) {
        surfaceID = GetStringArgument(env, args[0]);
    }

    uint64_t surfaceUniqueID = 0;
    StrToUint64(surfaceID, surfaceUniqueID);
    test->producerSurface_ = SurfaceUtils::GetInstance()->GetSurface(surfaceUniqueID);
    if (test->producerSurface_ == nullptr) {
        MEDIA_LOGE("producerSurface_ is null");
        return undefinedResult;
    }
    if (test->cameraHDIThread_ != nullptr) {
        test->isStart_.store(false);
        test->cameraHDIThread_->join();
        test->cameraHDIThread_.reset();
        test->cameraHDIThread_ = nullptr;
    }
    test->isStart_.store(true);
    test->cameraHDIThread_ = std::make_unique<std::thread>(&MediaDemoNapi::BufferLoop, test));
    return undefinedResult;
}

napi_value MediaDemoNapi::CloseStream(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }
    MediaDemoNapi *test = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&test));
    test->isStart_.store(false);
    if (test->cameraHDIThread_ != nullptr) {
        test->cameraHDIThread_->join();
        test->cameraHDIThread_.reset();
        test->cameraHDIThread_ = nullptr;
    }
    return undefinedResult;
}

napi_value MediaDemoNapi::SetResolution(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value args[2] = { nullptr };
    size_t argCount = 2;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        MEDIA_LOGE("failed to napi_get_cb_info");
        return undefinedResult;
    }
    MediaDemoNapi *test = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&test));
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_number) {
        if (napi_get_value_int32(env, args[0], &test->width_) != napi_ok) {
            return undefinedResult;
        }
    }
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_number) {
        if (napi_get_value_int32(env, args[1], &test->height_) != napi_ok) {
            return undefinedResult;
        }
    }
    return undefinedResult;
}

napi_value MediaDemoNapi::SetFrameCount(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_value jsThis = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        return undefinedResult;
    }
    MediaDemoNapi *test = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&test));
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_number) {
        if (napi_get_value_int32(env, args[0], &test->totalFrameCount_) != napi_ok) {
            return undefinedResult;
        }
    }
    if (test->totalFrameCount_ <= 0) {
        test->totalFrameCount_ = 300;
    }
    return undefinedResult;
}

napi_value MediaDemoNapi::SetFrameRate(napi_env env, napi_callback_info info)
{
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);
    napi_value args[1] = { nullptr };
    size_t argCount = 1;
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        return undefinedResult;
    }
    MediaDemoNapi *test = nullptr;
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&test));
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_number) {
        if (napi_get_value_int32(env, args[0], &test->frameRate_) != napi_ok) {
            return undefinedResult;
        }
    }
    return undefinedResult;
}
}
}