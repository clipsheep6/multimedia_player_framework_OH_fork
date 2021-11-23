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

#ifndef CODEC_COMMON_H
#define CODEC_COMMON_H

#include <gst/audio/audio.h>
#include <gst/gst.h>

namespace OHOS {
namespace Media {
static const GstAudioChannelPosition CHANNEL_POSITION[2][2] = {
    {GST_AUDIO_CHANNEL_POSITION_MONO},
    {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT, GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT},
};

struct BufferWrapper {
    enum Owner {
        APP = 0,
        SERVER,
        DOWNSTREAM
    };
    BufferWrapper(GstBuffer *gstBuffer, uint32_t index)
    : gstBuffer_(gstBuffer), index_(index)
    {
    }
    ~BufferWrapper()
    {
        if (gstBuffer_ != nullptr) {
            gst_buffer_unref(gstBuffer_);
        }
    }
    GstBuffer *gstBuffer_ = nullptr;
    uint32_t index_ = 0;
    Owner owner_ = DOWNSTREAM;
};

struct ProcessorConfig {
    ProcessorConfig(GstCaps *caps)
    : caps_(caps)
    {
    }
    ~ProcessorConfig()
    {
        if (caps_ != nullptr) {
            gst_caps_unref(caps_);
        }
    }
    GstCaps *caps_ = nullptr;
    bool pushBlankFrame_ = false;
};
} // Media
} // OHOS
#endif // CODEC_COMMON_H
