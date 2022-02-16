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

#include "avmuxer_engine_gst_impl.h"
#include <iostream>
#include "gst_utils.h"
#include "gstappsrc.h"
#include "media_errors.h"
#include "media_log.h"
#include "convert_codec_data.h"
#include <unistd.h>
#include "gstbaseparse.h"
#include "gst_mux_bin.h"
#include "uri_helper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerEngineGstImpl"};
}

namespace OHOS {
namespace Media {
static void StartFeed(GstAppSrc *src, guint length, gpointer user_data)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "AppSrc does not exist");
    std::string name = gst_element_get_name(src);
    int32_t trackID = name.back() - '0';
    (*reinterpret_cast<std::map<int32_t, bool> *>(user_data))[trackID] = true;
}

static void StopFeed(GstAppSrc *src, gpointer user_data)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "AppSrc does not exist");
    std::string name = gst_element_get_name(src);
    int32_t trackID = name.back() - '0';
    (*reinterpret_cast<std::map<int32_t, bool> *>(user_data))[trackID] = false;
}

AVMuxerEngineGstImpl::AVMuxerEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerEngineGstImpl::~AVMuxerEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    if (muxBin_ != nullptr) {
        gst_object_unref(muxBin_);
        muxBin_ = nullptr;
    }
}

int32_t AVMuxerEngineGstImpl::Init()
{
    muxBin_ = GST_MUX_BIN(gst_element_factory_make("muxbin", "muxbin"));
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Failed to create muxbin");

    int32_t ret = SetupMsgProcessor();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to setup message processor");

    allocator_ = gst_shmem_wrap_allocator_new();
    CHECK_AND_RETURN_RET_LOG(allocator_ != nullptr, MSERR_NO_MEMORY, "Failed to create allocator");

    return MSERR_OK;
}

std::vector<std::string> AVMuxerEngineGstImpl::GetAVMuxerFormatList()
{
    MEDIA_LOGD("GetAVMuxerFormatList");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, std::vector<std::string>(), "Muxbin does not exist");
    return std::vector<std::string>(FORMAT_TYPE.begin(), FORMAT_TYPE.end());
}

int32_t AVMuxerEngineGstImpl::SetOutput(const std::string &path, const std::string &format)
{
    MEDIA_LOGD("SetOutput");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    std::string rawUri;
    UriHelper uriHelper(path);
    uriHelper.FormatMe();
    if (uriHelper.UriType() == UriHelper::URI_TYPE_FILE) {
        rawUri = path.substr(strlen("file://"));
    } else if (uriHelper.UriType() == UriHelper::URI_TYPE_FD) {
        rawUri = path.substr(strlen("fd://"));
    } else {
        MEDIA_LOGE("Failed to check output path");
        return MSERR_INVALID_VAL;
    }

    g_object_set(muxBin_, "path", rawUri.c_str(), "mux", FORMAT_TO_MUX.at(format).c_str(), nullptr);
    format_ = format;

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetLocation(float latitude, float longitude)
{
    MEDIA_LOGD("SetLocation");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    GstElement *element = gst_bin_get_by_name(GST_BIN_CAST(muxBin_), FORMAT_TO_MUX.at(format_).c_str());
    GstTagSetter *tagsetter = GST_TAG_SETTER(element);
    gst_tag_setter_add_tags(tagsetter, GST_TAG_MERGE_REPLACE_ALL,
        "geo-location-latitude", latitude,
        "geo-location-longitude", longitude,
        nullptr);

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetOrientationHint(int degrees)
{
    MEDIA_LOGD("SetOrientationHint");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    GstElement *element = gst_bin_get_by_name(GST_BIN_CAST(muxBin_), FORMAT_TO_MUX.at(format_).c_str());
    GstTagSetter *tagsetter = GST_TAG_SETTER(element);
    gst_tag_setter_add_tags(tagsetter, GST_TAG_MERGE_REPLACE_ALL, "image-orientation", degrees, nullptr);

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Seth264Caps(const MediaDescription &trackDesc,
    const std::string &mimeType, int32_t trackId)
{
    MEDIA_LOGD("Seth264Caps");
    bool ret;
    int32_t width;
    int32_t height;
    int32_t frameRate;
    ret = trackDesc.GetIntValue(std::string(MD_KEY_WIDTH), width);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_HEIGHT), height);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_FRAME_RATE), frameRate);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
    MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
        width, height, frameRate);
    GstCaps *src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION, frameRate, 1,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(videoTrackNum_ < MAX_VIDEO_TRACK_NUM, MSERR_INVALID_OPERATION,
        "Only 1 video Tracks can be added");
    CapsMap_[trackId] = src_caps;
    videoTrackNum_++;
    
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Seth263Caps(const MediaDescription &trackDesc,
    const std::string &mimeType, int32_t trackId)
{
    MEDIA_LOGD("Seth263Caps");
    bool ret;
    int32_t width;
    int32_t height;
    int32_t frameRate;
    ret = trackDesc.GetIntValue(std::string(MD_KEY_WIDTH), width);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_HEIGHT), height);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_FRAME_RATE), frameRate);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
    MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
        width, height, frameRate);
    GstCaps *src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION, frameRate, 1,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(videoTrackNum_ < MAX_VIDEO_TRACK_NUM, MSERR_INVALID_OPERATION,
        "Only 1 video Tracks can be added");
    CapsMap_[trackId] = src_caps;
    videoTrackNum_++;
    
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetMPEG4Caps(const MediaDescription &trackDesc,
    const std::string &mimeType, int32_t trackId)
{
    MEDIA_LOGD("SetMEPG4Caps");
    bool ret;
    int32_t width;
    int32_t height;
    int32_t frameRate;
    ret = trackDesc.GetIntValue(std::string(MD_KEY_WIDTH), width);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_WIDTH");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_HEIGHT), height);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_HEIGHT");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_FRAME_RATE), frameRate);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_FRAME_RATE");
    MEDIA_LOGD("width is: %{public}d, height is: %{public}d, frameRate is: %{public}d",
        width, height, frameRate);
    GstCaps *src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 4,
        "systemstream", G_TYPE_BOOLEAN, FALSE,
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION, frameRate, 1,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(videoTrackNum_ < MAX_VIDEO_TRACK_NUM, MSERR_INVALID_OPERATION,
        "Only 1 video Tracks can be added");
    CapsMap_[trackId] = src_caps;
    videoTrackNum_++;
    
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetaacCaps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId)
{
    MEDIA_LOGD("SetaacCaps");
    bool ret;
    int32_t channels;
    int32_t rate;
    ret = trackDesc.GetIntValue(std::string(MD_KEY_CHANNEL_COUNT), channels);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_CHANNEL_COUNT");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_SAMPLE_RATE), rate);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_SAMPLE_RATE");
    MEDIA_LOGD("channels is: %{public}d, rate is: %{public}d", channels, rate);
    GstCaps *src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 4,
        "stream-format", G_TYPE_STRING, "adts",
        "channels", G_TYPE_INT, channels,
        "rate", G_TYPE_INT, rate,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(audioTrackNum_ < MAX_AUDIO_TRACK_NUM, MSERR_INVALID_OPERATION,
        "Only 16 audio Tracks can be added");
    CapsMap_[trackId] = src_caps;
    audioTrackNum_++;

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Setmp3Caps(const MediaDescription &trackDesc, const std::string &mimeType, int32_t trackId)
{
    MEDIA_LOGD("Setmp3Caps");
    bool ret;
    int32_t channels;
    int32_t rate;
    ret = trackDesc.GetIntValue(std::string(MD_KEY_CHANNEL_COUNT), channels);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_CHANNEL_COUNT");
    ret = trackDesc.GetIntValue(std::string(MD_KEY_SAMPLE_RATE), rate);
    CHECK_AND_RETURN_RET_LOG(ret == true, MSERR_INVALID_VAL, "Failed to get MD_KEY_SAMPLE_RATE");
    MEDIA_LOGD("channels is: %{public}d, rate is: %{public}d", channels, rate);
    GstCaps *src_caps = gst_caps_new_simple(MIME_MAP_ENCODE.at(mimeType).c_str(),
        "mpegversion", G_TYPE_INT, 1,
        "layer", G_TYPE_INT, 3,
        "channels", G_TYPE_INT, channels,
        "rate", G_TYPE_INT, rate,
        nullptr);
    CHECK_AND_RETURN_RET_LOG(audioTrackNum_ < MAX_AUDIO_TRACK_NUM, MSERR_INVALID_OPERATION,
        "Only 16 audio Tracks can be added");
    CapsMap_[trackId] = src_caps;
    audioTrackNum_++;

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    MEDIA_LOGD("AddTrack");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    std::string mimeType;
    bool val = trackDesc.GetStringValue(std::string(MD_KEY_CODEC_MIME), mimeType);
    CHECK_AND_RETURN_RET_LOG(val = true, MSERR_INVALID_VAL, "Failed to get MD_KEY_CODEC_MIME");
    CHECK_AND_RETURN_RET_LOG(FORMAT_TO_MIME.at(format_).find(mimeType) != FORMAT_TO_MIME.at(format_).end(),
        MSERR_INVALID_OPERATION, "The mime type can not be added in current container format");

    trackId = trackIdSet_.size() + 1;
    trackIdSet_.insert(trackId);
    trackId2EncodeType_.insert(std::make_pair(trackId, mimeType));
    needData_[trackId] = true;
    std::string name = "src_";
    name += static_cast<char>('0' + trackId);

    int32_t ret;
    if (trackId2EncodeType_[trackId] == std::string("video/x-h264")) {
        ret = Seth264Caps(trackDesc, mimeType, trackId);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Seth264Caps");
    } else if (trackId2EncodeType_[trackId] == std::string("video/x-h263")) {
        ret = Seth263Caps(trackDesc, mimeType, trackId);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Seth263Caps");
    } else if (trackId2EncodeType_[trackId] == std::string("video/mpeg4")) {
        ret = SetMPEG4Caps(trackDesc, mimeType, trackId);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetMPEG4Caps");
    } else if (trackId2EncodeType_[trackId] == std::string("audio/aac")) {
        ret = SetaacCaps(trackDesc, mimeType, trackId);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetaacCaps");
    } else if (trackId2EncodeType_[trackId] == std::string("audio/mp3")) {
        ret = Setmp3Caps(trackDesc, mimeType, trackId);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetaacCaps");
    }
    gst_mux_bin_add_track(muxBin_, AUDIO_MIME_TYPE.find(mimeType) == AUDIO_MIME_TYPE.end() ? VIDEO : AUDIO, name.c_str());

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Start()
{
    MEDIA_LOGD("Start");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");
    gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_READY);

    GstAppSrcCallbacks callbacks = {&StartFeed, &StopFeed, NULL};
    for (int32_t i : trackIdSet_) {
        std::string name = "src_";
        name += static_cast<char>('0' + i);
        GstAppSrc *src = GST_APP_SRC_CAST(gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str()));
        CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_INVALID_OPERATION, "src does not exist");
        gst_app_src_set_callbacks(src, &callbacks, reinterpret_cast<gpointer *>(&needData_), NULL);
    }
    return MSERR_OK; 
}

int32_t AVMuxerEngineGstImpl::Writeh264CodecData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("Writeh264CodecData");
    uint8_t *start = sampleData->GetBase();
    if ((start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x01) ||
        (start[0] == 0x00 && start[1] == 0x00 && start[2] == 0x00 && start[3] == 0x01)) {
        g_object_set(muxBin_, "h264parse", true, nullptr);
        gst_caps_set_simple(CapsMap_[sampleInfo.trackIdx],
            "alignment", G_TYPE_STRING, "nal",
            "stream-format", G_TYPE_STRING, "byte-stream",
            nullptr);
        g_object_set(src, "caps", CapsMap_[sampleInfo.trackIdx], nullptr);

        GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
        GstBuffer *buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, mem);
        GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
        GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);

        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
        CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");
    } else {
        GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
        GstBuffer *buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, mem);
        gst_caps_set_simple(CapsMap_[sampleInfo.trackIdx],
            "codec_data", GST_TYPE_BUFFER, buffer,
            "alignment", G_TYPE_STRING, "au",
            "stream-format", G_TYPE_STRING, "avc",
            nullptr);
        g_object_set(src, "caps", CapsMap_[sampleInfo.trackIdx], nullptr);
    }
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Writeh263CodecData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("Writeh263CodecData");
    g_object_set(src, "caps", CapsMap_[sampleInfo.trackIdx], nullptr);

    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::WriteMPEG4CodecData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("WriteMPEG4CodecData");
    g_object_set(muxBin_, "mpeg4parse", true, nullptr);
    g_object_set(src, "caps", CapsMap_[sampleInfo.trackIdx], nullptr);

    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::WriteaacCodecData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("WriteaacCodecData");
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    g_object_set(muxBin_, "aacparse", true, nullptr);
    g_object_set(src, "caps", CapsMap_[sampleInfo.trackIdx], nullptr);

    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Writemp3CodecData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("Writemp3CodecData");
    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    g_object_set(src, "caps", CapsMap_[sampleInfo.trackIdx], nullptr);

    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::WriteData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("WriteData");
    CHECK_AND_RETURN_RET_LOG(needData_[sampleInfo.trackIdx] == true, MSERR_INVALID_OPERATION,
        "Failed to push data, the queue is full");
    CHECK_AND_RETURN_RET_LOG(sampleInfo.timeUs >= 0, MSERR_INVALID_VAL, "Failed to check dts, dts muxt >= 0");

    GstMemory *mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
    GstBuffer *buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, mem);
    GST_BUFFER_DTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
    if (sampleInfo.flags == SYNC_FRAME) {
        gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
    }

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
    CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_INVALID_OPERATION, "Failed to push data");
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo)
{   
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr, MSERR_INVALID_VAL, "sampleData is nullptr");
    MEDIA_LOGD("WriteTrackSample, sampleInfo.trackIdx is %{public}d", sampleInfo.trackIdx);
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");
    CHECK_AND_RETURN_RET_LOG(sampleInfo.timeUs >= 0, MSERR_INVALID_VAL, "Failed to check dts, dts muxt >= 0");
    int32_t ret;

    std::string name = "src_";
    name += static_cast<char>('0' + sampleInfo.trackIdx);
    GstElement *src = gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str());
    MEDIA_LOGD("data[0] is: %{public}u, data[1] is: %{public}u, data[2] is: %{public}u, data[3] is: %{public}u,",
        ((uint8_t*)(sampleData->GetBase()))[0], ((uint8_t*)(sampleData->GetBase()))[1],
        ((uint8_t*)(sampleData->GetBase()))[2], ((uint8_t*)(sampleData->GetBase()))[3]);
    if (hasCaps_.find(sampleInfo.trackIdx) == hasCaps_.end() && sampleInfo.flags == CODEC_DATA) {
        if (trackId2EncodeType_[sampleInfo.trackIdx] == std::string("video/x-h264")) {
            ret = Writeh264CodecData(sampleData, sampleInfo, src);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Writeh264CodecData");
        } else if (trackId2EncodeType_[sampleInfo.trackIdx] == std::string("video/x-h263")) {
            ret = Writeh263CodecData(sampleData, sampleInfo, src);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Writeh263CodecData");
        } else if (trackId2EncodeType_[sampleInfo.trackIdx] == std::string("video/mpeg4")) {
            ret = WriteMPEG4CodecData(sampleData, sampleInfo, src);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteMPEG4CodecData");
        } else if (trackId2EncodeType_[sampleInfo.trackIdx] == std::string("audio/aac")) {
            ret = WriteaacCodecData(sampleData, sampleInfo, src);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteaacData");
        } else if (trackId2EncodeType_[sampleInfo.trackIdx] == std::string("audio/mp3")) {
            ret = Writemp3CodecData(sampleData, sampleInfo, src);
            CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Writemp3Data");
        }
        hasCaps_.insert(sampleInfo.trackIdx);
        if (hasCaps_ == trackIdSet_ && !isPause_) {
            gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PAUSED);
            isPause_ = true;
            MEDIA_LOGD("Current state is Pause");
        }
    } else {
        ret = WriteData(sampleData, sampleInfo, src);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteData");
        hasBuffer_.insert(sampleInfo.trackIdx);
        if (hasBuffer_ == trackIdSet_ && !isPlay_) {
            gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PLAYING);
            isPlay_ = true;
            MEDIA_LOGD("Current state is Play");
        }
    }
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Stop()
{
    MEDIA_LOGD("Stop");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    GstFlowReturn ret;
    if (isPlay_) {
        for (int32_t i : trackIdSet_) {
            std::string name = "src_";
            name += static_cast<char>('0' + i);
            GstAppSrc *src = GST_APP_SRC_CAST(gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str()));
            CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_INVALID_OPERATION, "src does not exist");
            ret = gst_app_src_end_of_stream(src);
            CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, ret, "Failed to push end of stream");
        }
        cond_.wait(lock, [this]() {
            return endFlag_ || errHappened_;
        });
    }
    gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_NULL);
    Clear();
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetupMsgProcessor()
{
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE_CAST(muxBin_));
    CHECK_AND_RETURN_RET_LOG(bus != nullptr, MSERR_INVALID_OPERATION, "Failed to create GstBus");

    auto msgNotifier = std::bind(&AVMuxerEngineGstImpl::OnNotifyMessage, this, std::placeholders::_1);
    msgProcessor_ = std::make_unique<GstMsgProcessor>(*bus, msgNotifier);
    gst_object_unref(bus);
    bus = nullptr;

    int32_t ret = msgProcessor_->Init();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    msgProcessor_->AddMsgFilter(ELEM_NAME(GST_ELEMENT_CAST(muxBin_)));

    return MSERR_OK;
}

void AVMuxerEngineGstImpl::OnNotifyMessage(const InnerMessage &msg)
{
    switch (msg.type) {
        case InnerMsgType::INNER_MSG_EOS: {
            MEDIA_LOGD("End of stream");
            endFlag_ = true;
            cond_.notify_all();
            break;
        }
        case InnerMsgType::INNER_MSG_ERROR: {
            MEDIA_LOGE("Error happened");
            msgProcessor_->FlushBegin();
            msgProcessor_->Reset();
            errHappened_ = true;
            cond_.notify_all();
            break;
        }
        case InnerMsgType::INNER_MSG_STATE_CHANGED: {
            MEDIA_LOGD("State change");
            GstState currState = static_cast<GstState>(msg.detail2);
            if (currState == GST_STATE_READY) {
                isReady_ = true;
            } else if (currState == GST_STATE_PAUSED) {
                isPause_ = true;
            } else if (currState == GST_STATE_PLAYING) {
                isPlay_ = true;
            }
            cond_.notify_all();
            break;
        }
        default:
            break;
    }
}

void AVMuxerEngineGstImpl::Clear()
{
    trackIdSet_.clear();
    hasCaps_.clear();
    hasBuffer_.clear();
    needData_.clear();
    CapsMap_.clear();
    endFlag_ = false;
    errHappened_ = false;
    isReady_ = false;
    isPause_ = false;
    isPlay_ = false;
    msgProcessor_->Reset();
}
}  // namespace Media
}  // namespace OHOS
