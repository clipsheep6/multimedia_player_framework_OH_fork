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
#include "gst_avspliter_pipeline.h"

#define CHECK_AND_BREAK_LOG(obj, cond, fmt, ...)       \
    if (1) {                                           \
        if (!(cond)) {                                 \
            GST_ERROR_OBJECT(obj, fmt, ##__VA_ARGS__); \
            break;                                     \
        }                                              \
    } else void (0)

enum {
    PROP_0,
    PROP_URI,
};

GST_DEBUG_CATEGORY_STATIC (gst_avspliter_pipeline_debug);
#define GST_CAT_DEFAULT gst_avspliter_pipeline_debug

#define gst_avspliter_pipeline_parent_class parent_class
G_DEFINE_TYPE(GstAVSpliterPipeline, gst_avspliter_pipeline, GST_TYPE_PIPELINE);

static void gst_avspliter_pipeline_dispose(GObject *object);
static void gst_avspliter_pipeline_finalize(GObject *object);
static void gst_avspliter_pipeline_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec);
static void gst_avspliter_pipeline_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_avspliter_pipeline_change_state(GstElement *element, GstStateChange transition);
static void gst_avspliter_pipeline_handle_message(GstBin *bin, GstMessage *message);
static gboolean activate_avspliter(GstAVSpliterPipeline *avspliter);
static gboolean deactivate_avspliter(GstAVSpliterPipeline *avspliter);
static gboolean build_urisourcebin(GstAVSpliterPipeline *avspliter);
static void free_urisourcebin(GstAVSpliterPipeline *avspliter);
static void urisrcbin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliterPipeline *avspliter);
static gboolean build_avspliter_bin(GstAVSpliterPipeline *avspliter);
static void free_avspliter_bin(GstAVSpliterPipeline *avspliter);
static void avspliter_bin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliterPipeline *avspliter);
static void avspliter_bin_no_more_pads_cb(GstElement *element, GstAVSpliterPipeline *avspliter);
static void avspliter_bin_pad_removed_cb(GstElement *element, GstPad *pad, GstAVSpliterPipeline *avspliter);
static void found_type_callback(GstElement *typefind, guint probability,
    GstCaps *caps, GstAVSpliterPipeline *avspliter);
static GstPadProbeReturn sink_pad_event_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data);
static gboolean try_add_sink(GstAVSpliterPipeline *avspliter, GstPad *pad, GstAVSpliterStream *stream);

static void gst_avspliter_pipeline_class_init(GstAVSpliterPipelineClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);
    GstElementClass *elementClass = GST_ELEMENT_CLASS(klass);
    g_return_if_fail(elementClass != nullptr);
    GstBinClass *binClass = GST_BIN_CLASS(klass);
    g_return_if_fail(binClass != nullptr);

    gobjectClass->dispose = gst_avspliter_pipeline_dispose;
    gobjectClass->finalize = gst_avspliter_pipeline_finalize;
    gobjectClass->set_property = gst_avspliter_pipeline_set_property;
    gobjectClass->get_property = gst_avspliter_pipeline_get_property;

    g_object_class_install_property(gobjectClass, PROP_URI,
        g_param_spec_string ("uri", "URI", "URI of the media to split track",
            NULL, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(elementClass,
        "AVSpliter Pipeline", "Generic/Bin",
        "Splite the tracks and output all samples to avsharedmemory",
        "OpenHarmony");

    elementClass->change_state = GST_DEBUG_FUNCPTR(gst_avspliter_pipeline_change_state);
    binClass->handle_message = GST_DEBUG_FUNCPTR(gst_avspliter_pipeline_handle_message);
}

static void gst_avspliter_pipeline_init(GstAVSpliterPipeline *avspliter)
{
    g_return_if_fail(avspliter != nullptr);

    avspliter->uri = nullptr;
    avspliter->streams = nullptr;
    avspliter->avsBin = nullptr;
    avspliter->urisourcebin = nullptr;
    avspliter->avsbinPadAddedId = 0;
    avspliter->avsbinPadRemovedId = 0;
    avspliter->urisrcbinPadAddedId = 0;
    avspliter->asyncPending = FALSE;
}

static void gst_avspliter_pipeline_dispose(GObject *object)
{
    GstAVSpliterPipeline *avspliter = GST_AVSPLITER_PIPELINE_CAST(object);
    g_return_if_fail(avspliter != nullptr);

    if (avspliter->uri != nullptr) {
        g_free(avspliter->uri);
        avspliter->uri = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_avspliter_pipeline_finalize(GObject *object)
{
    GstAVSpliterPipeline *avspliter = GST_AVSPLITER_PIPELINE_CAST(object);
    g_return_if_fail(avspliter != nullptr);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_avspliter_pipeline_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(value != nullptr && object != nullptr);
    GstAVSpliterPipeline *avspliter = GST_AVSPLITER_PIPELINE_CAST(object);

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

static void gst_avspliter_pipeline_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    g_return_if_fail(value != nullptr && object != nullptr);
    GstAVSpliterPipeline *avspliter = GST_AVSPLITER_PIPELINE_CAST(object);

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

static GstStateChangeReturn gst_avspliter_pipeline_change_state(GstElement *element, GstStateChange transition)
{
    g_return_val_if_fail(element != nullptr, GST_STATE_CHANGE_FAILURE);
    GstAVSpliterPipeline *avspliter = GST_AVSPLITER_PIPELINE_CAST(element);

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

static gboolean activate_avspliter(GstAVSpliterPipeline *avspliter)
{
    if (avspliter->uri == nullptr) {
        GST_ERROR_OBJECT(avspliter, "not set uri");
        return FALSE;
    }

    gboolean ret = build_urisourcebin(avspliter);
    g_return_val_if_fail(ret, FALSE);

    ret = build_avspliter_bin(avspliter);
    g_return_val_if_fail(ret, FALSE);

    g_object_set(avspliter->urisourcebin, "uri", avspliter->uri);
    return TRUE;
}

static gboolean deactivate_avspliter(GstAVSpliterPipeline *avspliter)
{
    for (GList *node = avspliter->streams; node != nullptr; node = node->next) {
        GstAVSpliterStream *stream = GST_AVSPLITER_STREAM(node->data);
        free_avspliter_stream(avspliter, stream);
    }
    g_list_free(avspliter->streams);
    avspliter->streams = nullptr;

    free_avspliter_bin(avspliter);
    free_urisourcebin(avspliter);
}

static gboolean build_urisourcebin(GstAVSpliterPipeline *avspliter)
{
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

static void free_urisourcebin(GstAVSpliterPipeline *avspliter)
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

static gboolean build_avspliter_bin(GstAVSpliterPipeline *avspliter)
{
    gboolean buildOK = FALSE;

    do {
        avspliter->avsBin = gst_element_factory_make("avspliterbin", "avspliterbin");
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsBin != nullptr,
            "create avspliter bin failed");

        avspliter->avsbinPadAddedId = g_signal_connect(avspliter->avsBin, "pad-added",
            G_CALLBACK(avspliter_bin_pad_added_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinPadAddedId != 0,
            "connect avspliter bin pad-added failed");

        avspliter->avsbinPadRemovedId = g_signal_connect(avspliter->avsBin, "pad-removed",
            G_CALLBACK(avspliter_bin_pad_removed_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinPadRemovedId != 0,
            "connect avspliter bin pad-removed failed");

        avspliter->avsbinNoMorePads = g_signal_connect(avspliter->avsBin, "no-more-pads",
            G_CALLBACK(avspliter_bin_no_more_pads_cb), avspliter);
        CHECK_AND_BREAK_LOG(avspliter, avspliter->avsbinNoMorePads != 0,
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

static void free_avspliter_bin(GstAVSpliterPipeline *avspliter)
{
    if (avspliter->avsbinPadAddedId != 0) {
        g_signal_handler_disconnect(avspliter->avsBin, avspliter->avsbinPadAddedId);
        avspliter->avsbinPadAddedId = 0;
    }

    if (avspliter->avsbinPadRemovedId != 0) {
        g_signal_handler_disconnect(avspliter->avsBin, avspliter->avsbinPadRemovedId);
        avspliter->avsbinPadRemovedId = 0;
    }

    if (avspliter->avsbinNoMorePads != 0) {
        g_signal_handler_disconnect(avspliter->avsBin, avspliter->avsbinNoMorePads);
        avspliter->avsbinNoMorePads = 0;
    }

    if (avspliter->avsBin != nullptr) {
        gst_bin_remove(GST_BIN_CAST(avspliter), avspliter->avsBin);
        avspliter->avsBin = nullptr;
    }
}

static void do_async_start(GstAVSpliterPipeline *avspliter)
{
    GstMessage *message = gst_message_new_async_start(GST_OBJECT_CAST(avspliter));
    g_return_if_fail(message != nullptr);
    avspliter->asyncPending = TRUE;

    GST_BIN_CLASS(parent_class)->handle_message(GST_BIN_CAST(avspliter), message);
}

static void do_async_done(GstAVSpliterPipeline *avspliter)
{
    if (avspliter->asyncPending) {
        GST_DEBUG_OBJECT(avspliter, "posting ASYNC_DONE");
        GstMessage *message = gst_message_new_async_done(GST_OBJECT_CAST(avspliter), GST_CLOCK_TIME_NONE);
        GST_BIN_CLASS (parent_class)->handle_message(GST_BIN_CAST(avspliter), message);

        avspliter->asyncPending = FALSE;
    }
}

static void urisrcbin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliterPipeline *avspliter)
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

static void avspliter_bin_pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliterPipeline *avspliter)
{
    if (!(element != nullptr && pad != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process avspliter bin pad added, param is invalid");
        return;
    }

    GST_DEBUG_OBJECT(avspliter, "avspliter bin src pad %s added", GST_PAD_NAME(pad));

    GstAVSpliterStream *stream = GST_AVSPLITER_STREAM(g_slice_alloc0(sizeof(GstAVSpliterStream)));
    if (stream == nullptr) {
        GST_ERROR_OBJECT(avspliter, "failed to alloc GstAVSpliterStream");
        return;
    }

    if (!try_add_sink(avspliter, pad, stream)) {
        free_avspliter_stream(avspliter, stream);
        return;
    }

    avspliter->streams = g_list_append(avspliter->streams, stream);
}

static void avspliter_bin_pad_removed_cb(GstElement *element, GstPad *pad, GstAVSpliterPipeline *avspliter)
{
    if (!(element != nullptr && pad != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process avspliter bin pad removed, param is invalid");
        return;
    }

    GST_DEBUG_OBJECT(avspliter, "avspliter bin src pad %s removed", GST_PAD_NAME(pad));

    GstAVSpliterStream *stream = nullptr;
    for (GList *node = avspliter->streams; node != nullptr; node = node->next) {
        GstAVSpliterStream *tmp = GST_AVSPLITER_STREAM(node->data);
        if (tmp->avsBinPad == pad) {
            stream = tmp;
            avspliter->streams = g_list_delete_link(avspliter->streams, node);
            break;
        }
    }

    if (stream == nullptr) {
        GST_ERROR_OBJECT(avspliter, "can find the pad %s in internel list", GST_PAD_NAME(pad));
        return;
    }

    free_avspliter_stream(avspliter, stream);
}

static void avspliter_bin_no_more_pads_cb(GstElement *element, GstAVSpliterPipeline *avspliter)
{
    if (!(element != nullptr && avspliter != nullptr)) {
        GST_ERROR_OBJECT(avspliter, "failed to process avspliter bin no-more pad, param is invalid");
        return;
    }

    GST_DEBUG_OBJECT(avspliter, "avspliter bin emit no more pads");

    do_async_done(avspliter);
}

static gboolean try_add_sink(GstAVSpliterPipeline *avspliter, GstPad *pad, GstAVSpliterStream *stream)
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
        GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM, sink_pad_event_cb, avspliter, nullptr);
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

    gst_element_set_locked_state(stream->shmemSink, FALSE);
    return TRUE;
}

static void free_avspliter_stream(GstAVSpliterPipeline *avspliter, GstAVSpliterStream *stream)
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

    g_slice_free(GstAVSpliterStream, stream);
}

static GstPadProbeReturn sink_pad_event_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{

}

gboolean plugin_init(GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_avspliter_pipeline_debug, "avspliter_pipeline", 0, "avspliter pipeline");

    return gst_element_register(plugin, "avspliter_pipeline", GST_RANK_NONE, GST_TYPE_AVSPLITER_PIPELINE);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _avspliter_pipeline,
    "Gstreamer AVSpliter Pipeline",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)