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

#include "avmuxer_util.h"
#include <tuple>
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerUtil"};
<<<<<<< HEAD
    constexpr uint32_t MSTONS = 1000000;
    constexpr uint32_t NUM_ZERO = 0;
    constexpr uint32_t NUM_ONE = 1;
    constexpr uint32_t NUM_TWO = 2;
=======
    constexpr uint32_t CAPS_FIELD_NAME_INDEX = 0;
    constexpr uint32_t CAPS_FIELD_TYPE_INDEX = 1;
    constexpr uint32_t CAPS_FIELD_VALUE_INDEX = 2;
>>>>>>> 604c0dc07647ad318c86796aac08fc2c51df1464
}

namespace OHOS {
namespace Media {
struct MultiValue {
    explicit MultiValue(int32_t val)
    {
        val_.intVal = val;
    }
    explicit MultiValue(const char *val)
    {
        val_.stringVal = val;
    }
    union Val {
        int32_t intVal;
        const char *stringVal;
    } val_ = {0};
};

std::map<std::string, std::vector<std::tuple<std::string, GType, MultiValue>>> optionCapsMap = {
    {"video/avc", {
        {"alignment", G_TYPE_STRING, MultiValue("nal")},
        {"stream-format", G_TYPE_STRING, MultiValue("byte-stream")}
    }},
    {"video/h263", {
    }},
    {"video/mp4v-es", {
        {"mpegversion", G_TYPE_INT, MultiValue(4)},
        {"systemstream", G_TYPE_BOOLEAN, MultiValue(FALSE)}
    }},
    {"audio/mp4a-latm", {
        {"mpegversion", G_TYPE_INT, MultiValue(4)},
        {"stream-format", G_TYPE_STRING, MultiValue("adts")}
    }},
    {"audio/mpeg", {
        {"mpegversion", G_TYPE_INT, MultiValue(1)},
        {"layer", G_TYPE_INT, MultiValue(3)}
    }}
};

TrackType AVMuxerUtil::CheckType(const std::string &mimeType)
{
    if (mimeType.find("video") == 0) {
        return VIDEO;
    } else if (mimeType.find("audio") == 0) {
        return AUDIO;
    } else {
        return UNKNOWN_TYPE;
    }
}

static int32_t parseParam(FormatParam &param, const MediaDescription &trackDesc, const std::string &mimeType)
{
    if (AVMuxerUtil::CheckType(mimeType) == VIDEO) {
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, param.width) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, param.height) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, param.frameRate) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
        MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
            param.width, param.height, param.frameRate);
    } else if (AVMuxerUtil::CheckType(mimeType) == AUDIO) {
        CHECK_AND_RETURN_RET_LOG(
            trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, param.channels) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_CHANNEL_COUNT");
        CHECK_AND_RETURN_RET_LOG(trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, param.rate) == true,
            MSERR_INVALID_VAL, "Failed to get MD_KEY_SAMPLE_RATE");
        MEDIA_LOGD("channels is: %{public}d, rate is: %{public}d", param.channels, param.rate);
    } else {
        MEDIA_LOGD("Faild to check track type");
        return MSERR_INVALID_VAL;
    }

    return MSERR_OK;
}

static void AddOptionCaps(GstCaps *src_caps, const std::string &mimeType)
{
    for (auto& elements : optionCapsMap[mimeType]) {
        switch (std::get<1>(elements)) {
            case G_TYPE_BOOLEAN:
            case G_TYPE_INT:
                gst_caps_set_simple(src_caps,
<<<<<<< HEAD
                    std::get<NUM_ZERO>(elements).c_str(),
                    std::get<NUM_ONE>(elements),
                    std::get<NUM_TWO>(elements).val_.intVal,
=======
                    std::get<CAPS_FIELD_NAME_INDEX>(elements).c_str(),
                    std::get<CAPS_FIELD_TYPE_INDEX>(elements),
                    std::get<CAPS_FIELD_VALUE_INDEX>(elements).val_.intVal,
>>>>>>> 604c0dc07647ad318c86796aac08fc2c51df1464
                    nullptr);
                break;
            case G_TYPE_STRING:
                gst_caps_set_simple(src_caps,
<<<<<<< HEAD
                    std::get<NUM_ZERO>(elements).c_str(),
                    std::get<NUM_ONE>(elements),
                    std::get<NUM_TWO>(elements).val_.stringVal,
=======
                    std::get<CAPS_FIELD_NAME_INDEX>(elements).c_str(),
                    std::get<CAPS_FIELD_TYPE_INDEX>(elements),
                    std::get<CAPS_FIELD_VALUE_INDEX>(elements).val_.stringVal,
>>>>>>> 604c0dc07647ad318c86796aac08fc2c51df1464
                    nullptr);
                break;
            default:
                break;
        }
    }
}

static void CreateCaps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps)
{
    if (AVMuxerUtil::CheckType(mimeType) == VIDEO) {
        src_caps = gst_caps_new_simple(std::get<0>(MIME_MAP_TYPE.at(mimeType)).c_str(),
            "width", G_TYPE_INT, param.width,
            "height", G_TYPE_INT, param.height,
            "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
            nullptr);
    } else if (AVMuxerUtil::CheckType(mimeType) == AUDIO) {
        src_caps = gst_caps_new_simple(std::get<0>(MIME_MAP_TYPE.at(mimeType)).c_str(),
            "channels", G_TYPE_INT, param.channels,
            "rate", G_TYPE_INT, param.rate,
            nullptr);
    } else {
        MEDIA_LOGE("Failed to check track type");
        return;
    }
    AddOptionCaps(src_caps, mimeType);
}

int32_t AVMuxerUtil::SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
    GstCaps *src_caps)
{
    MEDIA_LOGD("Set %{public}s cpas", mimeType.c_str());
    bool ret;
    FormatParam param;
    ret = parseParam(param, trackDesc, mimeType);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "Failed to call parseParam");
    CreateCaps(param, mimeType, src_caps);

    return MSERR_OK;
}
<<<<<<< HEAD

int32_t PushCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstAppSrc *src, GstShMemWrapAllocator *allocator)
{
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator), sampleData);
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_NO_MEMORY, "Failed to call gst_shmem_wrap");
    GstBuffer *buffer = gst_buffer_new();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MSERR_NO_MEMORY, "Failed to call gst_buffer_new");
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeMs * MSTONS);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeMs * MSTONS);
    if (sampleInfo.flags & AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }

    GstFlowReturn ret = gst_app_src_push_buffer(src, buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call gst_app_src_push_buffer");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstAppSrc *src, std::map<int, TrackInfo>& trackInfo, GstShMemWrapAllocator *allocator)
{
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr, MSERR_INVALID_VAL, "sampleData is nullptr");
    CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_INVALID_VAL, "src is nullptr");
    CHECK_AND_RETURN_RET_LOG(allocator != nullptr, MSERR_INVALID_VAL, "allocator is nullptr");
    int32_t ret = PushCodecData(sampleData, sampleInfo, src, allocator);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call PushCodecData");
    return MSERR_OK;
}
=======
>>>>>>> 604c0dc07647ad318c86796aac08fc2c51df1464
}
}