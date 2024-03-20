/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "player_codec_ctrl.h"
#include "surface.h"
#include "media_log.h"
#include "media_errors.h"
#include "param_wrapper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerCodecCtrl"};
}

namespace OHOS {
namespace Media {
constexpr uint32_t MAX_SOFT_BUFFERS = 10;
constexpr uint32_t DEFAULT_CACHE_BUFFERS = 1;

PlayerCodecCtrl::PlayerCodecCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    DisablePerformanceBySysParam();
}

PlayerCodecCtrl::~PlayerCodecCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    notifier_ = nullptr;
    elementMap_.clear();
}

void PlayerCodecCtrl::SetupCodecCb(const std::string &metaStr, GstElement *src, GstElement *videoSink,
    CapsFixErrorNotifier notifier)
{
    if (metaStr.find("Codec/Decoder/Video/Hardware") != std::string::npos) {
        // hardware dec
        isHardwareDec_ = true;
        notifier_ = notifier;
        DecoderElement element;
        element.isHardware = true;
        element.signalId = g_signal_connect(src, "caps-fix-error", G_CALLBACK(&PlayerCodecCtrl::CapsFixErrorCb), this);
        elementMap_[src] = element;
        MEDIA_LOGD("add decoder element size = %{public}zu", elementMap_.size());
        g_object_set(G_OBJECT(src), "player-mode", TRUE, nullptr);
        if (!codecTypeList_.empty()) {
            // For hls scene when change codec, the second codec should not go performance mode process.
            codecTypeList_.push_back(true);
            return;
        }
        // For performance mode.
        codecTypeList_.push_back(true);

        g_object_set(G_OBJECT(src), "player-scene", TRUE, nullptr);
        if (isEnablePerformanceMode_) {
            g_object_set(G_OBJECT(src), "performance-mode", TRUE, nullptr);
            g_object_set(G_OBJECT(videoSink), "performance-mode", TRUE, nullptr);
        }

        GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12", nullptr);
        g_object_set(G_OBJECT(videoSink), "caps", caps, nullptr);
        g_object_set(G_OBJECT(src), "sink-caps", caps, nullptr);
        gst_caps_unref(caps);

        GstBufferPool *pool;
        g_object_get(videoSink, "surface-pool", &pool, nullptr);
        g_object_set(G_OBJECT(src), "surface-pool", pool, nullptr);
    } else if (metaStr.find("Codec/Decoder/Video") != std::string::npos) {
        // software dec
        codecTypeList_.push_back(false);
        isHardwareDec_ = false;
    }
}

void PlayerCodecCtrl::DetectCodecSetup(const std::string &metaStr, GstElement *src, GstElement *videoSink,
    CapsFixErrorNotifier notifier)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("Codec Setup");
    SetupCodecCb(metaStr, src, videoSink, notifier);
    if (IsFirstCodecSetup()) {
        SetupCodecBufferNum(metaStr, videoSink);
        MEDIA_LOGD("0x%{public}06" PRIXPTR " Set isHardwareDec_ %{public}d", FAKE_POINTER(this), isHardwareDec_);
        g_object_set(videoSink, "is-hardware-decoder", isHardwareDec_, nullptr);
        isHEBCMode_ = isHardwareDec_;
    }
}

void PlayerCodecCtrl::SetupCodecBufferNum(const std::string &metaStr, GstElement *src) const
{
    if (metaStr.find("Sink/Video") != std::string::npos && !isHardwareDec_) {
        g_object_set(G_OBJECT(src), "max-pool-capacity", MAX_SOFT_BUFFERS, nullptr);
        g_object_set(G_OBJECT(src), "cache-buffers-num", DEFAULT_CACHE_BUFFERS, nullptr);
    }
}

void PlayerCodecCtrl::CapsFixErrorCb(const GstElement *decoder, gpointer userData)
{
    CHECK_AND_RETURN_LOG(decoder != nullptr, "decoder is nullptr");
    CHECK_AND_RETURN_LOG(userData != nullptr, "userData is nullptr");
    MEDIA_LOGD("CapsFixErrorCb in");

    auto playerCodecCtrl = static_cast<PlayerCodecCtrl *>(userData);
    if (playerCodecCtrl->notifier_ != nullptr) {
        playerCodecCtrl->notifier_();
    }
}

void PlayerCodecCtrl::DetectCodecUnSetup(GstElement *src, GstElement *videoSink)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("Codec UnSetup");
    auto it = elementMap_.find(src);
    if (it != elementMap_.end()) {
        if (elementMap_[src].signalId != 0) {
            g_signal_handler_disconnect(src, elementMap_[src].signalId);
            elementMap_[src].signalId = 0;
        }
        elementMap_.erase(it);
    }
    MEDIA_LOGD("del decoder element size = %{public}zu", elementMap_.size());
    HlsSwichSoftAndHardCodec(videoSink);
}

void PlayerCodecCtrl::HlsSwichSoftAndHardCodec(GstElement *videoSink)
{
    CHECK_AND_RETURN_LOG(!codecTypeList_.empty(), "codec type list is empty");

    bool codecType = codecTypeList_.front();
    codecTypeList_.pop_front();
    if ((codecTypeList_.empty()) || (codecType == codecTypeList_.front())) {
        MEDIA_LOGD("codec type is empty or the next is same");
        return;
    }

    GstCaps *caps = nullptr;
    if (codecTypeList_.front()) {
        caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV12", nullptr);
        g_object_set(G_OBJECT(videoSink), "max-pool-capacity", SURFACE_MAX_QUEUE_SIZE, nullptr);
        g_object_set(G_OBJECT(videoSink), "cache-buffers-num", 0, nullptr);
    } else {
        caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", nullptr);
        g_object_set(G_OBJECT(videoSink), "max-pool-capacity", MAX_SOFT_BUFFERS, nullptr);
        g_object_set(G_OBJECT(videoSink), "cache-buffers-num", DEFAULT_CACHE_BUFFERS, nullptr);
    }
    MEDIA_LOGI("Set isHardwareDec_ %{public}d", codecTypeList_.front());
    g_object_set(videoSink, "is-hardware-decoder", codecTypeList_.front(), nullptr);
    isHEBCMode_ = codecTypeList_.front();
    g_object_set(G_OBJECT(videoSink), "caps", caps, nullptr);
    gst_caps_unref(caps);
}

void PlayerCodecCtrl::EnhanceSeekPerformance(bool enable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("EnhanceSeekPerformance %{public}d", enable);
    for (auto &it : elementMap_) {
        if (it.second.isHardware) {
            g_object_set(it.first, "seeking", enable, nullptr);
        }
    }
}

int32_t PlayerCodecCtrl::GetHEBCMode() const
{
    return isHEBCMode_;
}

bool PlayerCodecCtrl::IsFirstCodecSetup() const
{
    return codecTypeList_.size() == 1;
}

int32_t PlayerCodecCtrl::HandleCodecBuffers(bool enable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("HandleCodecBuffers %{public}d", enable);
    for (auto &it : elementMap_) {
        if (it.second.isHardware) {
            if (enable) {
                g_object_set(it.first, "free_codec_buffers", enable, nullptr);
            } else {
                g_object_set(it.first, "recover_codec_buffers", enable, nullptr);
            }
            return MSERR_OK;
        }
    }
    return MSERR_INVALID_OPERATION;
}

void PlayerCodecCtrl::StopFormatChange()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("StopFormatChange");
    for (auto &it : elementMap_) {
        if (it.second.isHardware) {
            g_object_set(it.first, "stop-format-change", TRUE, nullptr);
        }
    }
}
void PlayerCodecCtrl::DisablePerformanceBySysParam()
{
    std::string cmd;
    int32_t ret = OHOS::system::GetStringParameter("sys.media.player.performance.enable", cmd, "");
    if (ret == 0 && !cmd.empty()) {
        isEnablePerformanceMode_ = cmd == "FALSE" ? false : true;
    }
}
} // Media
} // OHOS