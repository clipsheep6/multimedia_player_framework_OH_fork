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

#ifndef GST_PLAYER_VIDEO_RENDERER_HISI_H
#define GST_PLAYER_VIDEO_RENDERER_HISI_H

#include <memory>
#include <string>
#include <gst/gst.h>
#include <gst/player/player.h>
#include "i_player_engine.h"
#include "time_monitor.h"

typedef void * PrivateHandle;
#define DUMP_YUV 1

typedef struct {
    BufferHandle *bufHdl_;
    OHOS::sptr<OHOS::SurfaceBuffer> surfaceBufffer_;
    int32_t fence_;
    int32_t bufferIndex_; /* map with index */
    PrivateHandle priHandle_;
    gpointer release_func;
    gpointer port_context;
    gpointer buffer;
} GstMemInfo;

namespace OHOS {
namespace Media {
class GstPlayerVideoRendererCtrl : public NoCopyable {
public:
    explicit GstPlayerVideoRendererCtrl(const sptr<Surface> &surface);
    ~GstPlayerVideoRendererCtrl();

    int32_t InitVideoSink(const GstElement *playbin);
    int32_t InitAudioSink(const GstElement *playbin);
    const GstElement *GetVideoSink() const;
    const GstElement *GetAudioSink() const;
    int32_t PullVideoBuffer();
    int32_t UpdateSurfaceBuffer(const GstBuffer &buffer);
    void SetSurfaceBufferWidthAndHeight(uint32_t width, uint32_t height);
    void GetSurfaceBufferWidthAndHeight(uint32_t &width, uint32_t &height);
    void SetBufferNum(uint32_t num);
    void SetCancelBufferFlag(bool cancel);
    void GetSurfaceProducer(sptr<Surface> &surface);
    void PopMemBufFromQueue (GstMemory * mem, void *userData);
    void PushMemBufToQueue (GstMemInfo *item);
    static void *DequeueThdFunc (void *data);
    const sptr<Surface> GetProducerSurface() const;
    int32_t SetCallbacks(const std::weak_ptr<IPlayerEngineObs> &obs);

private:
    void UpdateResquestConfig(BufferRequestConfig &requestConfig, const GstVideoMeta *videoMeta) const;
    std::string GetVideoSinkFormat() const;
    void SetSurfaceTimeFromSysPara();
    void CancelAllSurfaceBuffer(void);
    sptr<Surface> producerSurface_ = nullptr;
    GstElement *videoSink_ = nullptr;
    GstElement *audioSink_ = nullptr;
    GstCaps *videoCaps_ = nullptr;
    GstCaps *audioCaps_ = nullptr;
    std::vector<gulong> signalIds_;
    std::weak_ptr<IPlayerEngineObs> obs_;

    bool surfaceTimeEnable = false;
    TimeMonitor surfaceTimeMonitor_;
    gulong signalId_ = 0;
    guint width_ = 0;
    guint height_ = 0;
    guint bufferNum_ = 0xff;
    bool cancelBuffer_ = false;
    GQueue memBuf_;
    GThread *dequeueThd_ = nullptr;
    gboolean dequeueThdExit_ = FALSE;
    GMutex  memBufMuxtex_;
#ifdef DUMP_YUV
    FILE *file_ = nullptr;
    guint cnt_ = 0;
#endif
};

class GstPlayerVideoRendererFactory {
public:
    GstPlayerVideoRendererFactory() = delete;
    ~GstPlayerVideoRendererFactory() = delete;
    static GstPlayerVideoRenderer *Create(const std::shared_ptr<GstPlayerVideoRendererCtrl> &rendererCtrl);
    static void Destroy(GstPlayerVideoRenderer *renderer);
};
} // namespace Media
} // namespace OHOS
#endif // GST_PLAYER_VIDEO_RENDERER_HISI_H
