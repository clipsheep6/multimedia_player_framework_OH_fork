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
#include "gst_avspliter_priv.h"
#include "gst_mem_sink.h"

enum {
    PROP_0,
    PROP_URI,
};

GST_DEBUG_CATEGORY_STATIC (gst_avspliter_debug);
#define GST_CAT_DEFAULT gst_avspliter_debug

#define gst_avspliter_parent_class parent_class
G_DEFINE_TYPE(GstAVSpliter, gst_avspliter, GST_TYPE_PIPELINE);

G_DEFINE_TYPE(GstAVSpliterMediaInfo, gst_avspliter_media_info, G_TYPE_OBJECT);
G_DEFINE_TYPE(GstAVSpliterSample, gst_avspliter_sample, G_TYPE_OBJECT);

static void gst_avspliter_dispose(GObject *object);
static void gst_avspliter_finalize(GObject *object);
static void gst_avspliter_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec);
static void gst_avspliter_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_avspliter_change_state(GstElement *element, GstStateChange transition);
static void gst_avspliter_handle_message(GstBin *bin, GstMessage *message);
static gboolean activate_avspliter(GstAVSpliter *avspliter);
static void deactivate_avspliter(GstAVSpliter *avspliter);
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
static void do_async_start(GstAVSpliter *avspliter);
static void do_async_done(GstAVSpliter *avspliter);
static GstPadProbeReturn sink_pad_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data);
gboolean gst_avspliter_seek_and_wait_flush(GstAVSpliter *avspliter, GstClockTime pos, GstSeekFlags flags);
static gboolean try_add_sink(GstAVSpliter *avspliter, GstPad *pad, GstAVSpliterStream *stream);
static void update_stream_media_info(GstAVSpliter *avspliter, GstAVSpliterStream *stream, GstEvent *event);
static void update_stream_pool_info(GstAVSpliter *avspliter, GstAVSpliterStream *stream, GstQuery *query);
static void update_stream_type(GstAVSpliter *avspliter, GstAVSpliterStream *stream);
static void set_stream_cache_limit(GstAVSpliter *avspliter, GstAVSpliterStream *stream);
static GstFlowReturn new_sample_cb(GstMemSink *sink, GstBuffer *sample, gpointer userdata);
static GstFlowReturn new_preroll_cb(GstMemSink *sink, GstBuffer *sample, gpointer userdata);
static void eos_cb(GstMemSink *sink, gpointer userdata);
static void do_flush(GstAVSpliter *avspliter, GstAVSpliterStream *stream, gboolean flushStart);
static gboolean init_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream);
static GstAVSpliterStream *add_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream);
static void free_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream, guint index);
static GstAVSpliterStream *find_avspliter_stream_by_sinkpad(GstAVSpliter *avspliter, GstPad *sinkpad);
static GstAVSpliterStream *find_avspliter_stream_by_sink(GstAVSpliter *avspliter, GstElement *sink);
static GstAVSpliterMediaInfo *gst_avspliter_media_info_copy(GstAVSpliterMediaInfo *mediaInfo);
static gchar *error_message_to_string(GstMessage *msg);

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
    avspliter->urisourcebin = nullptr;
    avspliter->urisrcbinPadAddedId = 0;
    avspliter->avsBin = nullptr;
    avspliter->avsbinPadAddedId = 0;
    avspliter->avsbinPadRemovedId = 0;
    avspliter->avsbinHaveTypeId = 0;
    avspliter->avsbinNoMorePadsId = 0;
    avspliter->noMorePads = FALSE;
    avspliter->asyncPending = FALSE;
    avspliter->mediaInfo = nullptr;
    avspliter->streams = nullptr;
    avspliter->defaultTrackId = INVALID_TRACK_ID;
    avspliter->hasAudio = FALSE;
    avspliter->hasVideo = FALSE;
    g_cond_init(&avspliter->seekCond);
    g_mutex_init(&avspliter->lock);
    avspliter->shutdown = FALSE;
    avspliter->isPlaying = FALSE;
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

    g_cond_clear(&avspliter->seekCond);
    g_mutex_clear(&avspliter->lock);

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
            GST_AVSPLITER_LOCK(avspliter);
            avspliter->shutdown = FALSE;
            GST_AVSPLITER_UNLOCK(avspliter);
            do_async_start(avspliter);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_AVSPLITER_LOCK(avspliter);
            avspliter->shutdown = TRUE;
            GST_AVSPLITER_UNLOCK(avspliter);
           break;
        default:
            break;
    }

    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(avspliter, "element failed to change states");
        do_async_done(avspliter);
        return GST_STATE_CHANGE_FAILURE;
    }

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING: {
            GST_AVSPLITER_LOCK(avspliter);
            avspliter->isPlaying = TRUE;
            GST_AVSPLITER_UNLOCK(avspliter);
            break;
        }
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            GST_AVSPLITER_LOCK(avspliter);
            avspliter->isPlaying = FALSE;
            GST_AVSPLITER_UNLOCK(avspliter);
            do_async_done(avspliter);
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            deactivate_avspliter(avspliter);
            break;
        default:
            break;
    }

    if (ret == GST_STATE_CHANGE_NO_PREROLL || ret == GST_STATE_CHANGE_FAILURE) {
        do_async_done(avspliter);
    }
    return ret;
}

static void gst_avspliter_handle_message(GstBin *bin, GstMessage *message)
{
    g_return_if_fail(bin != nullptr && message != nullptr);
    GstAVSpliter *avspliter = GST_AVSPLITER_CAST(bin);

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            gchar *fullMsg = error_message_to_string(message);
            if (fullMsg != nullptr) {
                GST_ERROR_OBJECT(avspliter, "%s", fullMsg);
                g_free(fullMsg);
            }
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            GstState oldstate = GST_STATE_NULL;
            GstState newstate = GST_STATE_NULL;
            GstState pending = GST_STATE_VOID_PENDING;
            gst_message_parse_state_changed(message, &oldstate, &newstate, &pending);
            if (oldstate == GST_STATE_PLAYING && newstate == GST_STATE_PLAYING) {
                GST_INFO_OBJECT(avspliter, "seek done");
            }
            break;
        }
        default:
            break;
    }

    GST_BIN_CLASS(parent_class)->handle_message(bin, message);
}

GstAVSpliterMediaInfo *gst_avspliter_get_media_info(GstAVSpliter *avspliter)
{
    g_return_val_if_fail(avspliter != nullptr, nullptr);

    GST_AVSPLITER_LOCK(avspliter);

    if (avspliter->shutdown || avspliter->mediaInfo == nullptr) {
        GST_ERROR_OBJECT(avspliter, "error state, currently no any media info probed");
        GST_AVSPLITER_UNLOCK(avspliter);
        return nullptr;
    }

    GstAVSpliterMediaInfo *rst = gst_avspliter_media_info_copy(avspliter->mediaInfo);

    GST_AVSPLITER_UNLOCK(avspliter);
    return rst;
}

gboolean gst_avspliter_do_select_track(GstAVSpliter *avspliter, guint trackIdx, gboolean select)
{
    gboolean res = FALSE;
    for (guint i = 0; i < avspliter->streams->len; i++) {
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
        if (stream->id != trackIdx) {
            continue;
        }
        stream->selected = select;
        GST_INFO_OBJECT(avspliter, "%s track(%u)", select ? "select" : "unselect", trackIdx);

        if (select && stream->needSeekBack) {
            gst_avspliter_seek_and_wait_flush(avspliter, stream->lastPos, GstSeekFlags::GST_SEEK_FLAG_ACCURATE);
            stream->needSeekBack = FALSE;
        }
        res = TRUE;
        break;
    }

    if (!res) {
        GST_ERROR_OBJECT(avspliter, "track id %u is invalid", trackIdx);
    }

    return res;
}

gboolean gst_avspliter_select_track(GstAVSpliter *avspliter, guint trackIdx, gboolean select)
{
    g_return_val_if_fail(avspliter != nullptr, FALSE);

    GST_AVSPLITER_LOCK(avspliter);

    if (avspliter->shutdown || avspliter->streams == nullptr) {
        GST_ERROR_OBJECT(avspliter, "error state, currently no any track probed");
        GST_AVSPLITER_UNLOCK(avspliter);
        return FALSE;
    }

    gboolean res = gst_avspliter_do_select_track(avspliter, trackIdx, select);

    GST_AVSPLITER_UNLOCK(avspliter);
    return res;
}

gboolean try_wait_one_track_sample(GstAVSpliter *avspliter, GstAVSpliterStream *stream)
{
    do {
        if (avspliter->shutdown) {
            return FALSE;
        }

        if (stream->needSeekBack) {
            gst_avspliter_seek_and_wait_flush(avspliter, stream->lastPos, GstSeekFlags::GST_SEEK_FLAG_ACCURATE);
            stream->needSeekBack = FALSE;
        }

        if (stream->cacheSize > 0) {
            return TRUE;
        }

        if (stream->eos) {
            return FALSE;
        }

        for (guint i = 0; i < avspliter->streams->len; i++) {
            GstAVSpliterStream *otherStream = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
            if (otherStream == stream) {
                continue;
            }

            GST_AVSPLITER_STREAM_LOCK(otherStream);
            if (otherStream->cacheSize > 0) {
                GstBuffer *buf = GST_BUFFER_CAST(gst_queue_array_peek_head(otherStream->sampleQueue));
                while (GST_BUFFER_PTS(buf) < otherStream->lastPos) {
                    gst_queue_array_pop_head(otherStream->sampleQueue);
                    gst_buffer_unref(buf);
                    buf = GST_BUFFER_CAST(gst_queue_array_peek_head(otherStream->sampleQueue));
                }
            }

            if (otherStream->eos) {
                GST_AVSPLITER_STREAM_UNLOCK(otherStream);
                continue;
            }

            if (otherStream->cacheSize < otherStream->cacheLimit) {
                GST_AVSPLITER_STREAM_UNLOCK(otherStream);
                continue;
            }

            gst_queue_array_clear(otherStream->sampleQueue);
            otherStream->needSeekBack = TRUE;
            GST_AVSPLITER_STREAM_UNLOCK(otherStream);
        }

        if (!avspliter->shutdown) {
            GST_AVSPLITER_UNLOCK(avspliter);
            GST_AVSPLITER_STREAM_WAIT(stream);
            GST_AVSPLITER_LOCK(avspliter);
        }
    } while (stream->cacheSize == 0);

    return TRUE;
}

TrackPullReturn try_pull_one_track_sample(GstAVSpliter *avspliter,
    GstAVSpliterStream *stream, GstClockTime endtime, GList **allSamples)
{
    TrackPullReturn ret = TRACK_PULL_SUCCESS;

    GST_AVSPLITER_STREAM_LOCK(stream);
    do {
        if (stream->cacheSize == 0) {
            if (!stream->selected) {
                ret = TRACK_PULL_UNSELECTED;
                break;
            }

            if (!try_wait_one_track_sample(avspliter, stream)) {
                if (stream->eos) {
                    GST_INFO_OBJECT(avspliter, "track(%u) is eos, skip it.", stream->id);
                    ret = TRACK_PULL_EOS;
                    break;
                }
                GST_ERROR_OBJECT(avspliter, "track(%u) not eos, but pull sample failed.", stream->id);
                ret = TRACK_PULL_FAILED;
                break;
            }
        }

        GstBuffer *buf = GST_BUFFER_CAST(gst_queue_array_peek_head(stream->sampleQueue));
        if (GST_BUFFER_PTS(buf) < stream->lastPos) {
            gst_queue_array_pop_head(stream->sampleQueue);
            gst_buffer_unref(buf);
            stream->cacheSize -= 1;
            continue;
        }

        if (!stream->selected) {
            ret = TRACK_PULL_UNSELECTED;
            break;
        }

        if (stream->needSeekBack) {
            gst_avspliter_seek_and_wait_flush(avspliter, stream->lastPos, GstSeekFlags::GST_SEEK_FLAG_ACCURATE);
            stream->needSeekBack = FALSE;
            continue;
        }

        if (GST_CLOCK_TIME_IS_VALID(endtime) && GST_BUFFER_PTS(buf) >= endtime) {
            ret = TRACK_PULL_ENDTIME;
            break;
        }

        GstAVSpliterSample *sample = GST_AVSPLITER_SAMPLE_CAST(g_object_new(GST_TYPE_AVSPLITER_SAMPLE, nullptr));
        g_return_val_if_fail(sample != nullptr, TRACK_PULL_FAILED);

        sample->buffer = gst_buffer_ref(buf);
        sample->trackId = stream->id;
        *allSamples = g_list_append(*allSamples, sample);
        stream->cacheSize -= 1;

        if (!try_wait_one_track_sample(avspliter, stream)) {
            if (stream->eos) {
                GST_INFO_OBJECT(avspliter, "track(%u) is eos, mark sample is eos.", stream->id);
                sample->eosFrame = TRUE;
            }
            GST_ERROR_OBJECT(avspliter, "track(%u) not eos, but pull sample failed.", stream->id);
            ret = TRACK_PULL_FAILED;
            break;
        }

        break;
    } while (TRUE);

    GST_AVSPLITER_STREAM_UNLOCK(stream);
    return ret;
}

AVSpliterPullReturn try_pull_one_sample(GstAVSpliter *avspliter, GstClockTime endtime, GstAVSpliterSample **out)
{
    gboolean allSelectedTrackEOS = TRUE;
    gboolean allSelectedTrackEndTime = TRUE;
    GList *allSelectedTrackSample = nullptr;

    for (guint i = 0; i < avspliter->streams->len; i++) {
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
        TrackPullReturn ret = try_pull_one_track_sample(avspliter, stream, endtime, &allSelectedTrackSample);
        if (ret == TRACK_PULL_UNSELECTED || ret == TRACK_PULL_SUCCESS) {
            continue;
        }
        if (ret == TRACK_PULL_EOS) {
            allSelectedTrackEndTime = FALSE;
        }
        if (ret == TRACK_PULL_ENDTIME) {
            allSelectedTrackEOS = FALSE;
        }
        if (ret == TRACK_PULL_FAILED) {
            GST_ERROR_OBJECT(avspliter, "pull sample from track(%u) failed", stream->id);
            g_list_free_full(allSelectedTrackSample, (GDestroyNotify)gst_object_unref);
            return AVS_PULL_FAILED;
        }
    }

    if (allSelectedTrackEndTime || allSelectedTrackEOS) { // allSelectedTrackSample must be nullptr
        GST_INFO_OBJECT(avspliter, "all selected track reach %s", allSelectedTrackEOS ? "eos" : "end of time");
        return allSelectedTrackEOS ? AVS_PULL_REACH_EOS : AVS_PULL_REACH_ENDTIME;
    }

    GstAVSpliterSample *minPtsSample = nullptr;
    GList *minPtsNode = nullptr;
    for (GList *node = allSelectedTrackSample; node != nullptr; node = node->next) {
        GstAVSpliterSample *sample = reinterpret_cast<GstAVSpliterSample *>(node->data);
        if (minPtsSample == nullptr) {
            minPtsSample = sample;
            minPtsNode = node;
        } else if (GST_BUFFER_PTS(minPtsSample->buffer) > GST_BUFFER_PTS(sample->buffer)) {
            minPtsSample = sample;
            minPtsNode = node;
        }
    }

    *out = minPtsSample;
    GstAVSpliterStream *minPtsStream = &g_array_index(avspliter->streams, GstAVSpliterStream, minPtsSample->trackId);
    GST_AVSPLITER_STREAM_LOCK(minPtsStream);
    gst_queue_array_pop_head(minPtsStream->sampleQueue);
    GST_AVSPLITER_STREAM_UNLOCK(minPtsStream);

    for (GList *node = allSelectedTrackSample; node != nullptr; node = node->next) {
        GstAVSpliterSample *sample = reinterpret_cast<GstAVSpliterSample *>(node->data);
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, sample->trackId);
        if (node != minPtsNode) {
            GST_AVSPLITER_STREAM_LOCK(stream);
            if (!stream->needSeekBack) {
                stream->cacheSize += 1;
            }
            GST_AVSPLITER_STREAM_UNLOCK(stream);
            gst_object_unref(sample);
        }
    }

    g_list_free(allSelectedTrackSample);
    return AVS_PULL_SUCCESS;
}

GList *try_pull_samples(GstAVSpliter *avspliter, GstClockTime endtime, guint bufcnt)
{
    GST_DEBUG_OBJECT(avspliter, "start pull samples...");
    GList *result = nullptr;
    gboolean noSelected = TRUE;

    for (guint i = 0; i < avspliter->streams->len; i++) {
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
        if (stream->selected) {
            noSelected = FALSE;
            break;
        }
    }

    if (noSelected) {
        if (avspliter->defaultTrackId == INVALID_TRACK_ID) {
            GST_ERROR_OBJECT(avspliter, "no any track selected and default trackid is invalid");
            return nullptr;
        }
        gst_avspliter_do_select_track(avspliter, avspliter->defaultTrackId, TRUE);
    }

    AVSpliterPullReturn ret;
    guint accum = 0;
    GstAVSpliterSample *outSample = nullptr;
    do {
        ret = try_pull_one_sample(avspliter, endtime, &outSample);
        if (ret == AVS_PULL_SUCCESS) {
            if (outSample == nullptr) {
                GST_ERROR_OBJECT(avspliter, "some error happend, outSample is null");
                break;
            }
            result = g_list_append(result, outSample);
            accum += 1;
        }

        if (ret == AVS_PULL_REACH_ENDTIME) {
            GST_INFO_OBJECT(avspliter, "reach endtime: %" GST_TIME_FORMAT
                " for all selected tracks, pull %u samples", GST_TIME_ARGS(endtime), accum);
        }

        if (ret == AVS_PULL_REACH_EOS) {
            GST_INFO_OBJECT(avspliter, "reach eos for all selected tracks, pull %u samples", accum);
        }
    } while (ret == AVS_PULL_SUCCESS && accum < bufcnt);

    if (noSelected) {
        gst_avspliter_select_track(avspliter, avspliter->defaultTrackId, FALSE);
    }

    GST_DEBUG_OBJECT(avspliter, "finish pull %u samples...", accum);
    return result;
}

GList *gst_avspliter_pull_samples(GstAVSpliter *avspliter,
    GstClockTime starttime, GstClockTime endtime, guint bufcnt)
{
    g_return_val_if_fail(avspliter != nullptr, nullptr);

    GST_AVSPLITER_LOCK(avspliter);

    if (!avspliter->isPlaying || (avspliter->streams == nullptr)) {
        GST_ERROR_OBJECT(avspliter, "error state, refuse to pull samples");
        GST_AVSPLITER_UNLOCK(avspliter);
        return nullptr;
    }

    /* notify the underlying pool of sink: the app has already release the reference to sharedmemory */
    for (guint trackId = 0; trackId < avspliter->streams->len; trackId++) {
        GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, trackId);
        gst_mem_sink_app_render(GST_MEM_SINK_CAST(stream->shmemSink), nullptr);
    }

    if ((bufcnt == 0 && !GST_CLOCK_TIME_IS_VALID(endtime)) ||
        (GST_CLOCK_TIME_IS_VALID(starttime) && endtime <= starttime)) {
        GST_ERROR_OBJECT(avspliter, "invalid param");
        GST_AVSPLITER_UNLOCK(avspliter);
        return nullptr;
    }

    GST_DEBUG_OBJECT(avspliter, "pull sample, starttime: %" GST_TIME_FORMAT
        ", endtime: %" GST_TIME_FORMAT " , bufcnt: %u",
        GST_TIME_ARGS(starttime), GST_TIME_ARGS(endtime), bufcnt);

    if (GST_CLOCK_TIME_IS_VALID(starttime)) {
        GST_AVSPLITER_LOCK(avspliter);
        for (guint trackId = 0; trackId < avspliter->streams->len; trackId++) {
            GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, trackId);
            stream->lastPos = (stream->lastPos > starttime) ? stream->lastPos : starttime;
        }
    }

    GList *result = try_pull_samples(avspliter, endtime, bufcnt);
    GST_AVSPLITER_UNLOCK(avspliter);

    return result;
}

gboolean gst_avspliter_do_seek(GstAVSpliter *avspliter, GstClockTime pos, GstSeekFlags flags)
{
    flags = (GstSeekFlags)(flags | GST_SEEK_FLAG_FLUSH);

    GstEvent *event = gst_event_new_seek(1, GST_FORMAT_TIME, flags, GST_SEEK_TYPE_SET,
        pos, GST_SEEK_TYPE_SET, GST_CLOCK_TIME_NONE);
    g_return_val_if_fail(event != nullptr, FALSE);

    GST_DEBUG_OBJECT(avspliter, "seek to %" GST_TIME_FORMAT, GST_TIME_ARGS(pos));

    gboolean res = gst_element_send_event(GST_ELEMENT_CAST(avspliter), event);

    GST_DEBUG_OBJECT(avspliter, "seek to %" GST_TIME_FORMAT " finished", GST_TIME_ARGS(pos));
    return res;
}

/**
 * Must be called with AVSpliter's lock
 */
gboolean gst_avspliter_seek_and_wait_flush(GstAVSpliter *avspliter, GstClockTime pos, GstSeekFlags flags)
{
    gboolean res = gst_avspliter_do_seek(avspliter, pos, flags);
    if (!res) {
        return res;
    }

    gboolean allStreamFlushed = FALSE;
    do {
        for (guint i = 0; i < avspliter->streams->len; i++) {
            GstAVSpliterStream *stream = &g_array_index(avspliter->streams, GstAVSpliterStream, i);
            if (!stream->isFlushing) {
                allStreamFlushed = FALSE;
                break;
            }
        }

        if (allStreamFlushed) {
            break;
        }

        g_cond_wait(&avspliter->seekCond, &avspliter->lock);
    } while (!avspliter->shutdown);

    GST_DEBUG_OBJECT(avspliter, "wait all flush failed");
    return allStreamFlushed;
}

gboolean gst_avspliter_seek(GstAVSpliter *avspliter, GstClockTime pos, GstSeekFlags flags)
{
    g_return_val_if_fail(avspliter != nullptr, FALSE);
    g_return_val_if_fail(GST_CLOCK_TIME_IS_VALID(pos), FALSE);

    GST_AVSPLITER_LOCK(avspliter);
    if (!avspliter->isPlaying) {
        GST_ERROR_OBJECT(avspliter, "error state, refuse to seek");
    }

    gboolean res = gst_avspliter_seek_and_wait_flush(avspliter, pos, flags);
    GST_AVSPLITER_UNLOCK(avspliter);
    return res;
}

static gboolean activate_avspliter(GstAVSpliter *avspliter)
{
    if (avspliter->uri == nullptr) {
        GST_ERROR_OBJECT(avspliter, "not set uri");
        return FALSE;
    }

    deactivate_avspliter(avspliter);

    static const guint reservedStreamCount = 5;
    avspliter->streams = g_array_sized_new(FALSE, TRUE, sizeof(GstAVSpliterStream), reservedStreamCount);
    if (avspliter->streams == nullptr) {
        GST_ERROR_OBJECT(avspliter, "allocate the avspliter streams failed");
        return FALSE;
    }

    avspliter->mediaInfo = reinterpret_cast<GstAVSpliterMediaInfo *>(
        g_object_new(GST_TYPE_AVSPLITER_MEDIA_INFO, nullptr));
    if (avspliter->mediaInfo == nullptr) {
        GST_ERROR_OBJECT(avspliter, "allocate the avspliter media info failed");
        return FALSE;
    }

    gboolean ret = build_urisourcebin(avspliter);
    g_return_val_if_fail(ret, FALSE);

    ret = build_avspliter_bin(avspliter);
    g_return_val_if_fail(ret, FALSE);

    g_object_set(avspliter->urisourcebin, "uri", avspliter->uri, nullptr);
    return TRUE;
}

static void deactivate_avspliter(GstAVSpliter *avspliter)
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

    GST_AVSPLITER_LOCK(avspliter);
    if (avspliter->mediaInfo->containerCaps != nullptr) {
        gst_object_unref(avspliter->mediaInfo->containerCaps);
    }
    avspliter->mediaInfo->containerCaps = gst_caps_ref(caps);
    GST_AVSPLITER_UNLOCK(avspliter);

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

    GstAVSpliterStream stream = { 0 };
    if (!init_avspliter_stream(avspliter, &stream)) {
        GST_ERROR_OBJECT(avspliter, "init avspliter stream failed");
        free_avspliter_stream(avspliter, &stream, -1);
        return;
    }

    if (!try_add_sink(avspliter, pad, &stream)) {
        free_avspliter_stream(avspliter, &stream, -1);
        return;
    }

    GST_AVSPLITER_LOCK(avspliter);
    if (add_avspliter_stream(avspliter, &stream) == nullptr) {
        GST_ERROR_OBJECT(avspliter, "create stream failed for pad %s added", GST_PAD_NAME(pad));
    }
    GST_AVSPLITER_UNLOCK(avspliter);
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
        GST_WARNING_OBJECT(avspliter, "can find the pad %s in internel list", GST_PAD_NAME(pad));
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
    g_return_val_if_fail(stream->shmemSink != nullptr, FALSE);

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
        return FALSE;
    }

    if (gst_pad_link(pad, stream->sinkpad) != GST_PAD_LINK_OK) {
        GST_ERROR_OBJECT(avspliter, "link pad %s:%s to pad %s:%s failed",
            GST_DEBUG_PAD_NAME(pad), GST_DEBUG_PAD_NAME(stream->sinkpad));
        return FALSE;
    }

    if (gst_element_set_state(stream->shmemSink, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(avspliter, "change shmemsink to paused failed");
        return FALSE;
    }

    gst_mem_sink_set_callback(GST_MEM_SINK(stream->shmemSink), &g_sinkCallbacks, avspliter, nullptr);

    gst_element_set_locked_state(stream->shmemSink, FALSE);

    return TRUE;
}

static gboolean init_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream)
{
    stream->avspliter = avspliter;
    stream->id = avspliter->streams->len;
    stream->avsBinPad = nullptr;
    stream->shmemSink = nullptr;
    stream->inBin = FALSE;
    stream->sinkpad = nullptr;
    stream->sinkPadProbeId = 0;
    gchar *streamId = g_strdup_printf("%d", avspliter->streams->len);
    stream->info = gst_stream_new(streamId, nullptr, GST_STREAM_TYPE_UNKNOWN, GST_STREAM_FLAG_NONE);
    g_free(streamId);

    if (stream->info == nullptr) {
        GST_ERROR_OBJECT(avspliter, "create gststream failed");
        return FALSE;
    }

    stream->selected = FALSE;
    stream->cacheLimit = 0;
    stream->sampleQueue = nullptr;
    stream->cacheSize = 0;
    stream->eos = FALSE;
    stream->lastPos = GST_CLOCK_TIME_NONE;
    stream->needSeekBack = FALSE;
    stream->isFlushing = FALSE;

    return TRUE;
}

/**
 * add new AVSpliterStream, must be called with AVSpliter's lock
 */
static GstAVSpliterStream *add_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream)
{
    g_array_append_val(avspliter->streams, *stream);
    if (avspliter->streams->data == nullptr) {
        GST_ERROR_OBJECT(avspliter, "append val to array failed");
        return nullptr;
    }

    GstAVSpliterStream *ownStream =
        &g_array_index(avspliter->streams, GstAVSpliterStream, avspliter->streams->len - 1);
    g_mutex_init(&ownStream->lock);
    g_cond_init(&ownStream->cond);

    avspliter->mediaInfo->streams = g_list_append(avspliter->mediaInfo->streams, stream->info);
    return stream;
}

static void free_avspliter_stream(GstAVSpliter *avspliter, GstAVSpliterStream *stream, guint index)
{
    if (index >= 0) {
        GST_AVSPLITER_STREAM_LOCK(stream);
    }

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
        GST_AVSPLITER_LOCK(avspliter);
        for (GList *node = avspliter->mediaInfo->streams; node != nullptr; node = node->next) {
            GstStream *gstStream = GST_STREAM_CAST(node->data);
            if (g_str_equal(gstStream->stream_id, stream->info->stream_id)) {
                avspliter->mediaInfo->streams = g_list_delete_link(avspliter->mediaInfo->streams, node);
                break;
            }
        }
        GST_AVSPLITER_UNLOCK(avspliter);
        gst_object_unref(stream->info);
        stream->info = nullptr;
    }

    if (stream->sampleQueue != nullptr) {
        gst_queue_array_free(stream->sampleQueue);
        stream->sampleQueue = nullptr;
    }
    stream->cacheLimit = 0;
    stream->cacheSize = 0;
    stream->eos = FALSE;
    stream->selected = FALSE;

    stream->avsBinPad = nullptr;
    stream->inBin = FALSE;

    if (index >= 0) {
        GST_AVSPLITER_STREAM_UNLOCK(stream);
        g_cond_clear(&stream->cond);
        g_mutex_clear(&stream->lock);

        GST_AVSPLITER_LOCK(avspliter);
        g_array_remove_index(avspliter->streams, index);
        GST_AVSPLITER_UNLOCK(avspliter);
    }
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
            case GST_EVENT_FLUSH_START:
                do_flush(avspliter, stream, TRUE);
                break;
            case GST_EVENT_FLUSH_STOP:
                do_flush(avspliter, stream, FALSE);
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

        GST_AVSPLITER_STREAM_LOCK(avspliterStream);

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

        GST_AVSPLITER_STREAM_UNLOCK(avspliterStream);
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

                GST_AVSPLITER_STREAM_LOCK(stream);
                gst_stream_set_caps(stream->info, caps);
                update_stream_type(avspliter, stream);
                set_stream_cache_limit(avspliter, stream);
                GST_AVSPLITER_STREAM_UNLOCK(stream);
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
                GST_AVSPLITER_LOCK(avspliter);
                GstObject **obj = reinterpret_cast<GstObject **>(&avspliter->mediaInfo->globalTags);
                gst_object_replace(obj, GST_OBJECT_CAST(gst_tag_list_ref(tagList)));
                GST_AVSPLITER_UNLOCK(avspliter);
            } else {
                GST_AVSPLITER_STREAM_LOCK(stream);
                gst_stream_set_tags(stream->info, tagList);
                GST_AVSPLITER_STREAM_UNLOCK(stream);
            }
            GST_DEBUG_OBJECT(avspliter, "got taglist: %" GST_PTR_FORMAT, tagList);
            break;
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

/**
 * update stream type info for AVSpliterStream, and update the default trackId.
 * Must be called with AVSpliterStream's lock, and it will acquire the AVSpliter's lock
 */
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

    GST_AVSPLITER_LOCK(avspliter);

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

    GST_AVSPLITER_UNLOCK(avspliter);
}

/**
 * set the stream sample cache limit according the stream type.
 */
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

    GST_AVSPLITER_STREAM_LOCK(stream);

    if (stream->sampleQueue == nullptr) {
        GST_ERROR_OBJECT(avspliter, "sample queue for trackId(%u) is null, unexpected !", stream->id);
        return GST_FLOW_ERROR;
    }

    GST_DEBUG_OBJECT(avspliter, "trackId(%u) new sample arrived, pts: %" GST_TIME_FORMAT ", cache size: %u/%u",
        stream->id, GST_TIME_ARGS(GST_BUFFER_PTS(sample)), stream->cacheSize, stream->cacheLimit);

    if (stream->isFlushing) {
        GST_DEBUG_OBJECT(avspliter, "is flushing, drop it");
        return GST_FLOW_FLUSHING;
    }

    if (GST_BUFFER_PTS(sample) < stream->lastPos) {
        GST_DEBUG_OBJECT(avspliter, "sample pts is less than lastPos: %" GST_TIME_FORMAT ", drop it.",
            GST_TIME_ARGS(stream->lastPos));
        return GST_FLOW_OK;
    }

    gst_queue_array_push_tail(stream->sampleQueue, gst_buffer_ref(sample));
    GST_AVSPLITER_STREAM_SIGNAL(stream);
    GST_AVSPLITER_STREAM_UNLOCK(stream);

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

    GST_AVSPLITER_STREAM_LOCK(stream);
    if (stream->isFlushing) {
        GST_DEBUG_OBJECT(avspliter, "is flushing, drop it");
        return;
    }

    stream->eos = TRUE;
    GST_AVSPLITER_STREAM_SIGNAL(stream);
    GST_AVSPLITER_STREAM_UNLOCK(stream);
}

static void do_flush(GstAVSpliter *avspliter, GstAVSpliterStream *stream, gboolean flushStart)
{
    GST_DEBUG_OBJECT(avspliter, "received flush event, flush %s", flushStart ? "start" : "stop");

    GST_AVSPLITER_STREAM_LOCK(stream);

    stream->isFlushing = flushStart;
    if (flushStart) {
        gst_queue_array_clear(stream->sampleQueue);
        stream->cacheSize = 0;
        stream->eos = FALSE;

        GST_AVSPLITER_LOCK(avspliter);
        g_cond_signal(&avspliter->seekCond);
        GST_AVSPLITER_UNLOCK(avspliter);
    }

    GST_AVSPLITER_STREAM_UNLOCK(stream);
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

static gchar *error_message_to_string(GstMessage *msg)
{
    GError *err = nullptr;
    gchar *debug = nullptr;
    gchar *message = nullptr;
    gchar *fullMsg = nullptr;

    gst_message_parse_error(msg, &err, &debug);
    if (err == nullptr) {
        if (debug != nullptr) {
            g_free(debug);
        }
        return nullptr;
    }

    message = gst_error_get_message(err->domain, err->code);
    if (debug != nullptr) {
        fullMsg = g_strdup_printf("%s\n%s\n%s", message, err->message, debug);
    } else {
        fullMsg = g_strdup_printf("%s\n%s", message, err->message);
    }

    g_free(message);
    g_free(debug);
    g_clear_error(&err);

    return fullMsg;
}

static void gst_avspliter_media_info_init(GstAVSpliterMediaInfo *mediaInfo)
{
    g_return_if_fail(mediaInfo != nullptr);

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

static void gst_avspliter_sample_init(GstAVSpliterSample *sample)
{
    sample->buffer = nullptr;
    sample->eosFrame = FALSE;
    sample->trackId = INVALID_TRACK_ID;
}

static void gst_avspliter_sample_finalize(GObject *object)
{
    GstAVSpliterSample *sample = GST_AVSPLITER_SAMPLE_CAST(object);
    g_return_if_fail(sample != nullptr);

    if (sample->buffer != nullptr) {
        gst_buffer_unref(sample->buffer);
        sample->buffer = nullptr;
    }

    G_OBJECT_CLASS(gst_avspliter_sample_parent_class)->finalize(object);
}

static void gst_avspliter_sample_class_init(GstAVSpliterSampleClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);

    gobjectClass->finalize = gst_avspliter_sample_finalize;
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
