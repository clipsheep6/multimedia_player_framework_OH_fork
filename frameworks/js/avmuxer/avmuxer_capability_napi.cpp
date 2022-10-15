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

#include "avmuxer_capability_napi.h"
#include "common_napi.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerCapabilityNapi"};
}

namespace OHOS {
namespace Media {
napi_value AVMuxerCapabilityNapi::GetAVMuxerFormatList(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("GetAVMuxerFormatList In");

    auto asyncCtx = std::make_unique<MediaAsyncContext>(env);
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncCtx->SignError(MSERR_EXT_INVALID_VAL, "Failed to napi_get_cb_info");
    }

    asyncCtx->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncCtx->deferred = CommonNapi::CreatePromise(env, asyncCtx->callbackRef, result);

    (void)napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncCtx->napi));

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "GetAVMuxerFormatList", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource,
        [](napi_env env, void *data) {
            auto asyncCtx = reinterpret_cast<MediaAsyncContext *>(data);
            CHECK_AND_RETURN_LOG(asyncCtx != nullptr, "asyncCtx is nullptr!");
            if (asyncCtx->napi == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "nullptr");
                return;
            }
            auto avmuxer = AVMuxerFactory::CreateAVMuxer();
            if (avmuxer == nullptr) {
                asyncCtx->SignError(MSERR_EXT_UNKNOWN, "No memory");
                return;
            }
            std::vector<std::string> formatList = avmuxer->GetAVMuxerFormatList();
            asyncCtx->JsResult = std::make_unique<MediaJsResultStringVector>(formatList);
        },
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncCtx.get()), &asyncCtx->work));

    NAPI_CALL(env, napi_queue_async_work(env, asyncCtx->work));
    asyncCtx.release();

    return result;
}
}  // namespace Media
}  // namespace OHOS
