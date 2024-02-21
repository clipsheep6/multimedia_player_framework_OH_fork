/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cmath>
#include <iostream>
#include "aw_common.h"
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "isoundpool.h"
#include "soundpoolcreate_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace AudioStandard;

namespace OHOS {
namespace Media {
SoundPoolCreateFuzzer::SoundPoolCreateFuzzer()
{
}

SoundPoolCreateFuzzer::~SoundPoolCreateFuzzer()
{
}

constexpr int32_t CONTENT_TYPE_LIST = 10;
const ContentType contentType_[CONTENT_TYPE_LIST] {
    CONTENT_TYPE_UNKNOWN,
    CONTENT_TYPE_SPEECH,
    CONTENT_TYPE_MUSIC,
    CONTENT_TYPE_MOVIE,
    CONTENT_TYPE_SONIFICATION,
    CONTENT_TYPE_RINGTONE,
    CONTENT_TYPE_PROMPT,
    CONTENT_TYPE_GAME,
    CONTENT_TYPE_DTMF,
    CONTENT_TYPE_ULTRASONIC
};

constexpr int32_t STREAM_USAGE_LIST = 21;
const StreamUsage streamUsage_[STREAM_USAGE_LIST] {
    STREAM_USAGE_UNKNOWN,
    STREAM_USAGE_MEDIA,
    STREAM_USAGE_MUSIC,
    STREAM_USAGE_VOICE_COMMUNICATION,
    STREAM_USAGE_VOICE_ASSISTANT,
    STREAM_USAGE_ALARM,
    STREAM_USAGE_VOICE_MESSAGE,
    STREAM_USAGE_NOTIFICATION_RINGTONE,
    STREAM_USAGE_RINGTONE,
    STREAM_USAGE_NOTIFICATION,
    STREAM_USAGE_ACCESSIBILITY,
    STREAM_USAGE_SYSTEM,
    STREAM_USAGE_MOVIE,
    STREAM_USAGE_GAME,
    STREAM_USAGE_AUDIOBOOK,
    STREAM_USAGE_NAVIGATION,
    STREAM_USAGE_DTMF,
    STREAM_USAGE_ENFORCED_TONE,
    STREAM_USAGE_ULTRASONIC,
    STREAM_USAGE_RANGING,
    STREAM_USAGE_VOICE_MODEM_COMMUNICATION
};

bool SoundPoolCreateFuzzer::FuzzSoundPoolCreate(uint8_t *data, size_t size)
{
    int32_t maxStreams = *reinterpret_cast<int32_t *>(data);
    AudioStandard::AudioRendererInfo audioRenderInfo;

    int32_t contenttypesubscript = *reinterpret_cast<int32_t *>(data) % (CONTENT_TYPE_LIST);
    audioRenderInfo.contentType = contentType_[contenttypesubscript];

    int32_t streamusagesubscript = *reinterpret_cast<int32_t *>(data) % (STREAM_USAGE_LIST);
    audioRenderInfo.streamUsage = streamUsage_[streamusagesubscript];

    audioRenderInfo.rendererFlags = 0;

    TestSoundPool::CreateSoundPool(maxStreams, audioRenderInfo);
    TestSoundPool::Release();
    return true;
}

bool SoundPoolCreateFuzzer::FuzzSoundPoolCreateFlags(uint8_t *data, size_t size)
{
    int maxStreams = 3;
    AudioStandard::AudioRendererInfo audioRenderInfo;

    int32_t contenttypesubscript = *reinterpret_cast<int32_t *>(data) % (CONTENT_TYPE_LIST);
    audioRenderInfo.contentType = contentType_[contenttypesubscript];

    int32_t streamusagesubscript = *reinterpret_cast<int32_t *>(data) % (STREAM_USAGE_LIST);
    audioRenderInfo.streamUsage = streamUsage_[streamusagesubscript];

    audioRenderInfo.rendererFlags = *reinterpret_cast<int32_t *>(data);

    TestSoundPool::CreateSoundPool(maxStreams, audioRenderInfo);
    TestSoundPool::Release();
    return true;
}
} // namespace Media

bool FuzzTestSoundPoolCreate(uint8_t *data, size_t size)
{
    auto soundPool_ = std::make_unique<SoundPoolCreateFuzzer>();
    return soundPool_->FuzzSoundPoolCreate(data, size);
}

bool FuzzTestSoundPoolCreateFlags(uint8_t *data, size_t size)
{
    auto soundPool_ = std::make_unique<SoundPoolCreateFuzzer>();
    return soundPool_->FuzzSoundPoolCreateFlags(data, size);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzTestSoundPoolCreateFlags(data, size);
    FuzzTestSoundPoolCreate(data, size);
    return 0;
}