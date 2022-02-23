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
using SetCapsFunc = std::function<void(GstCaps *src_caps)>;
using WriteCodecDataFunc = std::function<int32_t(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo)>;

static void SetH264Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps);
static void SetH263Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps);
static void SetMPEG4Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps);
static void SetAACCaps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps);
static void SetMP3Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps);

static int32_t WriteH264CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo);
static int32_t WriteH263CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo);
static int32_t WriteMPEG4CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo);
static int32_t WriteAACCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo);
static int32_t WriteMP3CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo);

static const std::map<MimeType, std::tuple<SetCapsFunc, WriteCodecDataFunc>> funcMap = {
    {MUX_H264, {SetH264Caps, WriteH264CodecData}},
    {MUX_H263, {SetH263Caps, WriteH263CodecData}},
    {MUX_MPEG4, {SetMPEG4Caps, WriteMPEG4CodecData}},
    {MUX_AAC, {SetAACCaps, WriteAACCodecData}},
    {MUX_MP3, {SetMP3Caps, WriteMP3CodecData}},
};

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

static void SetH264Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "width", G_TYPE_INT, param.width,
        "height", G_TYPE_INT, param.height,
        "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
        nullptr);
}

static void SetH263Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "width", G_TYPE_INT, param.width,
        "height", G_TYPE_INT, param.height,
        "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
        nullptr);
}

static void SetMPEG4Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 4,
        "systemstream", G_TYPE_BOOLEAN, FALSE,
        "width", G_TYPE_INT, param.width,
        "height", G_TYPE_INT, param.height,
        "framerate", GST_TYPE_FRACTION, param.frameRate, 1,
        nullptr);
}

static void SetAACCaps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps)
{
    src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 4,
        "stream-format", G_TYPE_STRING, "adts",
        "channels", G_TYPE_INT, param.channels,
        "rate", G_TYPE_INT, param.rate,
        nullptr);
}

static void SetMP3Caps(FormatParam &param, const std::string &mimeType, GstCaps *src_caps)
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
    std::get<0>(funcMap.at(type))(param, mimeType, src_caps);

    return MSERR_OK;
}

int32_t *PushCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstShMemWrapAllocator *allocator)
{
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call gst_app_src_push_buffer");

    return MSERR_OK;
}

int32_t AVMuxerUtil::WriteCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstElement *src, GstMuxBin *muxBin, std::map<int, MyType>& trackInfo, GstShMemWrapAllocator *allocator)
{
    
    std::get<1>(funcMap.at(trackInfo[sampleInfo.trackIdx].type_))(sampleData, sampleInfo, muxBin, trackInfo);
    g_object_set(src, "caps", trackInfo[sampleInfo.trackIdx].caps_, nullptr);
    GstFlowReturn ret = PushCodecData(sampleData, sampleInfo, allocator);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to call PushCodecData");
}

static void WriteH264CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo)
{
    MEDIA_LOGD("WriteH264CodecData");
    uint8_t *start = sampleData->GetBase();
    if ((start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) ||
        (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01)) {
        g_object_set(muxBin, "h264parse", true, nullptr);
        gst_caps_set_simple(trackInfo[sampleInfo.trackIdx].caps_,
            "alignment", G_TYPE_STRING, "nal",
            "stream-format", G_TYPE_STRING, "byte-stream",
            nullptr);
    } else {
        gst_caps_set_simple(trackInfo[sampleInfo.trackIdx].caps_,
            "codec_data", GST_TYPE_BUFFER, buffer,
            "alignment", G_TYPE_STRING, "au",
            "stream-format", G_TYPE_STRING, "avc",
            nullptr);
    }

    return MSERR_OK;
}

static void WriteH263CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo)
{
    MEDIA_LOGD("WriteH263CodecData");

    return MSERR_OK;
}

static void WriteMPEG4CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo)
{
    MEDIA_LOGD("WriteMPEG4CodecData");
    g_object_set(muxBin, "mpeg4parse", true, nullptr);

    return MSERR_OK;
}

static void WriteAACCodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo)
{
    MEDIA_LOGD("WriteAACCodecData");
    g_object_set(muxBin, "aacparse", true, nullptr);

    return MSERR_OK;
}

static void WriteMP3CodecData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo,
    GstMuxBin *muxBin, std::map<int, MyType>& trackInfo)
{
    MEDIA_LOGD("WriteMP3CodecData");

    return MSERR_OK;
}
}
}