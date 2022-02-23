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
#include "gstappsrc.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerUtil"};
}

namespace OHOS {
namespace Media {

struct MyClass {
    explicit MyClass(int32_t val) {
        val_.intVal = val;
    }
    explicit MyClass(const char *val) {
        val_.stringVal = val;
    }
    union MyGType {
        int32_t    intVal;
        const char *stringVal;
    } val_;
};

std::map<MimeType, std::vector<std::tuple<std::string, GType, MyClass>>> capsMap = {
    {MUX_H264, {{"alignment", G_TYPE_STRING, MyClass("nal")}, {"stream-format", G_TYPE_STRING, MyClass("byte-stream")}}},
    {MUX_H263, {}},
    {MUX_MPEG4, {{"mpegversion", G_TYPE_INT, MyClass(4)}, {"systemstream", G_TYPE_BOOLEAN, MyClass(FALSE)}}},
    {MUX_AAC, {{"mpegversion", G_TYPE_INT, MyClass(4)}, {"stream-format", G_TYPE_STRING, MyClass("adts")}}},
    {MUX_MP3, {{"mpegversion", G_TYPE_INT, MyClass(1)}, {"layer", G_TYPE_INT, MyClass(3)}}}
};

static int32_t parseParam(FormatParam &param, const MediaDescription &trackDesc, MimeType type) {
    bool ret;
    if (type < VIDEO_TYPE_END) {
        ret = trackDesc.GetIntValue(std::string(MD_KEY_WIDTH), param.width);
        CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
        ret = trackDesc.GetIntValue(std::string(MD_KEY_HEIGHT), param.height);
        CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
        ret = trackDesc.GetIntValue(std::string(MD_KEY_FRAME_RATE), param.frameRate);
        CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
        MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
            param.width, param.height, param.frameRate);
    } else {
        ret = trackDesc.GetIntValue(std::string(MD_KEY_CHANNEL_COUNT), param.channels);
        CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_CHANNEL_COUNT");
        ret = trackDesc.GetIntValue(std::string(MD_KEY_SAMPLE_RATE), param.rate);
        CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_SAMPLE_RATE");
        MEDIA_LOGD("channels is: %{public}d, rate is: %{public}d", param.channels, param.rate);
    }

    return MSERR_OK;
}

static void AddCaps(GstCaps *src_caps, MimeType type)
{
    for (auto& elements : capsMap[type]) {
        switch(std::get<1>(elements)) {
            case G_TYPE_BOOLEAN:
            case G_TYPE_INT:
                gst_caps_set_simple(src_caps,
                    std::get<0>(elements).c_str(), std::get<1>(elements), std::get<2>(elements).val_.intVal,
                    nullptr);
                break;
            case G_TYPE_STRING:
                gst_caps_set_simple(src_caps,
                    std::get<0>(elements).c_str(), std::get<1>(elements), std::get<2>(elements).val_.stringVal,
                    nullptr);
                break;
            default:
                break;
        }
    }
}

static void CreateCaps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps, MimeType type)
{
    if (type < VIDEO_TYPE_END) {
        src_caps = gst_caps_new_simple(std::get<0>(MIME_MAP_TYPE.at(mimeType)).c_str(),
            "width", G_TYPE_INT, param.width,
            "height", G_TYPE_INT, param.height,
            "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
            nullptr);
    } else {
        src_caps = gst_caps_new_simple(std::get<0>(MIME_MAP_TYPE.at(mimeType)).c_str(),
            "channels", G_TYPE_INT, param.channels,
            "rate", G_TYPE_INT, param.rate,
            nullptr);
    }
    AddCaps(src_caps, type);
}

int32_t AVMuxerUtil::SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
    GstCaps *src_caps, MimeType type)
{
    MEDIA_LOGD("Set %s cpas", mimeType.c_str());
    bool ret;
    FormatParam param;
    ret = parseParam(param, trackDesc, type);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "Failed to call parseParam");
    CreateCaps(param, mimeType, src_caps, type);

    return MSERR_OK;
}

int32_t PushCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstElement *src, GstShMemWrapAllocator *allocator)
{
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    if (sampleInfo.flags == SYNC_FRAME) {
        gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call gst_app_src_push_buffer");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstElement *src, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);
    int32_t ret = PushCodecData(sampleData, sampleInfo, src, allocator);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call PushCodecData");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstElement *src)
{
    CHECK_AND_RETURN_RET_LOG(trackInfo_[sampleInfo.trackIdx].needData_ == true, MSERR_INVALID_OPERATION,
        "Failed to push data, the queue is full");
    CHECK_AND_RETURN_RET_LOG(sampleInfo.timeUs >= 0, MSERR_INVALID_VAL, "Failed to check dts, dts muxt >= 0");

    int32_t ret = PushCodecData(sampleData, sampleInfo, src, allocator);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call PushCodecData");
    return MSERR_OK;
}
}
}