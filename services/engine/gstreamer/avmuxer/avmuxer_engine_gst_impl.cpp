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
    constexpr uint32_t ROTATION_90 = 90;
    constexpr uint32_t ROTATION_180 = 180;
    constexpr uint32_t ROTATION_270 = 270;
    constexpr int32_t MAX_LATITUDE = 90;
    constexpr int32_t MIN_LATITUDE = -90;
    constexpr int32_t MAX_LONGITUDE = 180;
    constexpr int32_t MIN_LONGITUDE = -180;
    constexpr uint32_t MULTIPLY10000 = 10000;
}

namespace OHOS {
namespace Media {
static constexpr uint32_t MAX_VIDEO_TRACK_NUM = 1;
static constexpr uint32_t MAX_AUDIO_TRACK_NUM = 16;

static void StartFeed(GstAppSrc *src, guint length, gpointer user_data)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "AppSrc does not exist");
    CHECK_AND_RETURN_LOG(user_data != nullptr, "User data does not exist");
    std::map<int32_t, TrackInfo> trackInfo = *reinterpret_cast<std::map<int32_t, TrackInfo> *>(user_data);
    for (auto& info : trackInfo) {
        if (info.second.src_ == src) {
            info.second.needData_ = true;
            break;
        }
    }
}

static void StopFeed(GstAppSrc *src, gpointer user_data)
{
    CHECK_AND_RETURN_LOG(src != nullptr, "AppSrc does not exist");
    CHECK_AND_RETURN_LOG(user_data != nullptr, "User data does not exist");
    std::map<int32_t, TrackInfo> trackInfo = *reinterpret_cast<std::map<int32_t, TrackInfo> *>(user_data);
    for (auto& info : trackInfo) {
        if (info.second.src_ == src) {
            info.second.needData_ = false;
            break;
        }
    }
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

    std::vector<std::string> formatList;
    for (auto& formats : FORMAT_TO_MIME) {
        formatList.push_back(formats.first);
    }
    return formatList;
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

    bool setLocationToMux = true;
    if (latitude < MIN_LATITUDE || latitude > MAX_LATITUDE || longitude < MIN_LONGITUDE
        || longitude > MAX_LONGITUDE) {
        setLocationToMux = false;
        MEDIA_LOGE("Invalid GeoLocation, latitude: %{public}f, longitude: %{public}f",
            latitude, longitude);
    }

    int32_t latitudex10000 = latitude * MULTIPLY10000;
    int32_t longitudex10000 = longitude * MULTIPLY10000;
    if (setLocationToMux) {
        g_object_set(muxBin_, "latitude", latitudex10000, "longitude", longitudex10000, nullptr);
    }

    return MSERR_OK;
}

int32_t AVMuxerEngineGstImpl::SetOrientationHint(int degrees)
{
    MEDIA_LOGD("SetOrientationHint");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");

    bool setRotationToMux = true;
    if (degrees != ROTATION_90 && degrees != ROTATION_180 && degrees != ROTATION_270) {
        setRotationToMux = false;
        MEDIA_LOGE("Invalid rotation: %{public}d, keep default 0", degrees);
    }

    if (setRotationToMux) {
        g_object_set(muxBin_, "degrees", degrees, nullptr);
    }

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
    trackInfo_[trackId] = TrackInfo();
    trackInfo_[trackId].type_ = std::get<1>(MIME_MAP_TYPE.at(mimeType));
    trackInfo_[trackId].needData_ = true;

    int32_t ret;
    GstCaps *src_caps = nullptr;
    if (AVMuxerUtil::isVideo(trackInfo_[trackId].type_)) {
        CHECK_AND_RETURN_RET_LOG(videoTrackNum_ < MAX_VIDEO_TRACK_NUM, MSERR_INVALID_OPERATION,
            "Only 1 video Tracks can be added");
        ret = AVMuxerUtil::SetCaps(trackDesc, mimeType, src_caps, trackInfo_[trackId].type_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetCaps");
        videoTrackNum_++;
    } else {
        CHECK_AND_RETURN_RET_LOG(audioTrackNum_ < MAX_AUDIO_TRACK_NUM, MSERR_INVALID_OPERATION,
            "Only 16 audio Tracks can be added");
        ret = AVMuxerUtil::SetCaps(trackDesc, mimeType, src_caps, trackInfo_[trackId].type_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call SetCaps");
        audioTrackNum_++;
    }

    trackInfo_[trackId].caps_ = src_caps;
    std::string name = "src_";
    name += static_cast<char>('0' + trackId);
    gst_mux_bin_add_track(muxBin_, AVMuxerUtil::isVideo(trackInfo_[trackId].type_) ? VIDEO : AUDIO, name.c_str());
    SetParse(trackInfo_[sampleInfo.trackIdx].type_);

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
        info.second.src_ = src;
    }

    return MSERR_OK; 
}

static bool isAllHasCaps(std::map<int, TrackInfo>& trackInfo)
{
    for (auto& info : trackInfo) {
        if (info.second.hasCodecData_ == false) {
            return false;
        }
    }
    return true;
}

static bool isAllHasBuffer(std::map<int, TrackInfo>& trackInfo)
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
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(errHappened_ != true, MSERR_INVALID_OPERATION, "Error happend");
    CHECK_AND_RETURN_RET_LOG(muxBin_ != nullptr, MSERR_INVALID_OPERATION, "Muxbin does not exist");
    CHECK_AND_RETURN_RET_LOG(sampleData != nullptr, MSERR_INVALID_VAL, "sampleData is nullptr");
    MEDIA_LOGD("WriteTrackSample, sampleInfo.trackIdx is %{public}d", sampleInfo.trackIdx);
    CHECK_AND_RETURN_RET_LOG(trackInfo_[sampleInfo.trackIdx].needData_ == true, MSERR_INVALID_OPERATION,
        "Failed to push data, the queue is full");
    CHECK_AND_RETURN_RET_LOG(sampleInfo.timeUs >= 0, MSERR_INVALID_VAL, "Failed to check dts, dts muxt >= 0");
    int32_t ret;

    GstAppSrc *src = trackInfo_[sampleInfo.trackIdx].src_;
    CHECK_AND_RETURN_RET_LOG(src != nullptr, MSERR_INVALID_VAL, "Failed to get AppSrc of trackid: %{public}d",
        sampleInfo.trackIdx);
    MEDIA_LOGD("data[0] is: %{public}u, data[1] is: %{public}u, data[2] is: %{public}u, data[3] is: %{public}u,",
        ((uint8_t*)(sampleData->GetBase()))[0], ((uint8_t*)(sampleData->GetBase()))[1],
        ((uint8_t*)(sampleData->GetBase()))[2], ((uint8_t*)(sampleData->GetBase()))[3]);
    if (trackInfo_[sampleInfo.trackIdx].hasCodecData_ == false && sampleInfo.flags == AVCODEC_BUFFER_FLAG_CODEDC_DATA) {
        g_object_set(src, "caps", trackInfo_[sampleInfo.trackIdx].caps_, nullptr);
        ret = AVMuxerUtil::WriteData(sampleData, sampleInfo, src, trackInfo_, allocator_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to write CodecData");
        trackInfo_[sampleInfo.trackIdx].hasCodecData_ = true;
    } else {
        ret = AVMuxerUtil::WriteData(sampleData, sampleInfo, src, trackInfo_, allocator_);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "Failed to call WriteData");
        trackInfo_[sampleInfo.trackIdx].hasBuffer_ = true;
    }

    if (isAllHasCaps(trackInfo_) && !isPause_) {
        gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PAUSED);
        isPause_ = true;
        MEDIA_LOGD("Current state is Pause");
    }
    if (isAllHasBuffer(trackInfo_) && !isPlay_) {
        gst_element_set_state(GST_ELEMENT_CAST(muxBin_), GST_STATE_PLAYING);
        isPlay_ = true;
        MEDIA_LOGD("Current state is Play");
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
            ret = gst_app_src_end_of_stream(info.second.src_);
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

void AVMuxerEngineGstImpl::SetParse(CodecMimeType type)
{
    switch(type) {
        case CODEC_MIMIE_TYPE_VIDEO_AVC:
            g_object_set(muxBin_, "videoParse", "h264parse", nullptr);
            break;
        case CODEC_MIMIE_TYPE_VIDEO_MPEG4:
            g_object_set(muxBin_, "videoParse", "mpeg4parse", nullptr);
            break;
        case CODEC_MIMIE_TYPE_AUDIO_AAC:
            g_object_set(muxBin_, "audioParse", "aacparse", nullptr);
            break;
        default:
            break;
    }
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
            std::unique_lock<std::mutex> lock(mutex_);
            MEDIA_LOGD("End of stream");
            endFlag_ = true;
            cond_.notify_all();
            break;
        }
        case InnerMsgType::INNER_MSG_ERROR: {
            std::unique_lock<std::mutex> lock(mutex_);
            MEDIA_LOGE("Error happened");
            msgProcessor_->FlushBegin();
            msgProcessor_->Reset();
            errHappened_ = true;
            cond_.notify_all();
            break;
        }
        case InnerMsgType::INNER_MSG_STATE_CHANGED: {
            std::unique_lock<std::mutex> lock(mutex_);
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
    mutex_.unlock();
    msgProcessor_->Reset();
    mutex_.lock();
}
}  // namespace Media
}  // namespace OHOS
