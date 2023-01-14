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

#include "audio_sink_factory.h"
#include "audio_sink_sv_impl.h"
#include "param_wrapper.h"

namespace OHOS {
namespace Media {
static bool AudioSinkDisEnable()
{
    std::string disenable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.audiosink.disenable", disenable, "");
    if (res != 0 || disenable.empty()) {
        return false;
    }
    return (disenable == "true" ? true : false);
}

std::unique_ptr<AudioSink> AudioSinkFactory::CreateAudioSink(GstBaseSink *sink)
{
    if (AudioSinkDisEnable()) {
        return std::make_unique<AudioSinkBypass>(sink);
    }
    return std::make_unique<AudioSinkSvImpl>(sink);
}
} // namespace Media
} // namespace OHOS
