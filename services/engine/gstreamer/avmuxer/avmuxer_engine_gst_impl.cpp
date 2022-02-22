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
static constexpr uint32_t MAX_VIDEO_TRACK_NUM = 1;
static constexpr uint32_t MAX_AUDIO_TRACK_NUM = 16;

static void StartFeed(GstAppSrc *src, guint length, gpointer user_data)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "AppSrc does not exist");
    std::string name = gst_element_get_name(src);
    int32_t trackID = name.back() - '0';
    (*reinterpret_cast<std::map<int32_t, MyType> *>(user_data))[trackID].needData_ = true;
}

static void StopFeed(GstAppSrc *src, gpointer user_data)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "AppSrc does not exist");
    std::string name = gst_element_get_name(src);
    int32_t trackID = name.back() - '0';
    (*reinterpret_cast<std::map<int32_t, MyType> *>(user_data))[trackID].needData_ = false;
}

AVMuxerEngineGstImpl::AVMuxerEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    funcMap_[MUX_H264] = AVMuxerUtil::Writeh264CodecData;
    funcMap_[MUX_H263] = AVMuxerUtil::Writeh263CodecData;
    funcMap_[MUX_MPEG4] = AVMuxerUtil::WriteMPEG4CodecData;
    funcMap_[MUX_AAC] = AVMuxerUtil::WriteaacCodecData;
    funcMap_[MUX_MP3] = AVMuxerUtil::Writemp3CodecData;
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
    muxBin_ = GST_MUX_BIN(gst_element_factory_make("muxbin", "avmuxerbin"));
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
        g_object_set(muxBin_, "path", rawUri.c_str(), "mux", FORMAT_TO_MUX.at(format).c_str(), nullptr);
    } else if (uriHelper.UriType() == UriHelper::URI_TYPE_FD) {
        rawUri = path.substr(strlen("fd://"));
        g_object_set(muxBin_, "fd", std::stoi(rawUri), "mux", FORMAT_TO_MUX.at(format).c_str(), nullptr);
    } else {
        MEDIA_LOGE("Failed to check output path");
        return MSERR_INVALID_VAL;
    }

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

int32_t AVMuxerEngineGstImpl::AddTrack(const MediaDescription &trackDesc, int32_t &trackId)
{
    MEDIA_LOGD("AddTrack");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    std::string mimeType;
    bool val = trackDesc.GetStringValue(std::string(MD_KEY_CODEC_MIME), mimeType);
    CHECK_AND_RETURN_RET_LOG(val = true, MSERR_INVALID_VAL, "Failed to get MD_KEY_CODEC_MIME");
    CHECK_AND_RETURN_RET_LOG(FORMAT_TO_MIME.at(format_).find(mimeType) != FORMAT_TO_MIME.at(format_).end(),
        MSERR_INVALID_OPERATION, "The mime type can not be added in current container format");

    trackId = trackInfo_.size() + 1;
    trackInfo_[trackId] = MyType();
    trackInfo_[trackId].type_ = MIME_MAP_TYPE.at(mimeType);
    trackInfo_[trackId].needData_ = true;
    std::string name = "src_";
    name += static_cast<char>('0' + trackId);

    int32_t ret;
    GstCaps *src_caps = nullptr;
    if (trackInfo_[trackId].type_ <= MUX_MPEG4) {
        CHECK_AND_RETURN_RET_LOG(videoTrackNum_ < MAX_VIDEO_TRACK_NUM, MSERR_INVALID_OPERATION,
            "Only 1 video Tracks can be added");
        ret = AVMuxerUtil::SetCaps(trackDesc, mimeType, src_caps, trackInfo_[trackId].type_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call Seth264Caps");
        videoTrackNum_++;
    } else {
        CHECK_AND_RETURN_RET_LOG(audioTrackNum_ < MAX_AUDIO_TRACK_NUM, MSERR_INVALID_OPERATION,
            "Only 16 audio Tracks can be added");
        ret = AVMuxerUtil::SetCaps(trackDesc, mimeType, src_caps, trackInfo_[trackId].type_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetaacCaps");
        audioTrackNum_++;
    }

    trackInfo_[trackId].caps_ = src_caps;
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
    for (auto& info : trackInfo_) {
        std::string name = "src_";
        name += static_cast<char>('0' + info.first);
        GstAppSrc *src = GST_APP_SRC_CAST(gst_bin_get_by_name(GST_BIN_CAST(muxBin_), name.c_str()));
        CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_INVALID_OPERATION, "src does not exist");
        gst_app_src_set_callbacks(src, &callbacks, reinterpret_cast<gpointer *>(&trackInfo_), NULL);
    }
    return MSERR_OK; 
}

int32_t AVMuxerEngineGstImpl::WriteData(std::shared_ptr<AVSharedMemory> sampleData,
    const TrackSampleInfo &sampleInfo, GstElement *src)
{
    MEDIA_LOGD("WriteData");
    CHECK_AND_RETURN_RET_LOG(trackInfo_[sampleInfo.trackIdx].needData_ == true, MSERR_INVALID_OPERATION,
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

bool isAllHasCaps(std::map<int, MyType>& trackInfo)
{
    for (auto& info : trackInfo) {
        if (info.second.hasCodecData_ == false) {
            return false;
        }
    }
    return true;
}

bool isAllHasBuffer(std::map<int, MyType>& trackInfo)
{
    for (auto& info : trackInfo) {
        if (info.second.hasBuffer_ == false) {
            return false;
        }
    }
    return true;
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
    if (trackInfo_[sampleInfo.trackIdx].hasCodecData_ == false && sampleInfo.flags == CODEC_DATA) {
        ret = funcMap_[trackInfo_[sampleInfo.trackIdx].type_](sampleData, sampleInfo, src, muxBin_, trackInfo_, allocator_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to write CodecData");

        trackInfo_[sampleInfo.trackIdx].hasCodecData_ = true;
        if (isAllHasCaps(trackInfo_) && !isPause_) {
            gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PAUSED);
            isPause_ = true;
            MEDIA_LOGD("Current state is Pause");
        }
    } else {
        ret = WriteData(sampleData, sampleInfo, src);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteData");
        trackInfo_[sampleInfo.trackIdx].hasBuffer_ = true;
        if (isAllHasBuffer(trackInfo_) && !isPlay_) {
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
        for (auto& info : trackInfo_) {
            std::string name = "src_";
            name += static_cast<char>('0' + info.first);
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
    trackInfo_.clear();
    endFlag_ = false;
    errHappened_ = false;
    isReady_ = false;
    isPause_ = false;
    isPlay_ = false;
    msgProcessor_->Reset();
}
}  // namespace Media
}  // namespace OHOS
