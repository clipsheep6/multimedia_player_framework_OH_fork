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
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerEngineGstImpl"};
}

namespace OHOS {
namespace Media {

int32_t AVMuxerUtil::Seth264Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps)
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
    
    return MSERR_OK;
}

int32_t AVMuxerUtil::Seth263Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps)
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
    
    return MSERR_OK;
}

int32_t AVMuxerUtil::SetMPEG4Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps)
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
    
    return MSERR_OK;
}

int32_t AVMuxerUtil::SetaacCaps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps)
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

    return MSERR_OK;
}

int32_t AVMuxerUtil::Setmp3Caps(const MediaDescription &trackDesc, const std::string &mimeType,
        int32_t trackId, GstCaps *src_caps)
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

    return MSERR_OK;
}
}
}