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

#include "player_sinkprovider.h"
#include "securec.h"
#include "display_type.h"
#include "param_wrapper.h"
#include "gst/video/gstvideometa.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_dfx.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerSinkProvider"};
    constexpr uint32_t DEFAULT_BUFFER_NUM = 8;
}

namespace OHOS {
namespace Media {
PlayerSinkProvider::PlayerSinkProvider(const sptr<Surface> &surface)
    : producerSurface_(surface)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerSinkProvider::~PlayerSinkProvider()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));

    producerSurface_ = nullptr;
    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    if (videoSink_ != nullptr) {
        gst_object_unref(videoSink_);
        videoSink_ = nullptr;
    }
    if (subtitleSink_ != nullptr) {
        gst_object_unref(subtitleSink_);
        subtitleSink_ = nullptr;
    }
    if (audioCaps_ != nullptr) {
        gst_caps_unref(audioCaps_);
        audioCaps_ = nullptr;
    }
    if (videoCaps_ != nullptr) {
        gst_caps_unref(videoCaps_);
        videoCaps_ = nullptr;
    }
}

PlayBinSinkProvider::SinkPtr PlayerSinkProvider::CreateAudioSink()
{
    constexpr gint rate = 44100;
    constexpr gint channels = 2;

    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    if (audioCaps_ == nullptr) {
        audioCaps_ = gst_caps_new_simple("audio/x-raw",
                                         "format", G_TYPE_STRING, "S16LE",
                                         "rate", G_TYPE_INT, rate,
                                         "channels", G_TYPE_INT, channels, nullptr);
        CHECK_AND_RETURN_RET_LOG(audioCaps_ != nullptr, nullptr, "gst_caps_new_simple failed..");

        audioSink_ = DoCreateAudioSink(audioCaps_, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(audioSink_ != nullptr, nullptr, "CreateAudioSink failed..");
    }

    return audioSink_;
}

bool PlayerSinkProvider::EnableOptRenderDelay() const
{
    std::string enable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.kpi.opt.renderdelay.enable", enable, "");
    if (res != 0 || enable.empty()) {
        MEDIA_LOGW("0x%{public}06" PRIXPTR " KPI-TRACE: get value fail, default enable", FAKE_POINTER(this));
        return true;
    }

    MEDIA_LOGI("KPI-TRACE: sys.media.kpi.opt.renderdelay.enable=%{public}s", enable.c_str());
    if (enable != "true") {
        return false;
    }
    return true;
}

GstElement *PlayerSinkProvider::DoCreateAudioSink(const GstCaps *caps, const gpointer userData)
{
    (void)caps;
    MEDIA_LOGI("0x%{public}06" PRIXPTR " CreateAudioSink in.", FAKE_POINTER(this));
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");

    auto sink = GST_ELEMENT_CAST(gst_object_ref_sink(gst_element_factory_make("audioserversink", nullptr)));
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    g_object_set(G_OBJECT(sink), "app-uid", uid_, nullptr);
    g_object_set(G_OBJECT(sink), "app-pid", pid_, nullptr);
    g_object_set(G_OBJECT(sink), "app-token-id", tokenId_, nullptr);

    gboolean enable = static_cast<gboolean>(EnableOptRenderDelay());
    g_object_set(G_OBJECT(sink), "enable-opt-render-delay", enable, nullptr);

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    if (pad == nullptr) {
        gst_object_unref(sink);
        MEDIA_LOGE("gst_element_get_static_pad failed..");
        return nullptr;
    }

    (void)gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
        PlayerSinkProvider::SinkPadProbeCb, userData, nullptr);
    gst_object_unref(pad);
    return sink;
}

bool PlayerSinkProvider::EnableKpiAVSyncLog() const
{
    std::string enable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.kpi.avsync.log.enable", enable, "");
    if (res != 0 || enable.empty()) {
        return false;
    }

    MEDIA_LOGI("KPI-TRACE: sys.media.kpi.avsync.log.enable=%{public}s", enable.c_str());
    if (enable != "true") {
        return false;
    }
    return true;
}

PlayBinSinkProvider::SinkPtr PlayerSinkProvider::CreateVideoSink()
{
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, nullptr,
        "producerSurface_ is nullptr, cannot create video sink!");
    if (videoCaps_ == nullptr) {
        videoCaps_ = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "RGBA", nullptr);
        CHECK_AND_RETURN_RET_LOG(videoCaps_ != nullptr, nullptr, "gst_caps_new_simple failed..");

        videoSink_ = DoCreateVideoSink(videoCaps_, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, nullptr, "CreateVideoSink failed..");
    }

    if (audioSink_ != nullptr) {
        MEDIA_LOGI("0x%{public}06" PRIXPTR " KPI-TRACE: set audio sink to video sink", FAKE_POINTER(this));
        gboolean enable = static_cast<gboolean>(EnableKpiAVSyncLog());
        g_object_set(G_OBJECT(videoSink_), "audio-sink", audioSink_, "enable-kpi-avsync-log", enable, nullptr);
    }

    (void)producerSurface_->SetQueueSize(DEFAULT_BUFFER_NUM);
    queueSize_ = DEFAULT_BUFFER_NUM;

    return videoSink_;
}

GstElement *PlayerSinkProvider::DoCreateVideoSink(const GstCaps *caps, const gpointer userData)
{
    MEDIA_LOGD("CreateVideoSink in.");
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "input caps is nullptr..");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");
    PlayerSinkProvider *sinkProvider = reinterpret_cast<PlayerSinkProvider *>(userData);

    auto sink = GST_ELEMENT_CAST(gst_object_ref_sink(gst_element_factory_make("videodisplaysink", "sink")));
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");
    gst_base_sink_set_async_enabled(GST_BASE_SINK(sink), FALSE);

    g_object_set(G_OBJECT(sink), "caps", caps, nullptr);
    g_object_set(G_OBJECT(sink), "surface", static_cast<gpointer>(sinkProvider->GetProducerSurface()), nullptr);
    g_object_set(G_OBJECT(sink), "video-scale-type", videoScaleType_, nullptr);

    GstMemSinkCallbacks sinkCallbacks = { PlayerSinkProvider::EosCb, PlayerSinkProvider::NewPrerollCb,
        PlayerSinkProvider::NewSampleCb };
    gst_mem_sink_set_callback(GST_MEM_SINK(sink), &sinkCallbacks, userData, nullptr);

    return sink;
}

PlayBinSinkProvider::SinkPtr PlayerSinkProvider::GetVideoSink()
{
    CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, nullptr, "videoSink is nullptr");
    return GST_ELEMENT_CAST(videoSink_);
}

void PlayerSinkProvider::FirstRenderFrame(gpointer userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "input userData is nullptr..");
    PlayerSinkProvider *sinkProvider = reinterpret_cast<PlayerSinkProvider *>(userData);

    sinkProvider->OnFirstRenderFrame();
}

void PlayerSinkProvider::OnFirstRenderFrame()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN(notifier_ != nullptr);
    if (GetFirstRenderFrameFlag()) {
        PlayBinMessage msg { PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_VIDEO_RENDERING_START, 0, {} };
        notifier_(msg);
        SetFirstRenderFrameFlag(false);
        MEDIA_LOGD("KPI-TRACE: FIRST-VIDEO-FRAME rendered");
    }
}

PlayBinSinkProvider::SinkPtr PlayerSinkProvider::CreateSubtitleSink()
{
    if (subtitleSink_ != nullptr) {
        gst_object_unref(subtitleSink_);
        subtitleSink_ = nullptr;
    }
    subtitleSink_ = DoCreateSubtitleSink(reinterpret_cast<gpointer>(this));
    CHECK_AND_RETURN_RET_LOG(subtitleSink_ != nullptr, nullptr, "CreateSubtitleSink failed..");
    g_object_set(G_OBJECT(subtitleSink_), "audio-sink", audioSink_, nullptr);
    return subtitleSink_;
}

GstElement *PlayerSinkProvider::DoCreateSubtitleSink(const gpointer userData)
{
    MEDIA_LOGI("CreateSubtitleSink in.");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");

    auto sink = GST_ELEMENT_CAST(gst_object_ref_sink(gst_element_factory_make("subtitledisplaysink", nullptr)));
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    GstSubtitleSinkCallbacks sinkCallbacks = { PlayerSinkProvider::SubtitleUpdated };
    gst_subtitle_sink_set_callback(GST_SUBTITLE_SINK(sink), &sinkCallbacks, userData, nullptr);
    MEDIA_LOGI("CreateSubtitleSink out.");
    return sink;
}

void PlayerSinkProvider::HandleSubtitleBuffer(GstBuffer *sample, Format &subtitle)
{
    if (sample == nullptr) {
        (void)subtitle.PutStringValue(PlayerKeys::SUBTITLE_TEXT, "");
        return;
    }
    GstMapInfo mapInfo;
    CHECK_AND_RETURN(gst_buffer_map(sample, &mapInfo, GST_MAP_READ));
    uint32_t gstBufferSize = static_cast<uint32_t>(gst_buffer_get_size(sample));
    char *textFrame = new (std::nothrow) char[gstBufferSize + 1];
    (void)memcpy_s(textFrame, gstBufferSize + 1, mapInfo.data, gstBufferSize);
    textFrame[gstBufferSize] = static_cast<char>(0);
    (void)subtitle.PutStringValue(PlayerKeys::SUBTITLE_TEXT, std::string_view(textFrame));
    MEDIA_LOGD("text = %{public}s", textFrame);
    delete[] textFrame;
    gst_buffer_unmap(sample, &mapInfo);
}

GstFlowReturn PlayerSinkProvider::SubtitleUpdated(GstBuffer *sample, gpointer userData)
{
    MediaTrace trace("PlayerSinkProvider::SubtitleUpdated");
    CHECK_AND_RETURN_RET(userData != nullptr, GST_FLOW_ERROR);
    PlayerSinkProvider *sinkProvider = reinterpret_cast<PlayerSinkProvider *>(userData);
    Format subtitle;
    sinkProvider->HandleSubtitleBuffer(sample, subtitle);
    sinkProvider->OnSubtitleUpdated(subtitle);
    return GST_FLOW_OK;
}

void PlayerSinkProvider::OnSubtitleUpdated(const Format &subtitle)
{
    std::unique_lock<std::mutex> lock(mutex_);
    MEDIA_LOGD("OnSubtitleUpdated enter");
    auto temp_notifier = notifier_;
    lock.unlock();
    CHECK_AND_RETURN(temp_notifier != nullptr);
    PlayBinMessage msg = {PLAYBIN_MSG_SUBTYPE, PLAYBIN_SUB_MSG_SUBTITLE_UPDATED, 0, subtitle};
    temp_notifier(msg);
    MEDIA_LOGD("OnSubtitleUpdated exit");
}

void PlayerSinkProvider::EosCb(GstMemSink *memSink, gpointer userData)
{
    (void)memSink;
    (void)userData;
    MEDIA_LOGD("EOS in");
}

GstFlowReturn PlayerSinkProvider::NewPrerollCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData)
{
    (void)userData;
    MEDIA_LOGD("NewPrerollCb in");
    MediaTrace trace("PlayerSinkProvider::NewPrerollCb");
    CHECK_AND_RETURN_RET(gst_mem_sink_app_preroll_render(memSink, sample) == GST_FLOW_OK, GST_FLOW_ERROR);
    return GST_FLOW_OK;
}

GstFlowReturn PlayerSinkProvider::NewSampleCb(GstMemSink *memSink, GstBuffer *sample, gpointer userData)
{
    MEDIA_LOGD("NewSampleCb in");
    MediaTrace trace("PlayerSinkProvider::NewSampleCb");
    CHECK_AND_RETURN_RET(gst_mem_sink_app_render(memSink, sample) == GST_FLOW_OK, GST_FLOW_ERROR);

    FirstRenderFrame(userData);
    return GST_FLOW_OK;
}

GstPadProbeReturn PlayerSinkProvider::SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    (void)pad;
    (void)userData;
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
        GstCaps *caps = nullptr;
        gboolean needPool;
        gst_query_parse_allocation(query, &caps, &needPool);

        auto s = gst_caps_get_structure(caps, 0);
        auto mediaType = gst_structure_get_name(s);
        gboolean isVideo = g_str_has_prefix(mediaType, "video/");
        if (isVideo) {
            gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);
        }
    }
    return GST_PAD_PROBE_OK;
}

void PlayerSinkProvider::SetMsgNotifier(PlayBinMsgNotifier notifier)
{
    std::unique_lock<std::mutex> lock(mutex_);
    notifier_ = notifier;
}

void PlayerSinkProvider::SetFirstRenderFrameFlag(bool firstRenderFrame)
{
    firstRenderFrame_ = firstRenderFrame;
}

bool PlayerSinkProvider::GetFirstRenderFrameFlag() const
{
    return firstRenderFrame_;
}

const sptr<Surface> PlayerSinkProvider::GetProducerSurface() const
{
    return producerSurface_;
}

void PlayerSinkProvider::SetVideoScaleType(const uint32_t videoScaleType)
{
    if (videoSink_ != nullptr) {
        g_object_set(videoSink_, "video-scale-type", videoScaleType, nullptr);
    } else {
        videoScaleType_ = videoScaleType;
    }
}

void PlayerSinkProvider::SetAppInfo(int32_t uid, int32_t pid, uint32_t tokenId)
{
    uid_ = uid;
    pid_ = pid;
    tokenId_ = tokenId;
}
}
}
