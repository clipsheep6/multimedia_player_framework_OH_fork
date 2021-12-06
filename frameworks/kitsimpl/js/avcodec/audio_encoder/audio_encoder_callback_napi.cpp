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

#include "audio_encoder_callback_napi.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AudioEncoderCallbackNapi"};
}

namespace OHOS {
namespace Media {
AudioEncoderCallbackNapi::AudioEncoderCallbackNapi(napi_env env)
    : env_(env)
{
    (void)env_;
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioEncoderCallbackNapi::~AudioEncoderCallbackNapi()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AudioEncoderCallbackNapi::OnError(AVCodecErrorType errorType, int32_t errCode)
{
    MEDIA_LOGD("OnError is called");
}

void AudioEncoderCallbackNapi::OnOutputFormatChanged(const Format &format)
{
    MEDIA_LOGD("OnOutputFormatChanged is called");
}

void AudioEncoderCallbackNapi::OnInputBufferAvailable(uint32_t index)
{
    MEDIA_LOGD("OnInputBufferAvailable is called, index: %{public}u", index);
}

void AudioEncoderCallbackNapi::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MEDIA_LOGD("OnOutputBufferAvailable is called, index: %{public}u", index);
}
}  // namespace Media
}  // namespace OHOS
