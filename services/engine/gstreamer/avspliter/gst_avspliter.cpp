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

#include "config.h"
#include "gst_avspliter.h"
#include "gst_shared_mem_sink.h"

#define CHECK_AND_BREAK_LOG(obj, cond, fmt, ...)       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            GST_ERROR_OBJECT(obj, fmt, ##__VA_ARGS__); \
            break;                                     \
        }                                              \
    } else void (0)

#define SAMPLE_LOCK(avspliter) g_mutex_lock(&avspliter->sampleLock)
#define SAMPLE_UNLOCK(avspliter) g_mutex_unlock(&avspliter->sampleLock);
#define WAIT_SAMPLE_COND(avspliter) g_cond_wait(&avspliter->sampleCond, &avspliter->sampleLock);
#define SIGNAL_SAMPLE_COND(avspliter) g_cond_signal(&avspliter->sampleCond)

static const guint INVALID_TRACK_ID = UINT_MAX;


enum {
    PROP_0,
    PROP_URI,
};

GST_DEBUG_CATEGORY_STATIC (gst_avspliter_debug);
#define GST_CAT_DEFAULT gst_avspliter_debug

#define gst_avspliter_parent_class parent_class
G_DEFINE_TYPE(GstAVSpliter, gst_avspliter, GST_TYPE_PIPELINE);

G_DEFINE_TYPE(GstAVSpliterMediaInfo, gst_avspliter_media_info, G_TYPE_OBJECT);

static void gst_avspliter_dispose(GObject *object);
static void gst_avspliter_finalize(GObject *object);
static void gst_avspliter_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec);
static void gst_avspliter_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_avspliter_change_state(GstElement *element, GstStateChange transition);
static void gst_avspliter_handle_message(GstBin *bin, GstMessage *message);
static gboolean activate_avspliter(GstAVSpliter *avspliter);
static gboolean deactivate_avspliter(GstAVSpliter *avspliter);
static gboolean build_urisourcebin(GstAVSpliter *avspliter);
static void free_urisourcebin(GstAVSpliter *avspliter);
static void urisrcbin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliter *avspliter);
static gboolean build_avspliter_bin(GstAVSpliter *avspliter);
static void free_avspliter_bin(GstAVSpliter *avspliter);
static void avspliter_bin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliter *avspliter);
static void avspliter_bin_no_more_pads_cb(GstElement *element, GstAVSpliter *avspliter);
static void avspliter_bin_pad_removed_cb(GstElement *element, GstPad *pad, GstAVSpliter *avspliter);
static void avspliter_bin_have_type_cb(GstElement *element, guint probability,
    GstCaps *caps, GstAVSpliter *avspliter);
static GstPadProbeReturn sink_pad_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data);
static gboolean try_add_sink(GstAVSpliter *avspliter, GstPad *pad, GstAVSpliterStream *stream);
static void update_stream_media_info(GstAVSpliter *avspliter, GstAVSpliterStream *stream, GstEvent *event);
static void update_stream_pool_info(GstAVSpliter *avspliter, GstAVSpliterStream *stream, GstQuery *query);
static void update_stream_type(GstAVSpliter *avspliter, GstAVSpliterStream *stream);
static void set_stream_cache_limit(GstAVSpliter *avspliter, GstAVSpliterStream *stream);
static GstFlowReturn new_sample_cb(GstMemSink *sink, GstBuffer *sample, gpointer userdata);
static GstFlowReturn new_preroll_cb(GstMemSink *sink, GstBuffer *sample, gpointer userdata);
static void eos_cb(GstMemSink *sink, gpointer userdata);
static GstAVSpliterStream *add_avspliter_stream(GstAVSpliter *avspliter);
static void free_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream, guint index);
static GstAVSpliterStream *find_avspliter_stream_by_sinkpad(GstAVSpliter *avspliter, GstPad *sinkpad);
static GstAVSpliterStream *find_avspliter_stream_by_sink(GstAVSpliter *avspliter, GstElement *sink);

static GstAVSpliterMediaInfo *gst_avspliter_media_info_copy(GstAVSpliterMediaInfo *mediaInfo);

static GstMemSinkCallbacks g_sinkCallbacks = {
    eos_cb,
    new_preroll_cb,
    new_sample_cb
};

static void gst_avspliter_class_init(GstAVSpliterClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);
    GstElementClass *elementClass = GST_ELEMENT_CLASS(klass);
    g_return_if_fail(elementClass != nullptr);
    GstBinClass *binClass = GST_BIN_CLASS(klass);
    g_return_if_fail(binClass != nullptr);

    gobjectClass->dispose = gst_avspliter_dispose;
    gobjectClass->finalize = gst_avspliter_finalize;
    gobjectClass->set_property = gst_avspliter_set_property;
    gobjectClass->get_property = gst_avspliter_get_property;

    g_object_class_install_property(gobjectClass, PROP_URI,
        g_param_spec_string ("uri", "URI", "URI of the media to split track",
            NULL, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(elementClass,
        "AVSpliter Pipeline", "Generic/Bin",
        "Splite the tracks and output all samples to avsharedmemory",
        "OpenHarmony");

    elementClass->change_state = GST_DEBUG_FUNCPTR(gst_avspliter_change_state);
    binClass->handle_message = GST_DEBUG_FUNCPTR(gst_avspliter_handle_message);
}

static void gst_avspliter_init(GstAVSpliter *avspliter)
{
    g_return_if_fail(avspliter != nullptr);

    avspliter->uri = nullptr;
    avspliter->streams = nullptr;
    avspliter->avsBin = nullptr;
    avspliter->urisourcebin = nullptr;
    avspliter->avsbinPadAddedId = 0;
    avspliter->avsbinPadRemovedId = 0;
    avspliter->urisrcbinPadAddedId = 0;
    avspliter->avsbinHaveTypeId = 0;
    avspliter->avsbinNoMorePadsId = 0;
    avspliter->asyncPending = FALSE;
    avspliter->noMorePads = FALSE;
    avspliter->defaultTrackId = INVALID_TRACK_ID;
    avspliter->mediaInfo = nullptr;
    avspliter->hasAudio = FALSE;
    avspliter->hasVideo = FALSE;
}

static void gst_avspliter_dispose(GObject *object)
{
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(object);
    g_return_if_fail(avspliter != nullptr);

    if (avspliter->uri != nullptr) {
        g_free(avspliter->uri);
        avspliter->uri = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_avspliter_finalize(GObject *object)
{
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(object);
    g_return_if_fail(avspliter != nullptr);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_avspliter_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(value != nullptr && object != nullptr);
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(object);

    switch (propId) {
        case PROP_URI:
            GST_OBJECT_LOCK(avspliter);
            g_free(avspliter->uri);
            avspliter->uri = g_value_dup_string(value);
            g_return_if_fail(avspliter->uri != nullptr);
            GST_INFO_OBJECT(object, "set uri: %s", avspliter->uri);
            GST_OBJECT_UNLOCK(avspliter);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_avspliter_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(value != nullptr && object != nullptr);
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(object);

    switch (propId) {
        case PROP_URI:
            GST_OBJECT_LOCK(avspliter);
            g_value_set_string(value, avspliter->uri);
            GST_OBJECT_UNLOCK(avspliter);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static GstStateChangeReturn gst_avspliter_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(element);

    switch (transition) {
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (!activate_avspliter(avspliter)) {
                deactivate_avspliter(avspliter);
                return GST_STATE_CHANGE_FAILURE;
            }
            do_async_start(avspliter);
            break;
    }

    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(avspliter, "element failed to change states");
        do_async_done(avspliter);
        return GST_STATE_CHANGE_FAILURE;
    }

    switch (transition) {
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            do_async_done(avspliter);
           break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            deactivate_avspliter(avspliter);
            break;
    }

    if (ret == GST_STATE_CHANGE_NO_PREROLL || ret == GST_STATE_CHANGE_FAILURE) {
        do_async_done(avspliter);
    }
    return ret;
}

GstAVSpliterMediaInfo *gst_avspliter_get_media_info(GstAVSpliter *avspliter)
{
    g_return_val_if_fail(avspliter != nullptr, nullptr);

    if (avspliter->mediaInfo == nullptr) {
        GST_ERROR_OBJECT(avspliter, "error state, currently no any media info probed");
        return nullptr;
    }

    return gst_avspliter_media_info_copy(avspliter->mediaInfo);
}

gboolean gst_avspliter_select_track(GstAVSpliter *avspliter, guint trackIdx, gboolean select)
{
    g_return_val_if_fail(avspliter != nullptr, FALSE);

    if (avspliter->streams == nullptr) {
        GST_ERROR_OBJECT(avspliter, "error state, currently no any track probed");
        return FALSE;
    }

    for (guint i = 0; i < avspliter->streams->len; i++) {
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
        if (stream->id == trackIdx) {
            stream->selected = select;
            GST_INFO_OBJECT(avspliter, "%s track(%u)", select ? "select" : "unselect", trackIdx);
            return TRUE;
        }
    }

    GST_ERROR_OBJECT(avspliter, "track id %u is invalid", trackIdx);
    return FALSE;
}

enum TrackPullReturn {
    TRACK_PULL_UNSELECTED,
    TRACK_PULL_ENDTIME,
    TRACK_PULL_EOS,
    TRACK_PULL_CACHE_EMPTY,
    TRACK_PULL_SUCCESS,
};

TrackPullReturn try_pull_one_sample(GstAVSpliter *avspliter,
    GstAVSpliterStream *stream, GstClockTime endtime, GstBuffer **out)
{
    /**
     * 加锁，防止被选择的轨道发生变化
     *
     * 循环直到满足退出条件
     *   检查是否shutdown了
     *
     *   遍历所有的轨道
     *     从该轨道的缓存中取出一帧（返回值：未被选择，达到endtime，达到eos，没有缓存，成功取帧）
     *       查看该帧的时间戳，确认是否要丢弃，如果要丢弃，则丢弃并重新取一帧，重新检查
     *       若不需要丢弃，则看是否该轨道被选择，若没有被选择，则直接返回未被选择
     *       若被选择，则看该帧是否超过了endtime，若超过了则重新push回队列，并返回endtime
     *       到这说明这一帧满足要求，返回成功取帧与该帧
     *       若不能从缓存中取出一帧，则看该轨道是否达到EOS，是则返回EOS
     *     若成功取出一帧，按照时间顺序插入到list中
     *
     *   遍历所有轨道结束，看这一轮遍历，是否有新pull出来的buffer，若有，则
     *     检查是否已经满足了所需的帧数，若满足则直接返回结果
     *     检查是否所有被选择的轨道都已经满足了endtime，若满足则直接返回结果
     *     到这，继续回到最开始的那一步，继续取帧
     *
     *   若没有新pull出来的帧，说明被选择的轨道没有缓存了，则检查是否需要等待新缓存
     *     遍历所有被选择的轨道，确认是否已经都达到了EOS，若是则立即退出
     *     遍历所有的轨道，确认是否有轨道的cacheSize顶满了cacheLimit,
     *       若是，则无法继续等待，否则有可能陷入无限等待，需要立即退出
     *       若不是，则检查是否shutdown了，没有shutdown就等待直到被唤醒
     */
}

GList *gst_avspliter_pull_samples(GstAVSpliter *avspliter,
    GstClockTime starttime, GstClockTime endtime, guint bufcnt)
{
    g_return_val_if_fail(avspliter != nullptr, nullptr);

    if (avspliter->streams == nullptr) {
        GST_ERROR_OBJECT(avspliter, "error state, currently no any track probed");
        return FALSE;
    }

    /* notify the underlying pool of sink: the app has already release the reference to sharedmemory */
    for (guint trackId = 0; trackId < avspliter->streams->len; trackId++) {
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, trackId);
        gst_mem_sink_app_render(GST_MEM_SINK_CAST(stream->shmemSink), nullptr);
    }

    if (bufcnt == 0 && endtime == GST_CLOCK_TIME_NONE) {
        GST_ERROR_OBJECT(avspliter, "invalid param");
        return nullptr;
    }

    GST_DEBUG_OBJECT(avspliter, "pull sample, starttime: %" GST_TIME_FORMAT
        ", endtime: " GST_TIME_FORMAT " , bufcnt: %u",
        GST_TIME_ARGS(starttime), GST_TIME_ARGS(endtime), bufcnt);

    SAMPLE_LOCK(avspliter);

    if (starttime != GST_CLOCK_TIME_NONE) {
        GstClockTime lastPos = GST_CLOCK_TIME_NONE;
        for (guint trackId = 0; trackId < avspliter->streams->len; trackId++) {
            GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, trackId);
            stream->lastPos = (stream->lastPos > starttime) ? stream->lastPos : starttime;
        }
    }

    SAMPLE_UNLOCK(avspliter);
}

static gboolean activate_avspliter(GstAVSpliter *avspliter)
{
    if (avspliter->uri == nullptr) {
        GST_ERROR_OBJECT(avspliter, "not set uri");
        return FALSE;
    }

    deactivate_avspliter(avspliter);

    avspliter->streams = g_array_new(FALSE, TRUE, sizeof(GstAVSpliterStream));
    if (avspliter->streams == nullptr) {
        GST_ERROR_OBJECT(avspliter, "allocate the avspliter streams failed");
        return FALSE;
    }

    avspliter->mediaInfo = reinterpret_cast<GstAVSpliterMediaInfo *>(
        g_object_new(GST_TYPE_AVSPLITER_MEDIA_INFO, nullptr));
    if (avspliter->streams == nullptr) {
        GST_ERROR_OBJECT(avspliter, "allocate the avspliter media info failed");
        return FALSE;
    }

    gboolean ret = build_urisourcebin(avspliter);
    g_return_val_if_fail(ret, FALSE);

    ret = build_avspliter_bin(avspliter);
    g_return_val_if_fail(ret, FALSE);

    g_object_set(avspliter->urisourcebin, "uri", avspliter->uri);
    return TRUE;
}

static gboolean deactivate_avspliter(GstAVSpliter *avspliter)
{
    if (avspliter->streams != nullptr) {
        for (guint i = 0; i < avspliter->streams->len; i++) {
            free_avspliter_stream(avspliter, &g_array_index(avspliter->streams, GstAVSpliterStream, i), i);
        }
        g_array_free(avspliter->streams, TRUE);
        avspliter->streams = nullptr;
    }

    gst_object_unref(avspliter->mediaInfo);
    avspliter->mediaInfo = nullptr;

    avspliter->hasAudio = FALSE;
    avspliter->hasVideo = FALSE;
    avspliter->defaultTrackId = INVALID_TRACK_ID;

    free_avspliter_bin(avspliter);
    free_urisourcebin(avspliter);
}

static gboolean build_urisourcebin(GstAVSpliter *avspliter)
{
    if (avspliter->urisourcebin != nullptr) {
        return TRUE;
    }

    gboolean buildOK = FALSE;

    do {
        avspliter->urisourcebin = gst_element_factory_make("urisourcebin", "urisourcebin");
        CHECK_AND_BREAK_LOG(avspliter, avspliter->urisourcebin != nullptr,
            "create urisourcebin failed");

        avspliter->urisrcbinPadAddedId = g_signal_connect(avspliter->urisourcebin, "pad-added",
            G_CALLBACK(urisrcbin_pad_added_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->urisrcbinPadAddedId != 0,
            "connect urisourcebin pad-added failed");

        if (!gst_bin_add(GST_BIN_CAST(avspliter), avspliter->urisourcebin)) {
            GST_ERROR_OBJECT(avspliter, "add urisourcebin to bin failed");
            break;
        }
        buildOK = TRUE;
    } while (0);

    if (buildOK) {
        return TRUE;
    }

    GST_ELEMENT_ERROR(avspliter, CORE, FAILED, ("build urisourcebin failed"), (""));
    return FALSE;
}

static void free_urisourcebin(GstAVSpliter *avspliter)
{
    if (avspliter->urisrcbinPadAddedId != 0) {
        g_signal_handler_disconnect(avspliter->urisourcebin, avspliter->urisrcbinPadAddedId);
        avspliter->urisrcbinPadAddedId = 0;
    }

    if (avspliter->urisourcebin != nullptr) {
        gst_bin_remove(GST_BIN_CAST(avspliter), avspliter->urisourcebin);
    }
    avspliter->urisourcebin = nullptr;
}

static gboolean build_avspliter_bin(GstAVSpliter *avspliter)
{
    if (avspliter->avsBin != nullptr) {
        return TRUE;
    }

    gboolean buildOK = FALSE;

    do {
        avspliter->avsBin = gst_element_factory_make("avspliterbin", "avspliterbin");
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsBin != nullptr,
            "create avspliter bin failed");

        avspliter->avsbinHaveTypeId = g_signal_connect(avspliter->avsBin, "have-type",
            G_CALLBACK(avspliter_bin_have_type_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinHaveTypeId != 0,
            "connect avspliter bin have-type failed");

        avspliter->avsbinPadAddedId = g_signal_connect(avspliter->avsBin, "pad-added",
            G_CALLBACK(avspliter_bin_pad_added_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinPadAddedId != 0,
            "connect avspliter bin pad-added failed");

        avspliter->avsbinPadRemovedId = g_signal_connect(avspliter->avsBin, "pad-removed",
            G_CALLBACK(avspliter_bin_pad_removed_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinPadRemovedId != 0,
            "connect avspliter bin pad-removed failed");

        avspliter->avsbinNoMorePadsId = g_signal_connect(avspliter->avsBin, "no-more-pads",
            G_CALLBACK(avspliter_bin_no_more_pads_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinNoMorePadsId != 0,
            "connect avspliter bin no-more-pads failed");

        if (!gst_bin_add(GST_BIN_CAST(avspliter),avspliter->avsBin)) {
            GST_ERROR_OBJECT(avspliter, "add avspliter bin to bin failed");
            break;
        }
        buildOK = TRUE;
    } while (0);

    if (buildOK) {
        return TRUE;
    }

    GST_ELEMENT_ERROR(avspliter, CORE, FAILED, ("build avspliter bin failed"), (""));
    return FALSE;
}

static void free_avspliter_bin(GstAVSpliter *avspliter)
{
    if (avspliter->avsbinPadAddedId != 0) {
        g_signal_handler_disconnect(avspliter->avsBin, avspliter->avsbinPadAddedId);
        avspliter->avsbinPadAddedId = 0;
    }

    if (avspliter->avsbinPadRemovedId != 0) {
        g_signal_handler_disconnect(avspliter->avsBin, avspliter->avsbinPadRemovedId);
        avspliter->avsbinPadRemovedId = 0;
    }

    if (avspliter->avsbinNoMorePadsId != 0) {
        g_signal_handler_disconnect(avspliter->avsBin, avspliter->avsbinNoMorePadsId);
        avspliter->avsbinNoMorePadsId = 0;
        avspliter->noMorePads = FALSE;
    }

    if (avspliter->avsBin != nullptr) {
        gst_bin_remove(GST_BIN_CAST(avspliter), avspliter->avsBin);
        avspliter->avsBin = nullptr;
    }
}

static void do_async_start(GstAVSpliter *avspliter)
{
    GstMessage *message = gst_message_new_async_start(GST_OBJECT_CAST(avspliter));
    g_return_if_fail(message != nullptr);
    avspliter->asyncPending = TRUE;

    GST_BIN_CLASS(parent_class)->handle_message(GST_BIN_CAST(avspliter), message);
}

static void do_async_done(GstAVSpliter *avspliter)
{
    if (avspliter->asyncPending) {
        GST_DEBUG_OBJECT(avspliter, "posting ASYNC_DONE");
        GstMessage *message = gst_message_new_async_done(GST_OBJECT_CAST(avspliter), GST_CLOCK_TIME_NONE);
        GST_BIN_CLASS (parent_class)->handle_message(GST_BIN_CAST(avspliter), message);

        avspliter->asyncPending = FALSE;
    }
}

static void avspliter_bin_have_type_cb(GstElement *element, guint probability,
    GstCaps *caps, GstAVSpliter *avspliter)
{
    if (!(avspliter != nullptr && caps != nullptr && element != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process have-type, param is valid");
        return;
    }

    g_return_if_fail(avspliter->mediaInfo != nullptr);

    if (avspliter->mediaInfo->containerCaps != nullptr) {
        gst_object_unref(avspliter->mediaInfo->containerCaps);
    }
    avspliter->mediaInfo->containerCaps = gst_caps_ref(caps);

    GST_INFO_OBJECT(avspliter, "got container format, caps: %" GST_PTR_FORMAT, caps);
}

static void urisrcbin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliter *avspliter)
{
    if (!(element != nullptr && pad != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process urisourcebin pad added, param is invalid");
        return;
    }

    GstPad *avsBinSinkPad = gst_element_get_static_pad(avspliter->avsBin, "sink");
    if (avsBinSinkPad == nullptr) {
        GST_ERROR_OBJECT(avspliter, "failed to link pad %s:%s to avspliterbin",
            GST_DEBUG_PAD_NAME(pad));
        return;
    }

    GstPadLinkReturn ret = gst_pad_link_full(pad, avsBinSinkPad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref(avsBinSinkPad);
    if (GST_PAD_LINK_FAILED(ret)) {
        GST_ERROR_OBJECT(avspliter, "failed to link pad %s:%s to avspliterbin's pad %s, reason %s (%d)",
            GST_DEBUG_PAD_NAME (pad), GST_PAD_NAME(avsBinSinkPad), gst_pad_link_get_name(ret), ret);
    }
}

static void avspliter_bin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliter *avspliter)
{
    if (!(element != nullptr && pad != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process avspliter bin pad added, param is invalid");
        return;
    }

    GST_DEBUG_OBJECT(avspliter, "avspliter bin src pad %s added", GST_PAD_NAME(pad));

    GstAVSpliterStream *stream = add_avspliter_stream(avspliter);
    if (stream == nullptr) {
        GST_ERROR_OBJECT(avspliter, "create stream failed for pad %s added", GST_PAD_NAME(pad));
        return;
    }

    if (!try_add_sink(avspliter, pad, stream)) {
        free_avspliter_stream(avspliter, stream, avspliter->streams->len);
        return;
    }

    avspliter->mediaInfo->streams = g_list_append(avspliter->mediaInfo->streams, stream->info);
}

static void avspliter_bin_pad_removed_cb(GstElement *element, GstPad *pad, GstAVSpliter *avspliter)
{
    if (!(element != nullptr && pad != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process avspliter bin pad removed, param is invalid");
        return;
    }

    GST_DEBUG_OBJECT(avspliter, "avspliter bin src pad %s removed", GST_PAD_NAME(pad));

    GstAVSpliterStream *stream = nullptr;
    guint index = 0;
    for (; index < avspliter->streams->len; index++) {
        GstAVSpliterStream *tmp = &g_array_index(avspliter->streams, GstAVSpliterStream, index);
        if (stream->avsBinPad == pad) {
            stream = tmp;
            break;
        }
    }

    if (stream == nullptr) {
        GST_ERROR_OBJECT(avspliter, "can find the pad %s in internel list", GST_PAD_NAME(pad));
        return;
    }

    free_avspliter_stream(avspliter, stream, index);
}

static void avspliter_bin_no_more_pads_cb(GstElement *element, GstAVSpliter *avspliter)
{
    if (!(element != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process avspliter bin no-more pad, param is invalid");
        return;
    }

    GST_DEBUG_OBJECT(avspliter, "avspliter bin emit no more pads");

    avspliter->noMorePads = TRUE;
    do_async_done(avspliter);
}

static gboolean try_add_sink(GstAVSpliter *avspliter, GstPad *pad, GstAVSpliterStream *stream)
{
    stream->avsBinPad = pad;
    stream->shmemSink = gst_element_factory_make("sharedmemsink", "shmemsink");
    (avspliter, stream->shmemSink != nullptr, "failed to create shmemsink");

    gst_element_set_locked_state(stream->shmemSink, TRUE);

    if (!gst_bin_add(GST_BIN_CAST(avspliter), stream->shmemSink)) {
        GST_ERROR_OBJECT(avspliter, "failed to add shmemsink to pipeline");
        return FALSE;
    }
    stream->inBin = TRUE;

    if (stream->shmemSink->sinkpads != nullptr && stream->shmemSink->sinkpads->data != nullptr) {
        stream->sinkpad = GST_PAD_CAST(gst_object_ref(stream->shmemSink->sinkpads));
    }
    if (stream->sinkpad == nullptr) {
        GST_ERROR_OBJECT(avspliter, "failed to get sinkpad of shmemsink");
        return FALSE;
    }

    stream->sinkPadProbeId = gst_pad_add_probe(stream->sinkpad,
        (GstPadProbeType)(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM),
        sink_pad_probe_cb, avspliter, nullptr);
    if (stream->sinkPadProbeId == 0) {
        GST_ERROR_OBJECT(avspliter, "add pad probe failed");
        return;
    }

    if (gst_pad_link(pad, stream->sinkpad) != GST_PAD_LINK_OK) {
        GST_ERROR_OBJECT(avspliter, "link pad %s:%s to pad %s:%s failed",
            GST_DEBUG_PAD_NAME(pad), GST_DEBUG_PAD_NAME(stream->sinkpad));
        return;
    }

    if (gst_element_set_state(stream->shmemSink, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(avspliter, "change shmemsink to paused failed");
        return;
    }

    gst_mem_sink_set_callback(GST_MEM_SINK(stream->shmemSink), &g_sinkCallbacks, avspliter, nullptr);

    gst_element_set_locked_state(stream->shmemSink, FALSE);

    return TRUE;
}

static GstAVSpliterStream *add_avspliter_stream(GstAVSpliter *avspliter)
{
    guint len = avspliter->streams->len;
    g_array_set_size(avspliter->streams, len + 1);
    GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, len);

    stream->id = len;
    gchar *streamId = g_strdup_printf("%d", avspliter->streams->len);
    stream->info = gst_stream_new(streamId, nullptr, GST_STREAM_TYPE_UNKNOWN, GST_STREAM_FLAG_NONE);
    g_free(streamId);

    if (stream->info == nullptr) {
        GST_ERROR_OBJECT(avspliter, "create stream failed");
        g_array_remove_index(avspliter->streams, len);
        return nullptr;
    }

    return stream;
}

static void free_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream, guint index)
{
    if (stream->sinkPadProbeId != 0) {
        gst_pad_remove_probe(stream->sinkpad, stream->sinkPadProbeId);
        stream->sinkPadProbeId = 0;
    }

    if (stream->sinkpad != nullptr) {
        gst_object_unref(stream->sinkpad);
        stream->sinkpad = nullptr;
    }

    if (stream->shmemSink != nullptr) {
        (void)gst_element_set_state(stream->shmemSink, GST_STATE_NULL);
        if (stream->inBin) {
            gst_bin_remove(GST_BIN_CAST(avspliter), stream->shmemSink);
        } else {
            gst_object_unref(stream->shmemSink);
        }
        stream->shmemSink = nullptr;
    }

    if (stream->info != nullptr) {
        for (GList *node = avspliter->mediaInfo->streams; node != nullptr; node = node->next) {
            GstStream *gstStream = GST_STREAM_CAST(node->data);
            if (g_str_equal(gstStream->stream_id, stream->info->stream_id)) {
                avspliter->mediaInfo->streams = g_list_delete_link(avspliter->mediaInfo->streams, node);
                break;
            }
        }
        gst_object_unref(stream->info);
        stream->info = nullptr;
    }

    if (stream->sampleQueue != nullptr) {
        gst_queue_array_free(stream->sampleQueue);
        stream->sampleQueue;
    }
    stream->cacheLimit = 0;
    stream->cacheSize = 0;
    stream->eos = FALSE;
    stream->selected = FALSE;

    stream->avsBinPad = nullptr;
    stream->inBin = FALSE;

    g_array_remove_index(avspliter->streams, index);
}

static GstPadProbeReturn sink_pad_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    g_return_val_if_fail(pad != nullptr && info != nullptr && data != nullptr, GST_PAD_PROBE_DROP);

    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(data);
    GstAVSpliterStream *stream = find_avspliter_stream_by_sinkpad(avspliter, pad);
    if (stream == nullptr) {
        GST_ERROR_OBJECT(avspliter, "unrecognized pad: %s:%s", GST_DEBUG_PAD_NAME(pad));
        return GST_PAD_PROBE_DROP;
    }

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
        g_return_val_if_fail(event != nullptr, GST_PAD_PROBE_DROP);
        GST_DEBUG_OBJECT(pad, "Seeing event '%s'", GST_EVENT_TYPE_NAME(event));

        switch (GST_EVENT_TYPE(event)) {
            case GST_EVENT_STREAM_START: /* fall-through */
            case GST_EVENT_CAPS: /* fall-through */
            case GST_EVENT_TAG:
                update_stream_media_info(avspliter, stream, event);
                break;
            default:
                break;
        }
    }

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM) {
        GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
        g_return_val_if_fail(query != nullptr, GST_PAD_PROBE_DROP);
        GST_LOG_OBJECT(pad, "Seeing query '%s'", GST_QUERY_TYPE_NAME(query));

        if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
            update_stream_pool_info(avspliter, stream, query);
        }
    }

    return GST_PAD_PROBE_OK;
}

static void parse_stream_start_event(GstAVSpliter *avspliter, GstAVSpliterStream *avspliterStream, GstEvent *event)
{
    const gchar *streamId = nullptr;
    gst_event_parse_stream_start(event, &streamId);

    GstStream *stream = nullptr;
    gst_event_parse_stream(event, &stream);
    if (stream == nullptr) {
        GST_DEBUG_OBJECT(avspliter, "stream is null, ignored. streamid: %s, trackId(%s)",
            streamId, avspliterStream->info->stream_id);
    } else {
        GST_DEBUG_OBJECT(avspliter, "Saw stream %s (GstStream %" GST_PTR_FORMAT "), trackId(%s)",
            streamId, stream, avspliterStream->info->stream_id);

        GstCaps *caps = gst_stream_get_caps(stream);
        if (caps != nullptr) {
            gst_stream_set_caps(avspliterStream->info, caps);
            gst_caps_unref(caps);
        }

        GstTagList *tag = gst_stream_get_tags(stream);
        if (tag != nullptr) {
            gst_stream_set_tags(avspliterStream->info, tag);
            gst_tag_list_ref(tag);
        }

        gst_stream_set_stream_flags(avspliterStream->info, gst_stream_get_stream_flags(stream));
        gst_stream_set_stream_type(avspliterStream->info, gst_stream_get_stream_type(stream));

        update_stream_type(avspliter, avspliterStream);
    }

    gst_object_unref(stream);
}

static void update_stream_media_info(GstAVSpliter *avspliter, GstAVSpliterStream *stream, GstEvent *event)
{
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_STREAM_START: {
            parse_stream_start_event(avspliter, stream, event);
            break;
        }
        case GST_EVENT_CAPS: {
            GstCaps *caps= nullptr;
            gst_event_parse_caps(event, &caps);
            if (caps != nullptr) {
                GST_DEBUG_OBJECT(avspliter, "got out caps: %" GST_PTR_FORMAT, caps);
                gst_stream_set_caps(stream->info, caps);
                update_stream_type(avspliter, stream);
                set_stream_cache_limit(avspliter, stream);
            }
            break;
        }
        case GST_EVENT_TAG: {
            GstTagList *tagList = nullptr;
            gst_event_parse_tag(event, &tagList);
            if (tagList != nullptr) {
                break;
            }

            if (gst_tag_list_get_scope(tagList) == GST_TAG_SCOPE_GLOBAL) {
                GstObject **obj = reinterpret_cast<GstObject **>(&avspliter->mediaInfo->globalTags);
                gst_object_replace(obj, GST_OBJECT_CAST(gst_tag_list_ref(tagList)));
            } else {
                gst_stream_set_tags(stream->info, tagList);
            }
            GST_DEBUG_OBJECT(avspliter, "got taglist: %" GST_PTR_FORMAT, tagList);
        }
        default:
            break;
    }
}

static void update_stream_pool_info(GstAVSpliter *avspliter, GstAVSpliterStream *stream, GstQuery *query)
{
    if (GST_QUERY_TYPE(query) == GST_QUERY_ALLOCATION) {
        gboolean needPool = FALSE;
        GstCaps *caps = nullptr;
        gst_query_parse_allocation(query, &caps, &needPool);
        if (!needPool) {
            return;
        }
        GST_ERROR_OBJECT(avspliter, "FIXME: upstream need pool, trackId(%u)", stream->id);
    }
}

static GstStreamType guess_stream_type_from_caps(GstCaps *caps)
{
    if (gst_caps_get_size(caps) < 1) {
        return GST_STREAM_TYPE_UNKNOWN;
    }

    GstStructure *s = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(s);

    if (g_str_has_prefix(name, "video/") || g_str_has_prefix(name, "image/")) {
        return GST_STREAM_TYPE_VIDEO;
    }

    if (g_str_has_prefix(name, "audio/")) {
        return GST_STREAM_TYPE_AUDIO;
    }

    return GST_STREAM_TYPE_UNKNOWN;
}

static void update_stream_type(GstAVSpliter *avspliter, GstAVSpliterStream *stream)
{
    GstStreamType type = gst_stream_get_stream_type(stream->info);
    if (type == GST_STREAM_TYPE_UNKNOWN) {
        GstCaps *caps = gst_stream_get_caps(stream->info);
        if (caps == nullptr) {
            return;
        }

        if (gst_caps_is_fixed(caps)) {
            GST_WARNING_OBJECT(avspliter, "caps is not fixed for trackId(%u), caps: %" GST_PTR_FORMAT,
                stream->id, caps);
        }

        type = guess_stream_type_from_caps(caps);
    }

    if (type == GST_STREAM_TYPE_UNKNOWN) {
        return;
    }

    gst_stream_set_stream_type(stream->info, type);

    if (type == GST_STREAM_TYPE_VIDEO) {
        if (!avspliter->hasVideo) {
            avspliter->defaultTrackId = stream->id;
        }
        avspliter->hasVideo = TRUE;
    } else if (type == GST_STREAM_TYPE_AUDIO) {
        if (!avspliter->hasAudio) {
            avspliter->defaultTrackId = stream->id;
        }
        avspliter->hasAudio = TRUE;
    } else if (avspliter->defaultTrackId == INVALID_TRACK_ID) {
        avspliter->defaultTrackId = stream->id;
    }

    GST_INFO_OBJECT(avspliter, "trackId(%u) stream type: %s, defaultId(%u)",
        stream->id, gst_stream_type_get_name(type), avspliter->defaultTrackId);
}

static void set_stream_cache_limit(GstAVSpliter *avspliter, GstAVSpliterStream *stream)
{
    GstStreamType streamType = gst_stream_get_stream_type(stream->info);
    if (streamType == GST_STREAM_TYPE_UNKNOWN) {
        GST_WARNING_OBJECT(avspliter, "stream type unknown for trackId(%u)"
            ", the pool capacity will be set defaultly.", stream->id);
        return;
    }

    static const guint defaultVideoTrackPoolCapacity = 8;
    static const guint defaultAudioTrackPoolCapacity = 64;

    if (streamType == GST_STREAM_TYPE_VIDEO) {
        g_object_set(stream->shmemSink, "max-pool-capacity", defaultVideoTrackPoolCapacity, nullptr);
        stream->cacheLimit = defaultVideoTrackPoolCapacity;
    } else if (streamType == GST_STREAM_TYPE_AUDIO) {
        g_object_set(stream->shmemSink, "max-pool-capacity", defaultAudioTrackPoolCapacity, nullptr);
        stream->cacheLimit = defaultAudioTrackPoolCapacity;
    }

    stream->sampleQueue = gst_queue_array_new(stream->cacheLimit);
    if (stream->sampleQueue == nullptr) {
        GST_ELEMENT_ERROR(avspliter, STREAM, FAILED,
            ("create sample queue failed for trackId(%u)", stream->id), (""));
        return;
    }
}

static GstFlowReturn new_sample_cb(GstMemSink *sink, GstBuffer *sample, gpointer userdata)
{
    g_return_val_if_fail(sink != nullptr && sample != nullptr && userdata != nullptr, GST_FLOW_ERROR);

    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(userdata);
    GstAVSpliterStream *stream = find_avspliter_stream_by_sink(avspliter, GST_ELEMENT_CAST(sink));
    g_return_val_if_fail(stream != nullptr, GST_FLOW_ERROR);

    if (stream->sampleQueue == nullptr) {
        GST_ERROR_OBJECT(avspliter, "sample queue for trackId(%u) is null, unexpected !", stream->id);
        return GST_FLOW_ERROR;
    }

    GST_DEBUG_OBJECT(avspliter, "trackId(%u) new sample arrived, pts: %" GST_TIME_FORMAT, ", cache size: %u/%u",
        stream->id, GST_TIME_ARGS(GST_BUFFER_PTS(sample)), stream->cacheSize, stream->cacheLimit);

    gst_queue_array_push_tail(stream->sampleQueue, gst_buffer_ref(sample));

    return GST_FLOW_OK;
}

static GstFlowReturn new_preroll_cb(GstMemSink *sink, GstBuffer *sample, gpointer userdata)
{
    g_return_val_if_fail(sink != nullptr && sample != nullptr && userdata != nullptr, GST_FLOW_ERROR);
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(userdata);

    GstAVSpliterStream *stream = find_avspliter_stream_by_sink(avspliter, GST_ELEMENT_CAST(sink));
    g_return_val_if_fail(stream != nullptr, GST_FLOW_ERROR);

    GST_DEBUG_OBJECT(avspliter, "trackId(%u) new preroll arrived, pts: %" GST_TIME_FORMAT,
        stream->id, GST_TIME_ARGS(GST_BUFFER_PTS(sample)));

    return GST_FLOW_OK;
}

static void eos_cb(GstMemSink *sink, gpointer userdata)
{
    g_return_if_fail(sink != nullptr && userdata != nullptr);
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(userdata);

    GstAVSpliterStream *stream = find_avspliter_stream_by_sink(avspliter, GST_ELEMENT_CAST(sink));
    g_return_if_fail(stream != nullptr);

    GST_DEBUG_OBJECT(avspliter, "trackId(%u) eos arrived", stream->id);
    stream->eos = TRUE;
}

static GstAVSpliterStream *find_avspliter_stream_by_sinkpad(GstAVSpliter *avspliter, GstPad *sinkpad)
{
    GstAVSpliterStream *stream = nullptr;
    for (guint i = 0; i < avspliter->streams->len; i++) {
        GstAVSpliterStream *tmp = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
        if (tmp->sinkpad == sinkpad) {
            stream = tmp;
            break;
        }
    }
    return stream;
}

static GstAVSpliterStream *find_avspliter_stream_by_sink(GstAVSpliter *avspliter, GstElement *sink)
{
    GstAVSpliterStream *stream = nullptr;
    for (guint i = 0; i < avspliter->streams->len; i++) {
        GstAVSpliterStream *tmp = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
        if (tmp->shmemSink == sink) {
            stream = tmp;
            break;
        }
    }

    return stream;
}

static void gst_avspliter_media_info_init(GstAVSpliterMediaInfo *mediaInfo)
{
    mediaInfo->globalTags = nullptr;
    mediaInfo->containerCaps = nullptr;
    mediaInfo->streams = nullptr;
}

static void gst_avspliter_media_info_finalize(GObject *object)
{
    GstAVSpliterMediaInfo *mediaInfo = GST_AVSPLITER_MEDIA_INFO(object);
    g_return_if_fail(mediaInfo != nullptr);

    if (mediaInfo->globalTags != nullptr) {
        gst_tag_list_unref(mediaInfo->globalTags);
        mediaInfo->globalTags = nullptr;
    }

    if (mediaInfo->containerCaps != nullptr) {
        gst_caps_unref(mediaInfo->containerCaps);
        mediaInfo->containerCaps = nullptr;
    }

    if (mediaInfo->streams != nullptr) {
        g_list_free_full(mediaInfo->streams, gst_object_unref);
        mediaInfo->streams = nullptr;
    }

    G_OBJECT_CLASS(gst_avspliter_media_info_parent_class)->finalize(object);
}

static void gst_avspliter_media_info_class_init(GstAVSpliterMediaInfoClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_avspliter_media_info_finalize;
}

static GstAVSpliterMediaInfo *gst_avspliter_media_info_copy(GstAVSpliterMediaInfo *mediaInfo)
{
    g_return_val_if_fail(mediaInfo != nullptr, nullptr);

    GstAVSpliterMediaInfo *newInfo = reinterpret_cast<GstAVSpliterMediaInfo*>(
        g_object_new(GST_TYPE_AVSPLITER_MEDIA_INFO, nullptr));
    g_return_val_if_fail(newInfo != nullptr, nullptr);

    if (mediaInfo->containerCaps != nullptr) {
        newInfo->containerCaps = gst_caps_ref(mediaInfo->containerCaps);
    }

    if (mediaInfo->globalTags != nullptr) {
        newInfo->globalTags = gst_tag_list_ref(mediaInfo->globalTags);
    }

    if (mediaInfo->streams != nullptr) {
        for (GList *node = mediaInfo->streams; node != nullptr; node = node->next) {
            newInfo->streams = g_list_append(newInfo->streams, gst_object_ref(node->data));
        }
    }

    return newInfo;
}

gboolean plugin_init(GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_avspliter_debug, "avspliter", 0, "avspliter pipeline");

    return gst_element_register(plugin, "avspliter", GST_RANK_NONE, GST_TYPE_AVSPLITER);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _avspliter,
    "Gstreamer AVSpliter Pipeline",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
