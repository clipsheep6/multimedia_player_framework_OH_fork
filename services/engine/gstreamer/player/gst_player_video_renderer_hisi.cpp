/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
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

#include "gst_player_video_renderer_hisi.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sync_fence.h>
#include "securec.h"
#include "string_ex.h"
#include "display_type.h"
#include "media_log.h"
#include "param_wrapper.h"
#include "media_errors.h"
#include "gstinfo.h"
#include "gst_player_video_renderer_allcator.h"
//#include "display_gralloc_private.h"
#include "gstbuffer.h"
#include "gstvsfsyncmeta.h"
#include "gstmeta.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerVideoRendererCtrl"};
    constexpr uint32_t MAX_DEFAULT_WIDTH = 10000;
    constexpr uint32_t MAX_DEFAULT_HEIGHT = 10000;
}

namespace OHOS {
namespace Media {
const std::string SURFACE_TIME_TAG = "UpdateSurfaceBuffer";
class GstPlayerVideoRendererCap {
public:
    GstPlayerVideoRendererCap() = delete;
    ~GstPlayerVideoRendererCap() = delete;
    static GstElement *CreateSink(GstPlayerVideoRenderer *renderer, GstPlayer *player);
    using DataAvailableFunc = GstFlowReturn (*)(const GstElement *appsink, gpointer userData);
    static GstElement *CreateAudioSink(const GstCaps *caps, const DataAvailableFunc callback, const gpointer userData);
    static GstElement *CreateVideoSink(const GstCaps *caps, const DataAvailableFunc callback, const gpointer userData,
        gulong &signalId);
    static GstFlowReturn VideoDataAvailableCb(const GstElement *appsink, const gpointer userData);
    static GstPadProbeReturn SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData);
    static void EosCb(const GstElement *appsink, gpointer userData);
};

struct _PlayerVideoRenderer {
    GObject parent;
    GstPlayerVideoRendererCtrl *rendererCtrl;
};

static void player_video_renderer_interface_init(GstPlayerVideoRendererInterface *iface)
{
    MEDIA_LOGI("Video renderer interface init");
    iface->create_video_sink = GstPlayerVideoRendererCap::CreateSink;
}

#define PLAYER_TYPE_VIDEO_RENDERER player_video_renderer_get_type()
    G_DECLARE_FINAL_TYPE(PlayerVideoRenderer, player_video_renderer, PLAYER, VIDEO_RENDERER, GObject)

G_DEFINE_TYPE_WITH_CODE(PlayerVideoRenderer, player_video_renderer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(GST_TYPE_PLAYER_VIDEO_RENDERER, player_video_renderer_interface_init));

static void player_video_renderer_init(PlayerVideoRenderer *self)
{
    (void)self;
}

static void player_video_renderer_class_init(PlayerVideoRendererClass *klass)
{
    (void)klass;
}

static GstPlayerVideoRenderer *player_video_renderer_new(
    const std::shared_ptr<GstPlayerVideoRendererCtrl> &rendererCtrl)
{
    MEDIA_LOGI("enter");
    (void)PLAYER_VIDEO_RENDERER(nullptr);
    (void)PLAYER_IS_VIDEO_RENDERER(nullptr);
    PlayerVideoRenderer *self = (PlayerVideoRenderer *)g_object_new(PLAYER_TYPE_VIDEO_RENDERER, nullptr);
    CHECK_AND_RETURN_RET_LOG(self != nullptr, nullptr, "g_object_new failed..");

    self->rendererCtrl = rendererCtrl.get();
    return reinterpret_cast<GstPlayerVideoRenderer *>(self);
}

GstElement *GstPlayerVideoRendererCap::CreateSink(GstPlayerVideoRenderer *renderer, GstPlayer *player)
{
    MEDIA_LOGI("CreateSink in.");
    CHECK_AND_RETURN_RET_LOG(renderer != nullptr, nullptr, "renderer is nullptr..");
    GstPlayerVideoRendererCtrl *userData = (reinterpret_cast<PlayerVideoRenderer *>(renderer))->rendererCtrl;
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "userData is nullptr..");

    GstElement *playbin = gst_player_get_pipeline(player);
    CHECK_AND_RETURN_RET_LOG(playbin != nullptr, nullptr, "playbin is nullptr..");

    (void)userData->InitAudioSink(playbin);
    if (userData->GetProducerSurface() != nullptr) {
        (void)userData->InitVideoSink(playbin);
    }

    gst_object_unref(playbin);
    if (userData->GetProducerSurface() != nullptr) {
        return const_cast<GstElement *>(userData->GetVideoSink());
    } else {
        return const_cast<GstElement *>(userData->GetAudioSink());
    }
}

GstElement *GstPlayerVideoRendererCap::CreateAudioSink(const GstCaps *caps,
    const DataAvailableFunc callback, const gpointer userData)
{
    (void)callback;
    (void)caps;
    MEDIA_LOGI("CreateAudioSink in.");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");

    auto sink = gst_element_factory_make("audioserversink", nullptr);
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    if (pad == nullptr) {
        gst_object_unref(sink);
        MEDIA_LOGE("gst_element_get_static_pad failed..");
        return nullptr;
    }

    (void)gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, SinkPadProbeCb, userData, nullptr);
    return sink;
}

GstElement *GstPlayerVideoRendererCap::CreateVideoSink(const GstCaps *caps,
    const DataAvailableFunc callback, const gpointer userData, gulong &signalId)
{
    MEDIA_LOGI("CreateVideoSink in.");
    CHECK_AND_RETURN_RET_LOG(caps != nullptr, nullptr, "input caps is nullptr..");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, nullptr, "input callback is nullptr..");
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, nullptr, "input userData is nullptr..");
    auto sink = gst_element_factory_make("appsink", nullptr);
    CHECK_AND_RETURN_RET_LOG(sink != nullptr, nullptr, "gst_element_factory_make failed..");

    signalId = g_signal_connect(G_OBJECT(sink), "new_sample", G_CALLBACK(callback), userData);
    g_object_set(G_OBJECT(sink), "caps", caps, nullptr);
    g_object_set(G_OBJECT(sink), "emit-signals", TRUE, nullptr);

    GstPad *pad = gst_element_get_static_pad(sink, "sink");
    CHECK_AND_RETURN_RET_LOG(pad != nullptr, nullptr, "gst_element_get_static_pad failed..");

    (void)gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, SinkPadProbeCb, userData, nullptr);

    return sink;
}

GstFlowReturn GstPlayerVideoRendererCap::VideoDataAvailableCb(const GstElement *appsink, const gpointer userData)
{
    (void)appsink;
    CHECK_AND_RETURN_RET_LOG(userData != nullptr, GST_FLOW_ERROR, "userData is nullptr..");
    auto ctrl = reinterpret_cast<GstPlayerVideoRendererCtrl *>(userData);
    int32_t ret = ctrl->PullVideoBuffer();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Failed to PullVideoBuffer!");
    }
    return GST_FLOW_OK;
}

void GstPlayerVideoRendererCap::EosCb(const GstElement *appsink, gpointer userData)
{
    (void)appsink;
    (void)userData;
    MEDIA_LOGI("EOS in");
}

GstPadProbeReturn GstPlayerVideoRendererCap::SinkPadProbeCb(GstPad *pad, GstPadProbeInfo *info, gpointer userData)
{
    (void)pad;
    (void)userData;
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    GST_INFO("%s:%d, query %" GST_PTR_FORMAT, __func__, __LINE__, query);

    if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
        GstCaps *caps = nullptr;
        gboolean needPool = true;
        gst_query_parse_allocation(query, &caps, &needPool);
        auto s = gst_caps_get_structure(caps, 0);
        GstPlayerVideoRendererCtrl *vsink = (GstPlayerVideoRendererCtrl *)userData;
        sptr<Surface> surface;
        vsink->GetSurfaceProducer(surface);

        auto mediaType = gst_structure_get_name(s);
        gboolean isVideo = g_str_has_prefix(mediaType, "video/");

        if (surface != nullptr && isVideo) {
            uint32_t bufferNum = 0;
            gst_structure_get_uint (s, "buffernum", &bufferNum);
            vsink->SetBufferNum(bufferNum);
            printf("[%s :%d] zsy##### bufferNum:%d\n", __func__, __LINE__, bufferNum);
            gint width = 0;
            gint height = 0;
            gst_structure_get_int (s, "width", &width);
            gst_structure_get_int (s, "height", &height);
            vsink->SetSurfaceBufferWidthAndHeight((uint32_t)width, (uint32_t)height);

            GstAllocationParams params;
            params.flags = (GstMemoryFlags)0;//GST_MEMORY_FLAG_LAST;
            params.align = 63;
            params.prefix = 0;
            params.padding = 0;
            GstAllocator *allocator = (GstAllocator*)gst_allcator_surfacebuf_new(vsink, surface);
            if(allocator != nullptr){
              gst_query_add_allocation_param (query, allocator, &params);
              GST_INFO("after setting query %" GST_PTR_FORMAT , query);
            }else{
              GST_ERROR("get allocator failed, allocate output mem failed!");
            }
        }
    }
    return GST_PAD_PROBE_OK;
}

GstPlayerVideoRendererCtrl::GstPlayerVideoRendererCtrl(const sptr<Surface> &surface)
    : producerSurface_(surface),
      surfaceTimeMonitor_(SURFACE_TIME_TAG),
      bufferNum_(0xff)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    SetSurfaceTimeFromSysPara();
    g_queue_init(&memBuf_);
    g_mutex_init(&memBufMuxtex_);
#ifdef DUMP_YUV
    file_ = fopen("/data/dump.yuv", "wb");
#endif
}

void GstPlayerVideoRendererCtrl::CancelAllSurfaceBuffer(void)
{
    MEDIA_LOGI("enter,%s:%d.", __func__, __LINE__);
    if (producerSurface_ == nullptr) {
        MEDIA_LOGE("%s:%d.  producerSurface_ nullptr", __func__, __LINE__);
        return;
    }
    int32_t len = g_queue_get_length(&memBuf_);
    for(int32_t idx = 0; idx < len; idx++){
        GstMemInfo *item = (GstMemInfo *)g_queue_peek_nth(&memBuf_, idx);
        if(item == nullptr || item->surfaceBufffer_ == nullptr) {
            continue;
        }
        producerSurface_->CancelBuffer(item->surfaceBufffer_);
        item->surfaceBufffer_ = nullptr;
        g_free(item);
        item = nullptr;
    }
    g_queue_clear(&memBuf_);
    MEDIA_LOGI("exit,%s:%d.", __func__, __LINE__);
}

GstPlayerVideoRendererCtrl::~GstPlayerVideoRendererCtrl()
{
    g_signal_handler_disconnect(G_OBJECT(videoSink_), signalId_);
    for (auto signalId : signalIds_) {
        g_signal_handler_disconnect(G_OBJECT(videoSink_), signalId);
    }

    if (videoSink_ != nullptr) {
        gst_object_unref(videoSink_);
        videoSink_ = nullptr;
        dequeueThdExit_ = TRUE;
        if (dequeueThd_ != 0) {
            g_thread_join (dequeueThd_);
            dequeueThd_ = 0;
        }
    }
    if (audioSink_ != nullptr) {
        gst_object_unref(audioSink_);
        audioSink_ = nullptr;
    }
    if (audioCaps_ != nullptr) {
        gst_caps_unref(audioCaps_);
        audioCaps_ = nullptr;
    }
    if (videoCaps_ != nullptr) {
        gst_caps_unref(videoCaps_);
        videoCaps_ = nullptr;
    }

    CancelAllSurfaceBuffer();
    g_mutex_clear(&memBufMuxtex_);
    producerSurface_.clear();
    producerSurface_ = nullptr;
#ifdef DUMP_YUV
    if (file_ != nullptr) {
        fflush(file_);
        fclose(file_);
        file_ = nullptr;
    }
#endif
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

const GstElement *GstPlayerVideoRendererCtrl::GetVideoSink() const
{
    return videoSink_;
}

#define ALLOCTOR_STRIDE_VDP(w) ((((w) % 256) == 0) ? (w) : (((((w) / 256) % 2) == 0) ? \
  ((((w) / 256) + 1) * 256) : ((((w) / 256) + 2) * 256)))

#define ALLOCTOR_ROUND_UP_128(num) (((num)+127)&~127)
#define ALLOCTOR_ROUND_UP_64(num) (((num)+63)&~63)
#define ALLOCTOR_ROUND_UP_16(num) (((num)+15)&~15)

void *GstPlayerVideoRendererCtrl::DequeueThdFunc (void *data)
{
    MEDIA_LOGI("%{public}s:%{public}d. enter, data:%{public}p", __func__, __LINE__, data);
    printf("[%s :%d] zsy##### DequeueThdFunc in\n", __func__, __LINE__);
    GstPlayerVideoRendererCtrl *vsink = (GstPlayerVideoRendererCtrl*)data;
    if (vsink == nullptr) {
        GST_ERROR_OBJECT(vsink, "vsink is null!");
        printf("[%s :%d] zsy##### DequeueThdFunc\n", __func__, __LINE__);
        return nullptr;
    }
    printf("[%s :%d] zsy##### DequeueThdFunc\n", __func__, __LINE__);
    sptr<Surface> surface;
    vsink->GetSurfaceProducer(surface);
    if (surface == nullptr) {
        MEDIA_LOGE("%s:%d.  Surface is nullptr.Video cannot be played.", __func__, __LINE__);
        printf("[%s :%d] zsy##### DequeueThdFunc\n", __func__, __LINE__);
        return nullptr;
    }
    int64_t cnt = 0;
    printf("[%s :%d] zsy##### DequeueThdFunc\n", __func__, __LINE__);
    while(!vsink->dequeueThdExit_){
        g_mutex_lock(&vsink->memBufMuxtex_);
        int32_t len = g_queue_get_length(&vsink->memBuf_);
        g_mutex_unlock(&vsink->memBufMuxtex_);
        if (len < static_cast<int32_t>(vsink->bufferNum_)) {
            g_usleep(1000000);
            cnt++;
            if (cnt < 100 ) {
                printf("[%s :%d] zsy##### DequeueThdFunc len:%d, vsink->bufferNum_:%d\n",
                    __func__, __LINE__, len, vsink->bufferNum_);
            }
            continue;
        }
        if (vsink->cancelBuffer_) {
            MEDIA_LOGE("%{public}s:%{public}d. cancel buffer process", __func__, __LINE__);
            break;
        }
        int32_t releaseFence = -1;
        sptr<SurfaceBuffer> surfaceBuffer;
        BufferRequestConfig requestConfig = {
            .width = ALLOCTOR_STRIDE_VDP(vsink->width_),
            .height = ALLOCTOR_ROUND_UP_64(vsink->height_),
            .strideAlignment = 8,
            .format = PIXEL_FMT_YCRCB_420_SP,
            .usage = HBM_USE_MEM_DMA | HBM_USE_CPU_READ | HBM_USE_CPU_WRITE,
            .timeout = 0,
        };
        g_mutex_lock(&vsink->memBufMuxtex_);
        //printf("[%s :%d] zsy##### DequeueThdFunc\n", __func__, __LINE__);
        vsink->producerSurface_->RequestBuffer(surfaceBuffer, releaseFence, requestConfig);
        if (surfaceBuffer == nullptr) {
            g_mutex_unlock(&vsink->memBufMuxtex_);
            g_usleep(20000);
            continue;
        }
        //printf("[%s :%d] zsy##### RequestBuffer:%p\n", __func__, __LINE__, surfaceBuffer->GetBufferHandle());
        if (releaseFence != -1) {
            OHOS::sptr<OHOS::SyncFence> autoFence = new(std::nothrow) OHOS::SyncFence(releaseFence);
            if (autoFence != nullptr) {
                autoFence->Wait(100); // 100ms
            }
        }
        BufferHandle *bufHandle = surfaceBuffer->GetBufferHandle();
        for(int32_t idx = 0; idx < len; idx++){
            GstMemInfo *item = (GstMemInfo *)g_queue_peek_nth(&vsink->memBuf_, idx);
            if(item == nullptr) {
                continue;
            }
            if(item->bufHdl_ == bufHandle) {
                item->fence_ = releaseFence;
                item->surfaceBufffer_ = surfaceBuffer;
                printf("[%s :%d] zsy##### handle:%p, func:%p,context:%p w-h:%d-%d\n",
                  __func__, __LINE__, bufHandle, item->release_func, item->port_context,
                  requestConfig.width, requestConfig.height);
                PGST_OUTPUT_BUFFER_RELEASE gst_output_buffer_release = (PGST_OUTPUT_BUFFER_RELEASE)(item->release_func);
                gst_output_buffer_release(item->port_context, item->buffer);
                break;
            }
        }
        g_mutex_unlock(&vsink->memBufMuxtex_);
    }
    MEDIA_LOGI("DequeueThdFunc out.");
    printf("[%s :%d] zsy##### DequeueThdFunc out\n", __func__, __LINE__);
    return nullptr;
}

const GstElement *GstPlayerVideoRendererCtrl::GetAudioSink() const
{
    return audioSink_;
}

int32_t GstPlayerVideoRendererCtrl::InitVideoSink(const GstElement *playbin)
{
    if (videoCaps_ == nullptr) {
        videoCaps_ = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "NV21", nullptr);
        CHECK_AND_RETURN_RET_LOG(videoCaps_ != nullptr, MSERR_INVALID_OPERATION, "gst_caps_new_simple failed..");

        videoSink_ = GstPlayerVideoRendererCap::CreateVideoSink(videoCaps_,
            GstPlayerVideoRendererCap::VideoDataAvailableCb, reinterpret_cast<gpointer>(this), signalId_);
        CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, MSERR_INVALID_OPERATION, "CreateVideoSink failed..");

        g_object_set(const_cast<GstElement *>(playbin), "video-sink", videoSink_, nullptr);
    }

    dequeueThdExit_ = FALSE;
    dequeueThd_ = g_thread_new("vsink_buffer_thd", DequeueThdFunc, this);
    MEDIA_LOGI("%s:%d. out", __func__, __LINE__);

    return MSERR_OK;
}

std::string GstPlayerVideoRendererCtrl::GetVideoSinkFormat() const
{
    std::string formatName = "NV21";
    if (producerSurface_ != nullptr) {
        const std::string surfaceFormat = "SURFACE_FORMAT";
        std::string format = producerSurface_->GetUserData(surfaceFormat);
        MEDIA_LOGE("surfaceFormat is %{public}s!", format.c_str());
        if (format == std::to_string(PIXEL_FMT_RGBA_8888)) {
            formatName = "RGBA";
        } else {
            formatName = "NV21";
        }
    }
    MEDIA_LOGI("gst_caps_new_simple format is %{public}s!", formatName.c_str());
    return formatName;
}

const sptr<Surface> GstPlayerVideoRendererCtrl::GetProducerSurface() const
{
    return producerSurface_;
}

int32_t GstPlayerVideoRendererCtrl::InitAudioSink(const GstElement *playbin)
{
    constexpr gint rate = 44100;
    constexpr gint channels = 2;
    if (audioCaps_ == nullptr) {
        audioCaps_ = gst_caps_new_simple("audio/x-raw",
                                         "format", G_TYPE_STRING, "S16LE",
                                         "rate", G_TYPE_INT, rate,
                                         "channels", G_TYPE_INT, channels, nullptr);
        CHECK_AND_RETURN_RET_LOG(audioCaps_ != nullptr, MSERR_INVALID_OPERATION, "gst_caps_new_simple failed..");

        audioSink_ = GstPlayerVideoRendererCap::CreateAudioSink(audioCaps_, nullptr, reinterpret_cast<gpointer>(this));
        CHECK_AND_RETURN_RET_LOG(audioSink_ != nullptr, MSERR_INVALID_OPERATION, "CreateAudioSink failed..");

        g_object_set(const_cast<GstElement *>(playbin), "audio-sink", audioSink_, nullptr);
    }
    return MSERR_OK;
}

int32_t GstPlayerVideoRendererCtrl::PullVideoBuffer()
{
    CHECK_AND_RETURN_RET_LOG(videoSink_ != nullptr, MSERR_INVALID_OPERATION, "videoSink_ is nullptr..");

    GstSample *sample = nullptr;
    g_signal_emit_by_name(G_OBJECT(videoSink_), "pull-sample", &sample);
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, MSERR_INVALID_OPERATION, "sample is nullptr..");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (buf == nullptr) {
        MEDIA_LOGE("gst_sample_get_buffer err");
        gst_sample_unref(sample);
        return MSERR_INVALID_OPERATION;
    }

    int32_t ret = UpdateSurfaceBuffer(*buf);
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Failed to update surface buffer and please provide the sptr<Surface>!");
    }

    gst_sample_unref(sample);
    return ret;
}

void GstPlayerVideoRendererCtrl::SetSurfaceTimeFromSysPara()
{
    std::string timeEnable;
    int32_t res = OHOS::system::GetStringParameter("sys.media.time.surface", timeEnable, "");
    if (res != 0 || timeEnable.empty()) {
        surfaceTimeEnable = false;
        MEDIA_LOGD("sys.media.time.surface=false");
        return;
    }
    MEDIA_LOGD("sys.media.time.surface=%{public}s", timeEnable.c_str());

    if (timeEnable == "true") {
        surfaceTimeEnable = true;
    }
}

void GstPlayerVideoRendererCtrl::UpdateResquestConfig(BufferRequestConfig &requestConfig,
    const GstVideoMeta *videoMeta) const
{
    requestConfig.width = static_cast<int32_t>(videoMeta->width);
    requestConfig.height = static_cast<int32_t>(videoMeta->height);
    constexpr int32_t strideAlignment = 8;
    const std::string surfaceFormat = "SURFACE_FORMAT";
    requestConfig.strideAlignment = strideAlignment;
    std::string format = producerSurface_->GetUserData(surfaceFormat);
    if (format == std::to_string(PIXEL_FMT_RGBA_8888)) {
        requestConfig.format = PIXEL_FMT_RGBA_8888;
    } else {
        requestConfig.format = PIXEL_FMT_YCRCB_420_SP;
    }
    requestConfig.usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
    requestConfig.timeout = 0;
}

int32_t GstPlayerVideoRendererCtrl::UpdateSurfaceBuffer(const GstBuffer &buffer)
{
    printf("[%s :%d] zsy##### UpdateSurfaceBuffer\n", __func__, __LINE__);
    CHECK_AND_RETURN_RET_LOG(producerSurface_ != nullptr, MSERR_INVALID_OPERATION,
        "Surface is nullptr.Video cannot be played.");
    if (surfaceTimeEnable) {
        surfaceTimeMonitor_.StartTime();
    }
    GstBuffer *buf = const_cast<GstBuffer *>(&buffer);

    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(buf);
    CHECK_AND_RETURN_RET_LOG(videoMeta != nullptr, MSERR_INVALID_VAL, "gst_buffer_get_video_meta failed..");
    CHECK_AND_RETURN_RET_LOG(videoMeta->width < MAX_DEFAULT_WIDTH && videoMeta->height < MAX_DEFAULT_HEIGHT,
        MSERR_INVALID_VAL, "Surface is nullptr.Video cannot be played.");

    gsize size = gst_buffer_get_size(buf);
    CHECK_AND_RETURN_RET_LOG(size > 0, MSERR_INVALID_VAL, "gst_buffer_get_size failed..");

    GstMapInfo info;
    if(!gst_buffer_map ((GstBuffer *)&buffer, &info, GST_MAP_READ)){
        return GST_FLOW_ERROR;
    }

    PrivateHandle handle = (PrivateHandle)info.data;
    GstVsfSyncMeta *vsfMeta = gst_buffer_get_vsf_sync_meta(buf);
    if (vsfMeta == nullptr) {
      GST_ERROR("Error, can't get vsf meta from gstbuffer.");
      printf("[%s :%d] zsy##### vsfMeta null\n", __func__, __LINE__);
    }
    g_mutex_lock(&memBufMuxtex_);
    guint queueLen = g_queue_get_length(&memBuf_);
    guint i = 0;
    GstMemInfo *target = nullptr;
    for(i = 0; i < queueLen; i++){
        GstMemInfo *item = (GstMemInfo *)g_queue_peek_nth(&memBuf_, i);
        if(item == nullptr) {
            GST_ERROR("peek GstVideoMmzVoFlag :%d is null, why?", i);
            continue;
        }
        /* if it is tvp, vdec pass phyaddr,otherwise pass virtual addr, Now, we not support tvp */
        if(item->priHandle_ == handle){
            target = item;
            break;
        }
    }
    g_mutex_unlock(&memBufMuxtex_);
    if (target == nullptr) {
        return GST_FLOW_ERROR;
    }

    if (vsfMeta != nullptr) {
        target->release_func = vsfMeta->release_func;
        target->port_context = vsfMeta->port_context;
        target->buffer = vsfMeta->buffer;
    }

    BufferFlushConfig flushConfig = {};
    flushConfig.damage.x = 0;
    flushConfig.damage.y = 0;
    flushConfig.damage.w = static_cast<int32_t>(videoMeta->width);
    flushConfig.damage.h = static_cast<int32_t>(videoMeta->height);
#ifdef DUMP_YUV
    if (file_ != nullptr) {
        void *addr = target->surfaceBufffer_->GetVirAddr();
        uint32_t size = target->surfaceBufffer_->GetSize();
        fwrite(addr, 1, size, file_);
        fflush(file_);
        cnt_++;
        if (cnt_ == 50) {
            cnt_ = 0;
            fseek(file_, 0, SEEK_SET);
        }
    }
#endif
    int32_t ret = producerSurface_->FlushBuffer(target->surfaceBufffer_, -1, flushConfig);
    printf("[%s :%d] zsy##### FlushBuffer:%p, ret:%d\n", __func__, __LINE__, handle, ret);
    printf("[%s :%d] zsy##### handle:%p, func:%p,context:%p \n",
      __func__, __LINE__,handle, target->release_func, target->port_context);
    if (ret != SURFACE_ERROR_OK) {
        producerSurface_->CancelBuffer(target->surfaceBufffer_);
        if (ret != OHOS::SurfaceError::SURFACE_ERROR_OK) {
            GST_ERROR("cancel buffer to surface failed, %d", ret);
        }
        MEDIA_LOGE("[CancelBuffer:%{public}p, handle:%{public}p, func:%{public}p,context:%{public}p",
           target->buffer, handle, target->release_func, target->port_context);
        return MSERR_INVALID_OPERATION;
    }
    if (surfaceTimeEnable) {
        surfaceTimeMonitor_.FinishTime();
    }
    return MSERR_OK;
}

void GstPlayerVideoRendererCtrl::SetSurfaceBufferWidthAndHeight(uint32_t width, uint32_t height)
{
    if (width_ != width || height_ != height) {
        width_ = width;
        height_ = height;
        MEDIA_LOGI("%s:%d. width_:%d, height_:%d, surfacenull:%d",
            __func__, __LINE__, width_, height_, producerSurface_ == nullptr);
        producerSurface_->SetDefaultWidthAndHeight(width_, height_);
    }
}

void GstPlayerVideoRendererCtrl::GetSurfaceBufferWidthAndHeight(uint32_t &width, uint32_t &height)
{
    width = width_;
    height = height_;
}

void GstPlayerVideoRendererCtrl::SetBufferNum(uint32_t num)
{
    if (bufferNum_ != num && num != 0) {
        bufferNum_ = num;
        MEDIA_LOGI("%s:%d. bufferNum_:%d, surfacenull:%d",
            __func__, __LINE__, bufferNum_, producerSurface_ == nullptr);

        producerSurface_->SetQueueSize(bufferNum_);
    }
}

void GstPlayerVideoRendererCtrl::SetCancelBufferFlag(bool cancel)
{
    if (cancelBuffer_ != cancel) {
        g_mutex_lock(&memBufMuxtex_);
        cancelBuffer_ = cancel;
        g_mutex_unlock(&memBufMuxtex_);
        MEDIA_LOGI("%s:%d. cancelBuffer_:%d, surfacenull:%d",
            __func__, __LINE__, cancelBuffer_, producerSurface_ == nullptr);
    }
}

void GstPlayerVideoRendererCtrl::PopMemBufFromQueue (GstMemory *mem, void *userData)
{
  GstMemorySurfaceBuf *bufMem = (GstMemorySurfaceBuf *) mem;
  guint len = 0;

  g_mutex_lock(&memBufMuxtex_);
  len = g_queue_get_length(&memBuf_);
  for(int32_t idx = 0; idx < len; idx++){
    GstMemInfo* item = (GstMemInfo *)g_queue_peek_nth(&memBuf_, idx);
    if(item != nullptr) {
        GST_ERROR("peek GstVideoMmzVoFlag %d:is null, why?", idx);
        continue;
    }
    if(item->priHandle_ == bufMem->data){
        gst_allocator_free_buffer((GstAllocator *)userData, item);
        g_free(item);
        g_queue_pop_nth(&memBuf_, idx);
        break;
    }
  }
  g_mutex_unlock(&memBufMuxtex_);
}

void GstPlayerVideoRendererCtrl::PushMemBufToQueue (GstMemInfo *item)
{
  g_mutex_lock(&memBufMuxtex_);
  g_queue_push_tail(&memBuf_, item);
  g_mutex_unlock(&memBufMuxtex_);
}

void GstPlayerVideoRendererCtrl::GetSurfaceProducer(sptr<Surface> &surface)
{
    surface = producerSurface_;
}

int32_t GstPlayerVideoRendererCtrl::SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    obs_ = obs;
    return MSERR_OK;
}

GstPlayerVideoRenderer *GstPlayerVideoRendererFactory::Create(
    const std::shared_ptr<GstPlayerVideoRendererCtrl> &rendererCtrl)
{
    CHECK_AND_RETURN_RET_LOG(rendererCtrl != nullptr, nullptr, "rendererCtrl is nullptr..");
    return player_video_renderer_new(rendererCtrl);
}

void GstPlayerVideoRendererFactory::Destroy(GstPlayerVideoRenderer *renderer)
{
    CHECK_AND_RETURN_LOG(renderer != nullptr, "renderer is nullptr");
    gst_object_unref(renderer);
}
} // namespace Media
} // namespace OHOS
