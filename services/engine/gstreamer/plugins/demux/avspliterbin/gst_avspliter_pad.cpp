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

#include "gst_avspliter_pad.h"
#include "gst_avspliter_utils.h"
#include "gst_avspliter_bin_priv.h"

GType gst_avspliter_pad_get_type(void);
G_DEFINE_TYPE(GstAVSpliterPad, gst_avspliter_pad, GST_TYPE_GHOST_PAD);

static void gst_avspliter_pad_dispose(GObject *object);
static void gst_avspliter_pad_update_caps(GstAVSpliterPad *avspliterPad, GstCaps *caps);
static void gst_avspliter_pad_update_tags(GstAVSpliterPad *avspliterPad, GstTagList *tags);
static GstEvent *gst_avspliter_pad_stream_start_event(GstAVSpliterPad *avspliterPad, GstEvent *event);
static GstPadProbeReturn gst_avspliter_pad_event_probe(GstPad *pad, GstPadProbeInfo *info, gpointer userdata);
static gboolean clear_sticky_events(GstPad *pad, GstEvent **event, gpointer userdata);
static gboolean copy_sticky_events(GstPad *pad, GstEvent **eventptr, gpointer userdata);
static GstPadProbeReturn src_pad_block_callback(GstPad *pad, GstPadProbeInfo *info, gpointer userdata);

static void gst_avspliter_pad_class_init(GstAVSpliterPadClass *klass)
{
    g_return_if_fail(klass != nullptr);

    GObjectClass *gobjectKlass = reinterpret_cast<GObjectClass *>(klass);
    gobjectKlass->dispose = gst_avspliter_pad_dispose;
}

static void gst_avspliter_pad_init(GstAVSpliterPad *pad)
{
    pad->chain = NULL;
    pad->blocked = FALSE;
    pad->exposed = FALSE;
    gst_object_ref_sink(pad);
}

static void gst_avspliter_pad_dispose(GObject *object)
{
    GstAVSpliterPad *avspliterPad = GST_AVSPLITER_PAD_CAST(object);
    g_return_if_fail(avspliterPad != nullptr);

    gst_avspliter_pad_set_target(avspliterPad, nullptr);

    GstObject **obj = reinterpret_cast<GstObject **>(&avspliterPad->activeStream);
    gst_object_replace(obj, nullptr);

    G_OBJECT_CLASS(gst_avspliter_pad_parent_class)->dispose(object);
}

GstAVSpliterPad *gst_avspliter_pad_new(GstAVSpliterChain *chain, GstStaticPadTemplate *tmpl)
{
    GST_DEBUG_OBJECT(chain->avspliter, "making new avspliterpad");

    GstPadTemplate *padTmpl = gst_static_pad_template_get(tmpl);
    g_return_val_if_fail(padTmpl != nullptr, nullptr);
    GstAVSpliterPad *avspliterPad = GST_AVSPLITER_PAD_CAST(
        g_object_new(GST_TYPE_AVSPLITER_PAD, "direction", GST_PAD_SRC, "template", padTmpl, nullptr));
    if (avspliterPad == nullptr) {
        GST_ERROR_OBJECT(chain->avspliter, "new avspliter pad failed");
        gst_object_unref(padTmpl);
        return nullptr;
    }
    (void)gst_ghost_pad_construct(GST_GHOST_PAD_CAST(avspliterPad));

    avspliterPad->chain = chain;
    avspliterPad->avspliter = chain->avspliter;
    // proxypad is the peer of ghostpad's target pad
    GstProxyPad *proxyPad = gst_proxy_pad_get_internal(GST_PROXY_PAD(avspliterPad));
    gst_pad_add_probe(GST_PAD_CAST(proxyPad), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        gst_avspliter_pad_event_probe, avspliterPad, nullptr);

    gst_object_unref(padTmpl);
    gst_object_unref(proxyPad);
    return avspliterPad;
}

void gst_avspliter_pad_set_target(GstAVSpliterPad *avspliterPad, GstPad *target)
{
    GstPad *oldTarget = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(avspliterPad));
    if (oldTarget != nullptr) {
        gst_object_unref(oldTarget);
    }

    if (oldTarget == target) {
        return;
    }

    gst_pad_sticky_events_foreach(GST_PAD_CAST(avspliterPad), clear_sticky_events, nullptr);
    gst_ghost_pad_set_target(GST_GHOST_PAD_CAST(avspliterPad), target);

    if (target == nullptr) {
        GST_LOG_OBJECT(avspliterPad->avspliter, "Setting pad %" GST_PTR_FORMAT " target to null", avspliterPad);
    } else {
        GST_LOG_OBJECT(avspliterPad->avspliter, "Setting pad %" GST_PTR_FORMAT
            " target to %" GST_PTR_FORMAT, avspliterPad, target);
        gst_pad_sticky_events_foreach(target, copy_sticky_events, avspliterPad);
    }
}

void gst_avspliter_pad_active(GstAVSpliterPad *avspliterPad, GstAVSpliterChain *chain)
{
    g_return_if_fail(chain != nullptr);
    avspliterPad->chain = chain; // TODO 是否多余
    gst_pad_set_active(GST_PAD_CAST(avspliterPad), TRUE);
    gst_avspliter_pad_set_block(avspliterPad, TRUE);
}

/* call with dyn lock held */
void gst_avspliter_pad_do_set_block(GstAVSpliterPad *avspliterPad, gboolean needBlock)
{
    GstAVSpliterBin *avspliter = avspliterPad->avspliter;

    GstPad *targetPad = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(avspliterPad));
    if (targetPad == nullptr) {
        return;
    }

    if (needBlock) {
        if (!avspliter->shutDown && (avspliterPad->blockId == 0)) {
            avspliterPad->blockId = gst_pad_add_probe(targetPad,
                static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM |
                GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM), src_pad_block_callback,
                gst_object_ref(avspliterPad), (GDestroyNotify)gst_object_unref);
            gst_object_ref(avspliterPad);
            avspliter->blockedPads = g_list_prepend(avspliter->blockedPads, avspliterPad);
        }
    } else {
        if (avspliterPad->blockId != 0) {
            gst_pad_remove_probe(targetPad, avspliterPad->blockId);
            avspliterPad->blockId = 0;
        }
        avspliterPad->blocked = FALSE;

        GList *node = g_list_find(avspliter->blockedPads, avspliterPad);
        if (node != nullptr) {
            gst_object_unref(avspliterPad);
            avspliter->blockedPads = g_list_delete_link(avspliter->blockedPads, node);
        }
    }

    if (avspliter->shutDown) {
        /* deactivate to force flushing state to prevent NOT_LINKED errors */
        gst_pad_set_active(GST_PAD_CAST(avspliterPad), FALSE);
    }

    gst_object_unref(targetPad);
}

void gst_avspliter_pad_set_block(GstAVSpliterPad *avspliterPad, gboolean needBlock)
{
    GstAVSpliterBin *avspliter = avspliterPad->avspliter;

    DYN_LOCK(avspliter);
    GST_DEBUG_OBJECT(avspliterPad, "blocking pad: %d", needBlock);
    gst_avspliter_pad_do_set_block(avspliterPad, needBlock);
    DYN_UNLOCK(avspliter);
}

static void gst_avspliter_pad_update_caps(GstAVSpliterPad *avspliterPad, GstCaps *caps)
{
    if (caps == nullptr || avspliterPad->activeStream == nullptr)  {
        return;
    }

    GST_DEBUG_OBJECT(avspliterPad, "Storing caps %" GST_PTR_FORMAT
        " on stream %" GST_PTR_FORMAT, caps, avspliterPad->activeStream);

    if (gst_caps_is_fixed(caps)) {
        gst_stream_set_caps(avspliterPad->activeStream, caps);
    }

    if (gst_stream_get_stream_type(avspliterPad->activeStream) == GST_STREAM_TYPE_UNKNOWN)  {
        GstStreamType newType = guess_stream_type_from_caps(caps);
        if (newType != GST_STREAM_TYPE_UNKNOWN) {
            gst_stream_set_stream_type(avspliterPad->activeStream, newType);
        }
    }
}

static void gst_avspliter_pad_update_tags(GstAVSpliterPad *avspliterPad, GstTagList *tags)
{
    if (tags == nullptr || avspliterPad->activeStream == nullptr) {
        return;
    }

    if (gst_tag_list_get_scope(tags) != GST_TAG_SCOPE_STREAM) {
        return;
    }

    GST_DEBUG_OBJECT(avspliterPad, "Storing new tags %" GST_PTR_FORMAT
        " on stream %" GST_PTR_FORMAT, tags, avspliterPad->activeStream);
    gst_stream_set_tags(avspliterPad->activeStream, tags);
}

static GstEvent *gst_avspliter_pad_stream_start_event(GstAVSpliterPad *avspliterPad, GstEvent *event)
{
    const gchar *streamId = nullptr;
    gst_event_parse_stream_start(event, &streamId);

    gboolean repeatEvent = FALSE;
    if (avspliterPad->activeStream != nullptr && g_str_equal(avspliterPad->activeStream->stream_id, streamId)) {
        repeatEvent = TRUE;
    }

    GstStream *stream = nullptr;
    gst_event_parse_stream(event, &stream);
    if (stream == nullptr) {
        GstCaps *caps = gst_pad_get_current_caps(GST_PAD_CAST(avspliterPad));
        if (caps == nullptr) {
            GstPad *peer = gst_ghost_pad_get_target(GST_GHOST_PAD(avspliterPad));
            caps = gst_pad_get_current_caps(peer);
            gst_object_unref(peer);
        }
        if (caps == nullptr && avspliterPad->chain != nullptr && avspliterPad->chain->startCaps != nullptr) {
            caps = gst_caps_ref(avspliterPad->chain->startCaps);
        }

        GST_DEBUG_OBJECT(avspliterPad,
            "Saw stream_start with no GstStream, Adding one. Caps %" GST_PTR_FORMAT, caps);

        if (repeatEvent) {
            stream = GST_STREAM_CAST(gst_object_ref(avspliterPad->activeStream));
        } else {
            stream = gst_stream_new(streamId, nullptr, GST_STREAM_TYPE_UNKNOWN, GST_STREAM_FLAG_NONE);
            GstObject **obj = reinterpret_cast<GstObject **>(&avspliterPad->activeStream);
            gst_object_replace(obj, GST_OBJECT_CAST(stream));
        }

        if (caps != nullptr) {
            gst_avspliter_pad_update_caps(avspliterPad, caps);
            gst_caps_unref(caps);
        }

        event = gst_event_make_writable(event);
        gst_event_set_stream(event, stream);
    }

    gst_object_unref(stream);
    GST_LOG_OBJECT (avspliterPad, "Saw stream %s (GstStream %" GST_PTR_FORMAT ")", stream->stream_id, stream);
    return event;
}

static GstPadProbeReturn gst_avspliter_pad_event_probe(GstPad *pad, GstPadProbeInfo *info, gpointer userdata)
{
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    g_return_val_if_fail(event != nullptr, GST_PAD_PROBE_DROP);
    GstObject *parent = gst_pad_get_parent(pad);
    GstAVSpliterPad *avspliterPad = GST_AVSPLITER_PAD_CAST(parent);
    g_return_val_if_fail(avspliterPad != nullptr, GST_PAD_PROBE_DROP);
    // TODO ??? 为什么默认drop掉，都drop掉了，下游岂不是收不到事件
    GstPadProbeReturn ret = GST_PAD_PROBE_DROP;

    GST_LOG_OBJECT(pad, "%s avspliterpad: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(event), avspliterPad);

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_CAPS: {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            gst_avspliter_pad_update_caps(avspliterPad, caps);
            break;
        }
        case GST_EVENT_TAG: {
            GstTagList *tags = nullptr;
            gst_event_parse_tag(event, &tags);
            gst_avspliter_pad_update_tags(avspliterPad, tags);
            break;
        }
        case GST_EVENT_STREAM_START: {
            GST_PAD_PROBE_INFO_DATA(info) = gst_avspliter_pad_stream_start_event(avspliterPad, event);
            break;
        }
        case GST_EVENT_EOS: {
            GST_INFO_OBJECT(pad, "EOS received");
            ret = GST_PAD_PROBE_OK;
            break;
        }
        default:
            break;
    }

    gst_object_unref(parent);
    return ret;
}

static gboolean clear_sticky_events(GstPad *pad, GstEvent **event, gpointer userdata)
{
    GST_DEBUG_OBJECT(pad, "clearing sticky event %" GST_PTR_FORMAT, *event);
    gst_event_unref(*event);
    *event = nullptr;
    return TRUE;
}

static gboolean copy_sticky_events(GstPad *pad, GstEvent **eventptr, gpointer userdata)
{
    GstAVSpliterPad *avspliterPad = GST_AVSPLITER_PAD_CAST(userdata);
    GstEvent *event = gst_event_ref(*eventptr);
    g_return_val_if_fail(event != nullptr, FALSE);

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_CAPS: {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            gst_avspliter_pad_update_caps(avspliterPad, caps);
            break;
        }
        case GST_EVENT_STREAM_START: {
            event = gst_avspliter_pad_stream_start_event(avspliterPad, event);
            break;
        }
        default:
            break;
    }

    GST_DEBUG_OBJECT(avspliterPad, "store stick event %" GST_PTR_FORMAT, event);
    gst_pad_store_sticky_event(GST_PAD_CAST(avspliterPad), event);
    gst_event_unref(event);

    return TRUE;
}

static GstPadProbeReturn src_pad_block_callback(GstPad *pad, GstPadProbeInfo *info, gpointer userdata)
{
    g_return_val_if_fail(pad != nullptr && info != nullptr && userdata != nullptr, GST_PAD_PROBE_DROP);
    GstAVSpliterPad *avspliterPad = reinterpret_cast<GstAVSpliterPad *>(userdata);

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
        g_return_val_if_fail(event != nullptr, GST_PAD_PROBE_DROP);
        GST_LOG_OBJECT(pad, "Seeing event '%s'", GST_EVENT_TYPE_NAME(event));

        /**
         * do not block on sticky event or out-of-band events otherwise the allocation query from
         * demuxr might block the loop thread
         */
        if (!GST_EVENT_IS_SERIALIZED(event)) {
            GST_LOG_OBJECT(pad, "Letting OOB event through");
            return GST_PAD_PROBE_PASS;
        }

        if (GST_EVENT_IS_STICKY(event) && GST_EVENT_TYPE(event) != GST_EVENT_EOS) {
            GstPad *peer = gst_pad_get_peer(pad);  // the GstProxyPad's internel pad
            (void)gst_pad_send_event(peer, event);
            gst_object_unref(peer);
            GST_LOG_OBJECT(pad, "Manually pushed sticky event through");
            return GST_PAD_PROBE_HANDLED;
        }
    }

    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM) {
        GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
        g_return_val_if_fail(query != nullptr, GST_PAD_PROBE_DROP);

        if (!GST_QUERY_IS_SERIALIZED(query)) {
            GST_LOG_OBJECT(pad, "Letting non-serialized query through");
            return GST_PAD_PROBE_PASS;
        }

        /**
         * do not block on allocation queries before we have caps, this would deadlock because we
         * are need the GST_EVENT_CAPS to continue everything, but the event is serialized.
         */
        if (!gst_pad_has_current_caps(pad)) {
            GST_LOG_OBJECT(pad, "letting serilized query before caps through");
            return GST_PAD_PROBE_PASS;
        }
    }

    GstAVSpliterChain *chain = avspliterPad->chain;
    GstAVSpliterBin *avspliter = avspliterPad->avspliter;
    g_return_val_if_fail(avspliter != nullptr, GST_PAD_PROBE_DROP);
    GST_LOG_OBJECT(avspliterPad, "blocked: avspliterPad->chain: 0x%06" PRIxPTR, FAKE_POINTER(chain));
    avspliterPad->blocked = TRUE;

    EXPOSE_LOCK(avspliter);
    if (avspliter->chain != nullptr) {
        // got EOS event or Buffer, we try to expose the whole AVSpliterBin immediately!
        if (!gst_avspliter_bin_expose(avspliter)) {
            GST_WARNING_OBJECT(avspliter, "Couldn't expose group");
        }
    }
    EXPOSE_UNLOCK(avspliter);

    return GST_PAD_PROBE_OK;
}
