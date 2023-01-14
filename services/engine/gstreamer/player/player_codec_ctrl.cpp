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
}

PlayerCodecCtrl::~PlayerCodecCtrl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    notifier_ = nullptr;
    if (decoder_ != nullptr) {
        if (signalId_ != 0) {
            g_signal_handler_disconnect(decoder_, signalId_);
            signalId_ = 0;
        }
        gst_object_unref(decoder_);
        decoder_ = nullptr;
    }
}

void PlayerCodecCtrl::SetupCodecCb(const std::string &metaStr, GstElement *src, GstElement *videoSink)
{
    if (metaStr.find("Codec/Decoder/Video/Hardware") != std::string::npos) {
        // hardware dec
        isHardwareDec_ = true;
        if (decoder_ != nullptr) {
            gst_object_unref(decoder_);
            decoder_ = nullptr;
        }
        g_object_set(G_OBJECT(src), "player-mode", TRUE, nullptr);
        decoder_ = GST_ELEMENT_CAST(gst_object_ref(src));
        if (!codecTypeList_.empty()) {
            // For hls scene when change codec, the second codec should not go performance mode process.
            codecTypeList_.push_back(true);
            return;
        }
        // For performance mode.
        codecTypeList_.push_back(true);

        g_object_set(G_OBJECT(src), "enable-slice-cat", TRUE, nullptr); // Enable slice
        g_object_set(G_OBJECT(src), "performance-mode", TRUE, nullptr);
        g_object_set(G_OBJECT(videoSink), "performance-mode", TRUE, nullptr);

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

void PlayerCodecCtrl::DetectCodecSetup(const std::string &metaStr, GstElement *src, GstElement *videoSink)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_LOGD("Codec Setup");
    SetupCodecCb(metaStr, src, videoSink);
    SetupCodecBufferNum(metaStr, src);
}

void PlayerCodecCtrl::SetupCodecBufferNum(const std::string &metaStr, GstElement *src) const
{
    if (metaStr.find("Sink/Video") != std::string::npos && !isHardwareDec_) {
        g_object_set(G_OBJECT(src), "max-pool-capacity", MAX_SOFT_BUFFERS, nullptr);
        g_object_set(G_OBJECT(src), "cache-buffers-num", DEFAULT_CACHE_BUFFERS, nullptr);
    }
}

void PlayerCodecCtrl::SetCapsFixErrorCb(CapsFixErrorNotifier notifier)
{
    notifier_ = notifier;
    CHECK_AND_RETURN_LOG(decoder_ != nullptr, "decoder_ is nullptr");
    signalId_ = g_signal_connect(decoder_, "caps-fix-error", G_CALLBACK(&PlayerCodecCtrl::CapsFixErrorCb), this);
}

void PlayerCodecCtrl::CapsFixErrorCb(const GstElement *decoder, gpointer userData)
{
    MEDIA_LOGD("CapsFixErrorCb in");
    if (decoder == nullptr || userData == nullptr) {
        MEDIA_LOGE("param is nullptr");
        return;
    }

    auto playerCodecCtrl = static_cast<PlayerCodecCtrl *>(userData);
    if (playerCodecCtrl->notifier_ != nullptr) {
        playerCodecCtrl->notifier_();
    }
}

void PlayerCodecCtrl::DetectCodecUnSetup(GstElement *src, GstElement *videoSink)
{
    std::lock_guard<std::mutex> lock(mutex_);
    (void)src;
    MEDIA_LOGD("Codec UnSetup");
    if (decoder_ != nullptr) {
        gst_object_unref(decoder_);
        decoder_ = nullptr;
    }
    HlsSwichSoftAndHardCodec(videoSink);
}

void PlayerCodecCtrl::HlsSwichSoftAndHardCodec(GstElement *videoSink)
{
    if (codecTypeList_.empty()) {
        MEDIA_LOGE("codec type list is empty");
        return;
    }

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
    g_object_set(G_OBJECT(videoSink), "caps", caps, nullptr);
    gst_caps_unref(caps);
}

void PlayerCodecCtrl::EnhanceSeekPerformance(bool enable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isHardwareDec_ && decoder_ != nullptr) {
        g_object_set(decoder_, "seeking", enable, nullptr);
    }
}
} // Media
} // OHOS