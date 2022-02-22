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
#include "gstappsrc.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerUtil"};
}

namespace OHOS {
namespace Media {
using SetCapsFunc = std::function<void(GstCaps *src_caps)>;
static void SetH264Caps(GstCaps);
static void SetH263Caps(GstCaps);
static void SetMPEG4Caps(GstCaps);
static void SetAACCaps(GstCaps);
static void SetMP3Caps(GstCaps);

static int32_t parseParam(FormatParam &param, const MediaDescription &trackDesc, bool isVideo) {
    bool ret;
    if (isVideo) {
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

std::map<MimeType, SetCapsFunc> funcMap = {
    {MUX_H264, SetH264Caps},
    {MUX_H263, SetH263Caps},
    {MUX_MPEG4, SetMPEG4Caps},
    {MUX_AAC, SetAACCaps},
    {MUX_MP3, SetMP3Caps},
};

static void SetH264Caps(GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "width", G_TYPE_INT, param.width,
        "height", G_TYPE_INT, param.height,
        "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
        nullptr);
}

static void SetH263Caps(GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "width", G_TYPE_INT, param.width,
        "height", G_TYPE_INT, param.height,
        "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
        nullptr);
}

static void SetMPEG4Caps(GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 4,
        "systemstream", G_TYPE_BOOLEAN, FALSE,
        "width", G_TYPE_INT, param.width,
        "height", G_TYPE_INT, param.height,
        "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
        nullptr);
}

static void SetAACCaps(GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 4,
        "stream-format", G_TYPE_STRING, "adts",
        "channels", G_TYPE_INT, param.channels,
        "rate", G_TYPE_INT, param.rate,
        nullptr);
}

static void SetMP3Caps(GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 1,
        "layer", G_TYPE_INT, 3,
        "channels", G_TYPE_INT, param.channels,
        "rate", G_TYPE_INT, param.rate,
        nullptr);
}

int32_t AVMuxerUtil::SetCaps(const MediaDescription &trackDesc, const std::string &mimeType,
    GstCaps *src_caps, MimeType type)
{
    MEDIA_LOGD("Set %s cpas", mimeType.c_str());
    bool ret;
    FormatParam param;
    if (type <= MUX_MPEG4) {
        ret = parseParam(param, trackDesc, true);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "Failed to call parseParam");
    } else {
        ret = parseParam(param, trackDesc, false);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, MSERR_INVALID_VAL, "Failed to call parseParam");
    }
    funcMap[type](src_caps);

    return MSERR_OK;
}

GstBuffer *PushCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstShMemWrapAllocator *allocator)
{
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);

    return buffer;
}

int32_t AVMuxerUtil::Writeh264CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    MEDIA_LOGD("Writeh264CodecData");
    uint8_t *start = sampleData->GetBase();
    if ((start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) ||
        (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01)) {
        g_object_set(muxBin, "h264parse", true, nullptr);
        gst_caps_set_simple(trackInfo[sampleInfo.trackIdx].caps_,
            "alignment", G_TYPE_STRING, "nal",
            "stream-format", G_TYPE_STRING, "byte-stream",
            nullptr);
        g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);

        GstBuffer *buffer = PushCodecData(sampleData, sampleInfo, allocator);

        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
        CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");
    } else {
        GstBuffer *buffer = PushCodecData(sampleData, sampleInfo, allocator);
        gst_caps_set_simple(trackInfo[sampleInfo.trackIdx].caps_,
            "codec_data", GST_TYPE_BUFFER, buffer,
            "alignment", G_TYPE_STRING, "au",
            "stream-format", G_TYPE_STRING, "avc",
            nullptr);
        g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);

        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
        CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");
    }
    return MSERR_OK;
}

int32_t AVMuxerUtil::Writeh263CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    MEDIA_LOGD("Writeh263CodecData");
    g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);

    GstBuffer *buffer = PushCodecData(sampleData, sampleInfo, allocator);
    gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteMPEG4CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    MEDIA_LOGD("WriteMPEG4CodecData");
    g_object_set(muxBin, "mpeg4parse", true, nullptr);
    g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);

    GstBuffer *buffer = PushCodecData(sampleData, sampleInfo, allocator);
    gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteaacCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    MEDIA_LOGD("WriteaacCodecData");
    g_object_set(muxBin, "aacparse", true, nullptr);
    g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);

    GstBuffer *buffer = PushCodecData(sampleData, sampleInfo, allocator);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerUtil::Writemp3CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
        GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    MEDIA_LOGD("Writemp3CodecData");
    g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);

    GstBuffer *buffer = PushCodecData(sampleData, sampleInfo, allocator);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}
}
}