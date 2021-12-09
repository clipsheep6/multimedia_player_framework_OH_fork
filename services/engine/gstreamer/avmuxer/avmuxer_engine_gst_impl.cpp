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

#include <iostream>
#include "avmuxer_engine_gst_impl.h"
#include "gst_utils.h"
#include "gstappsrc.h"
#include "media_errors.h"
#include "media_log.h"
#include "convert_codec_data.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerEngineGstImpl"};
}

namespace OHOS {
namespace Media {
static void start_feed(GstAppSrc* src, guint length, gpointer user_data)
{
    char* name = gst_element_get_name(src);
    int32_t trackID = name[0] - '0';
    (*reinterpret_cast<std::map<int32_t, bool>*>(user_data))[trackID] = true;
}

static void stop_feed(GstAppSrc *src, gpointer user_data)
{
    char* name = gst_element_get_name(src);
    int32_t trackID = name[0] - '0';
    (*reinterpret_cast<std::map<int32_t, bool>*>(user_data))[trackID] = false;
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

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetOutput(const std::string &path, const std::string &format)
{
    MEDIA_LOGI("SetOutput");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");

    g_object_set(muxBin_, "path", path.c_str(), "mux", formatToMux.at(format).c_str(), nullptr);
    format_ = format;

    return MSERR_OK;
    
}

int32_t AVMuxerEngineGstImpl::SetLocation(float latitude, float longitude)
{
    MEDIA_LOGI("SetLocation");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");

    GstElement* element = gst_bin_get_by_name(GST_BIN_CAST(muxBin_), "splitmuxsink");
    GstTagSetter* tagsetter = GST_TAG_SETTER(element);
    gst_tag_setter_add_tags(tagsetter, GST_TAG_MERGE_REPLACE_ALL,
        "geo-location-latitude", latitude,
        "geo-location-longitude", longitude,
        nullptr);

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetOrientationHint(int degrees)
{
    MEDIA_LOGI("SetOrientationHint");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");

    GstElement* element = gst_bin_get_by_name(GST_BIN_CAST(muxBin_), "splitmuxsink");
    GstTagSetter* tagsetter = GST_TAG_SETTER(element);
    gst_tag_setter_add_tags(tagsetter, GST_TAG_MERGE_REPLACE_ALL, "image-orientation", degrees, nullptr);

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    MEDIA_LOGI("AddTrack");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");

    std::string mimeType;
    trackDesc.GetStringValue(std::string(MD_KEY_CODEC_MIME), mimeType);
    CHECK_AND_RETURN_RET_LOG(formatToEncode.at(format_).find(mimeType) != formatToEncode.at(format_).end(),
        MSERR_INVALID_OPERATION, "The mime type can not be added in current container format");

    trackId = trackIdSet.size() + 1;
    trackIdSet.insert(trackId);
    needData_[trackId] = true;
    std::string name = "src_";
    name += static_cast<char>('0' + trackId);

    int32_t width;
    int32_t height;
    trackDesc.GetIntValue(std::string(MD_KEY_WIDTH), width);
    trackDesc.GetIntValue(std::string(MD_KEY_HEIGHT), height);
    MEDIA_LOGI("width is: %{public}d", width);
    MEDIA_LOGI("height is: %{public}d", height);
    GstCaps* src_caps = nullptr;
    if (audioEncodeType.find(mimeType) == audioEncodeType.end()) {
        src_caps = gst_caps_new_simple(mimeType.c_str(),
            "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height,
            "alignment", G_TYPE_STRING, "au",
            "stream-format", G_TYPE_STRING, "avc",
            nullptr);
        CHECK_AND_RETURN_RET_LOG(videoTrackNum < MAX_VIDEO_TRACK_NUM, MSERR_INVALID_OPERATION, "Only 1 video Tracks can be added");
        addTrack(muxBin_, VIDEO, name.c_str());
    } else {
        CHECK_AND_RETURN_RET_LOG(audioTrackNum < MAX_AUDIO_TRACK_NUM, MSERR_INVALID_OPERATION, "Only 16 audio Tracks can be added");
        addTrack(muxBin_, AUDIO, name.c_str());
    }
    CapsMat[trackId] = src_caps;

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Start()
{
    MEDIA_LOGI("Start");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");
    gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_READY);

    GstAppSrcCallbacks callbacks = {&start_feed, &stop_feed, NULL};
    for (int32_t i : trackIdSet) {
        std::string name = "src_";
        name += static_cast<char>('0' + i);
        GstAppSrc* src = GST_APP_SRC_CAST(gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str()));
        CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_UNKNOWN, "src does not exist");
        gst_app_src_set_callbacks(src, &callbacks, reinterpret_cast<gpointer*>(&needData_), NULL);
    }
    return MSERR_OK; 
}

int32_t AVMuxerEngineGstImpl::WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo)
{
    MEDIA_LOGI("WriteTrackSample");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");

    std::string name = "src_";
    name += static_cast<char>('0' + sampleInfo.trackIdx);
    GstElement* src = gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str());
    if (hasCaps.find(sampleInfo.trackIdx) == hasCaps.end() && sampleInfo.flags == CODEC_DATA) {
        // std::shared_ptr<ConvertCodecData> convertImpl = std::make_shared<ConvertCodecData>();
        // GstBuffer* codecData = convertImpl->GetCodecBuffer(sampleData);
        // CHECK_AND_RETURN_RET_LOG(codecData != nullptr, MSERR_UNKNOWN, "Failed to get codec data");
        GstMemory* mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
        GstBuffer* buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, mem);
        gst_caps_set_simple(CapsMat[sampleInfo.trackIdx], "codec_data", GST_TYPE_BUFFER, buffer, nullptr);
        g_object_set (src, "caps", CapsMat[sampleInfo.trackIdx], nullptr);
        hasCaps.insert(sampleInfo.trackIdx);
        if (hasCaps == trackIdSet && !isPause_) {
            gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PAUSED);
            cond_.wait(lock, [this]() { return isPause_ || errHappened_; });
            MEDIA_LOGI("Current state is Pause");
        }
    } else {
        CHECK_AND_RETURN_RET_LOG(needData_[sampleInfo.trackIdx] == true, MSERR_UNKNOWN, "Failed to push data, the queue is full");
        GstFlowReturn ret;
        GstMemory* mem = gst_shmem_wrap(GST_ALLOCATOR_CAST(allocator_), sampleData);
        GstBuffer* buffer = gst_buffer_new();
        gst_buffer_append_memory(buffer, mem);
        MEDIA_LOGI("sampleInfo.timeUs is: %{public}lld", sampleInfo.timeUs);
        if (sampleInfo.timeUs < 0) {
            GST_BUFFER_PTS(buffer) = GST_CLOCK_TIME_NONE;
        } else {
            GST_BUFFER_PTS(buffer) = static_cast<uint64_t>(sampleInfo.timeUs * 1000);
        }
        if (sampleInfo.flags == SYNC_FRAME) {
            gst_buffer_set_flags(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        }
        ret = gst_app_src_push_buffer(GST_APP_SRC(src), buffer);
        MEDIA_LOGI("ret is: %{public}d", ret);
        CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_UNKNOWN, "Failed to push data");
        hasBuffer.insert(sampleInfo.trackIdx);
        if (hasBuffer == trackIdSet && !isPlay_) {
            gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PLAYING);
            cond_.wait(lock, [this]() { return isPlay_ || errHappened_; });
            MEDIA_LOGI("Current state is Play");
        }

    }
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::Stop()
{
    MEDIA_LOGI("Stop");
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_UNKNOWN, "Muxbin does not exist");

    GstFlowReturn ret;
    // CHECK_AND_RETURN_RET_LOG(hasCaps == trackIdSet && hasBuffer == trackIdSet, MSERR_UNKNOWN, "Not all track has cpas or buffer");
    // g_signal_emit_by_name(GST_BIN_CAST(muxBin_), "end-of-stream", NULL);
    for (int32_t i : trackIdSet) {
        std::string name = "src_";
        name += static_cast<char>('0' + i);
        GstAppSrc* src = GST_APP_SRC_CAST(gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str()));
        CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_UNKNOWN, "src does not exist");
        ret = gst_app_src_end_of_stream(src);
        CHECK_AND_RETURN_RET_LOG(ret == GST_FLOW_OK, MSERR_UNKNOWN, "Failed to push end of stream");
    }
    cond_.wait(lock, [this]() { return endFlag_ || errHappened_; });
    gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_NULL);
    clear();
    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetupMsgProcessor()
{
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE_CAST(muxBin_));
    CHECK_AND_RETURN_RET_LOG(bus != nullptr, MSERR_UNKNOWN, "Failed to create GstBus");

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
            MEDIA_LOGI("End of stream");
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
            MEDIA_LOGI("State change");
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

void AVMuxerEngineGstImpl::clear()
{
    trackIdSet.clear();
    hasCaps.clear();
    hasBuffer.clear();
    needData_.clear();
    CapsMat.clear();
    endFlag_ = false;
    errHappened_ = false;
    msgProcessor_->Reset();
}
}  // namespace Media
}  // namespace OHOS
