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

#include "gst_avspliter_chain.h"
#include "gst/pbutils/descriptions.h"
#include "gst/pbutils/missing-plugins.h"
#include "gst/playback/gstplaybackutils.h"
#include "gst_avspliter_utils.h"
#include "gst_avspliter_bin_priv.h"

#define GST_AVSPLITER_ELEMENT(obj) (GstAVSpliterElement *)(obj)

typedef struct _GstAVSpliterElement GstAVSpliterElement;

struct _GstAVSpliterElement {
    GstElement *element;
    GstElement *capsfilter;
    gulong padAddedSignalId;
    gulong padRemoveSignalId;
    gulong noMorePadsSignalId;
};

typedef struct _GstPendingPad GstPendingPad;

struct _GstPendingPad {
    GstPad *pad;
    GstAVSpliterChain *chain;
    gulong eventProbeId;
    gulong notifyCapsSignalId;
};

static void pending_pad_free(GstPendingPad *ppad);
static GstPadProbeReturn pending_pad_event_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data);
static void pending_pad_caps_notify_cb(GstPad *pad, GParamSpec *unused, GstAVSpliterChain *chain);
static void pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliterChain *chain);
static void pad_removed_cb(GstElement *element, GstPad *pad, GstAVSpliterChain *chain);
static void no_more_pads_cb(GstElement *element, GstAVSpliterChain *chain);

enum AnalyzeResult {
    ANALYZE_UNKNOWN_TYPE,
    ANALYZE_DEFER,
    ANALYZE_CONTINUE,
    ANALYZE_EXPOSE,
};

struct AnalyzeParams {
    GstPad *pad;   // new pad
    GstCaps *caps; // caps for new pad
    GstElement *src; // new pad's parent element
    GstAVSpliterPad *avspliterPad; // the ghost pad for new pad
    GList *factories; // the factories for creating the next element
    gchar *deadDetails; // if the caps is unknow type, record the detail for current analyze process.
    gboolean isParserConverter; // the src is parser_converter
};

static gboolean defer_caps_setup(GstAVSpliterChain *chain, AnalyzeParams *params);
static void mark_end_pad(GstAVSpliterChain *chain, GstCaps *caps, GstPad *pad, GstAVSpliterPad *dpad);
static void process_unknown_type(GstAVSpliterChain *chain, AnalyzeParams *params);
static GstCaps *filter_caps_for_capsfilter(AnalyzeParams *params);
static gboolean connect_new_pad(GstAVSpliterChain *chain, AnalyzeParams *params);
static void process_analyze_result(GstAVSpliterChain *chain, AnalyzeParams *params, AnalyzeResult result);
static GstAVSpliterChain *add_children_chain(GstAVSpliterChain *chain, AnalyzeParams *params);
static AnalyzeResult analyze_caps_before_continue(GstAVSpliterChain *chain, AnalyzeParams *params);
static AnalyzeResult add_capsfilter_for_parser_converter(GstAVSpliterChain *chain, AnalyzeParams *params);

struct ConnectPadParams {
    GstAVSpliterPad *avspliterPad;
    GstAVSpliterElement *avspliterElem;
    GstPad *srcpad;
    GstCaps *srccaps;
    GstElement *element;
    GstPad *sinkpad;
    GstElement *nextElem;
    GList *nextSrcPads;
    gboolean addToBin;
    gboolean isParserConverter;
    gboolean isSimpleDemuxer;
    GString *errorDetails;
};

static void init_connect_pad_param(AnalyzeParams *params, ConnectPadParams *connParams);
static gboolean check_parser_repeat_in_chain(GstElementFactory *factory,
    GstAVSpliterChain *chain, ConnectPadParams *params);
static gboolean add_new_demux_element(GstAVSpliterChain *chain, ConnectPadParams *params);
static void cleanup_add_new_element(GstAVSpliterChain *chain, ConnectPadParams *params, gboolean ret);
static void free_demux_element(GstAVSpliterChain *chain, GstAVSpliterElement *element);
static gboolean try_add_new_element(GstAVSpliterChain *chain,
    ConnectPadParams *params, GstElementFactory *factory);
static void collect_new_element_srcpads(GstAVSpliterChain *chain, ConnectPadParams *params);
static void connect_new_element_srcpads(GstAVSpliterChain *chain, ConnectPadParams *params);
static gboolean change_new_element_to_pause(GstAVSpliterChain *chain, ConnectPadParams *params);

static gboolean all_child_chain_is_complete(GstAVSpliterChain *chain);

GstAVSpliterChain *gst_avspliter_chain_new(
    GstAVSpliterBin *avspliter, GstAVSpliterChain *parent, GstPad *startPad, GstCaps *startCaps)
{
    GstAVSpliterChain *chain = g_slice_new0(GstAVSpliterChain);
    if (chain == nullptr) {
        GST_ERROR_OBJECT(avspliter, "create new chain failed");
        return nullptr;
    }

    GST_DEBUG_OBJECT(avspliter, "Createing new chain 0x%06" PRIXPTR " with parent chain: 0x%06" PRIXPTR,
        FAKE_POINTER(chain), FAKE_POINTER(parent));

    chain->parentChain = parent;
    chain->avspliter = avspliter;
    g_mutex_init(&chain->lock);
    chain->startPad = GST_PAD_CAST(gst_object_ref(startPad));
    if (startCaps != nullptr) {
        chain->startCaps = gst_caps_ref(startCaps);
    }
    chain->hasParser = FALSE;
    chain->elements = nullptr;
    chain->endCaps = nullptr;
    chain->endPad = nullptr;
    chain->isDead = FALSE;
    chain->isDemuxer = FALSE;
    chain->noMorePads = FALSE;
    chain->pendingPads = nullptr;
    chain->childChains = nullptr;
    chain->currentPad = nullptr;
    chain->deadDetails = nullptr;

    return chain;
}

void gst_avspliter_chain_free(GstAVSpliterChain *chain)
{
    CHAIN_LOCK(chain);
    GST_DEBUG_OBJECT(chain->avspliter, "free chain 0x%06" PRIXPTR, FAKE_POINTER(chain));

    for (GList *node = chain->childChains; node != nullptr; node = node->next) {
        gst_avspliter_chain_free(reinterpret_cast<GstAVSpliterChain *>(node->data));
    }
    g_list_free(chain->childChains);
    chain->childChains = nullptr;

    gst_object_replace(reinterpret_cast<GstObject **>(&chain->currentPad), nullptr);

    for (GList *node = chain->pendingPads; node != nullptr; node = node->next) {
        pending_pad_free(reinterpret_cast<GstPendingPad *>(node->data));
    }
    g_list_free(chain->pendingPads);
    chain->pendingPads = nullptr;

    for (GList *node = chain->elements; node != nullptr; node = node->next) {
        GstAVSpliterElement *delem = reinterpret_cast<GstAVSpliterElement *>(node->data);
        free_demux_element(chain, delem);
    }
    g_list_free(chain->elements);
    chain->elements = nullptr;

    if (chain->endPad != nullptr) {
        if (chain->endPad->exposed) {
            GstPad *endpad = GST_PAD_CAST(chain->endPad);
            GST_DEBUG_OBJECT(chain->avspliter, "Removing pad %s:%s", GST_DEBUG_PAD_NAME(endpad));
            gst_pad_push_event(endpad, gst_event_new_eos());
            gst_element_remove_pad(GST_ELEMENT_CAST(chain->avspliter), endpad);
        }

        gst_avspliter_pad_set_target(chain->endPad, nullptr);
        chain->endPad->exposed = FALSE;
        gst_object_unref(chain->endPad);
        chain->endPad = nullptr;
    }

    FREE_GST_OBJECT(chain->currentPad);
    FREE_GST_OBJECT(chain->startPad);
    FREE_GST_OBJECT(chain->startCaps);
    FREE_GST_OBJECT(chain->endCaps);

    if (chain->deadDetails != nullptr) {
        g_free(chain->deadDetails);
        chain->deadDetails = nullptr;
    }

    CHAIN_UNLOCK(chain);

    g_mutex_clear(&chain->lock);
    g_slice_free(GstAVSpliterChain, chain);
}

/* called when a new pad is discovered. It will perform some basic actions
 * before trying to link something to it.
 *
 *  - Check the caps, don't do anything when there are no caps or when they have no good type.
 *  - signal AUTOPLUG_CONTINUE to check if we need to continue autoplugging this pad.
 *  - if the caps are non-fixed, setup a handler to continue autoplugging when the caps become
 *    fixed (connect to notify::caps).
 *  - get list of factories to autoplug.
 *  - continue autoplugging to one of the factories.
 */
void gst_avspliter_chain_analyze_new_pad(GstElement *src, GstPad *pad, GstCaps *caps, GstAVSpliterChain *chain)
{
    GstAVSpliterBin *avspliter = chain->avspliter;
    GST_DEBUG_OBJECT(avspliter, "Pad %s: %s caps: %" GST_PTR_FORMAT, GST_DEBUG_PAD_NAME(pad), caps);

    AnalyzeResult ret = ANALYZE_UNKNOWN_TYPE;
    AnalyzeParams params = { pad, caps, src, nullptr, nullptr, nullptr, FALSE };
    params.isParserConverter = check_is_parser_converter(params.src);

    do {
        // check whether the pad is from the last element in this chain.
        if (chain->elements != nullptr) {
            GstAVSpliterElement *lastDemuxElem = GST_AVSPLITER_ELEMENT(chain->elements->data);
            if (src != lastDemuxElem->element && src != lastDemuxElem->capsfilter) {
                GST_ERROR_OBJECT(avspliter, "New pad not from the last element in this chain");
                return;
            }
        }

        if (chain->isDemuxer) {
            chain = add_children_chain(chain, &params);
            CHECK_AND_BREAK_LOG(avspliter, chain != nullptr, "Add children for demuxer failed");
        }

        // we need update the current avspliter pad for this chain
        if (chain->currentPad == nullptr) {
            chain->currentPad = gst_avspliter_pad_new(chain, chain->avspliter->srcTmpl);
            CHECK_AND_BREAK_LOG(avspliter, chain->currentPad != nullptr,  "create current avspliterPad failed");
        }

        gst_pad_set_active(GST_PAD_CAST(chain->currentPad), TRUE);
        gst_avspliter_pad_set_target(chain->currentPad, pad);
        params.avspliterPad = GST_AVSPLITER_PAD(gst_object_ref(chain->currentPad));

        // check whether this caps has good type and whether we need to expose the pad directly.
        ret = analyze_caps_before_continue(chain, &params);
        CHECK_AND_BREAK(ret == ANALYZE_CONTINUE);

        ret = add_capsfilter_for_parser_converter(chain, &params);
        CHECK_AND_BREAK(ret == ANALYZE_CONTINUE);

        if (!connect_new_pad(chain, &params)) {
            ret = ANALYZE_UNKNOWN_TYPE;
            break;
        }
    } while (0);

    return process_analyze_result(chain, &params, ret);
}

/* Must called with expose lock */
gboolean gst_avspliter_chain_is_complete(GstAVSpliterChain *chain)
{
    if (chain == nullptr) {
        return FALSE;
    }

    gboolean complete = FALSE;

    CHAIN_LOCK(chain);
    do {
        if (chain->avspliter->shutDown) {
            break;
        }

        if (chain->isDead) {
            complete = TRUE;
            break;
        }

        if (chain->endPad != nullptr && (chain->endPad->blocked || chain->endPad->exposed)) {
            complete = TRUE;
            break;
        }

        if (chain->isDemuxer && chain->noMorePads) {
            complete = all_child_chain_is_complete(chain);
            break;
        }

        if (chain->hasParser) {
            complete = TRUE;
            break;
        }
    } while (0);

    CHAIN_UNLOCK(chain);
    GST_DEBUG_OBJECT(chain->avspliter, "Chain 0x%06" PRIXPTR " is complete: %d",
        FAKE_POINTER(chain), complete);
    return complete;
}

/* muse be called with chain lock */
gboolean gst_avspliter_chain_expose(GstAVSpliterChain *chain, GList **endpads,
    gboolean *missingPlugin, GString *missingPluginDetails)
{
    if (chain->isDead) {
        *missingPlugin = TRUE;
        if (chain->endCaps == nullptr) {
            return TRUE;
        }
        if (chain->deadDetails != nullptr) {
            g_string_append(missingPluginDetails, chain->deadDetails);
            g_string_append_c(missingPluginDetails, '\n');
        } else {
            gchar *desc = gst_pb_utils_get_codec_description(chain->endCaps);
            gchar *capsStr = gst_caps_to_string(chain->endCaps);
            g_string_append_printf(missingPluginDetails, "Missing parser: %s (%s)\n", desc, capsStr);
            g_free(capsStr);
            g_free(desc);
        }
        return TRUE;
    }

    /* The Chain has a pending pad from a parser, let's expose it as the endpad. */
    if (chain->endPad == nullptr && chain->hasParser && chain->pendingPads != nullptr) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(chain->pendingPads->data);
        GstPad *endpad = GST_PAD_CAST(gst_object_ref(ppad->pad));
        GstElement *elem = GST_ELEMENT_CAST(gst_object_get_parent(GST_OBJECT_CAST(endpad)));
        chain->pendingPads = g_list_remove(chain->pendingPads, ppad);
        pending_pad_free(ppad);
        GST_DEBUG_OBJECT(chain->avspliter, "Exposing pad %" GST_PTR_FORMAT " with incomplete caps"
            " because it's parsed", endpad);
        mark_end_pad(chain, nullptr, endpad, chain->currentPad);
        gst_object_unref(endpad);
        gst_object_unref(elem);
    }

    if (chain->endPad != nullptr) {
        *endpads = g_list_prepend(*endpads, gst_object_ref(chain->endPad));
        return TRUE;
    }

    if (chain->childChains == nullptr) {
        return FALSE;
    }

    gboolean ret = FALSE;
    for (GList *node = chain->childChains; node != nullptr; node = node->next) {
        GstAVSpliterChain *child = reinterpret_cast<GstAVSpliterChain *>(node->data);
        CHAIN_LOCK(child);
        ret |= gst_avspliter_chain_expose(child, endpads, missingPlugin, missingPluginDetails);
        CHAIN_UNLOCK(child);
    }

    return ret;
}

static gboolean all_child_chain_is_complete(GstAVSpliterChain *chain)
{
    gboolean childComplete = TRUE;
    for (GList *node = chain->childChains; node != nullptr; node = node->next) {
        GstAVSpliterChain *child = reinterpret_cast<GstAVSpliterChain *>(node->data);
        /* if the child chain blocked, we must complete this whole demuxer, since
            * everything is synchronous, we can't proceed otherwise. */
        if (child->endPad != nullptr && chain->endPad->blocked) {
            break;
        }

        if (!gst_avspliter_chain_is_complete(child)) {
            childComplete = FALSE;
            break;
        }
    }
    GST_DEBUG_OBJECT(chain->avspliter, "chain: 0x%06" PRIXPTR " is complete: %d",
        FAKE_POINTER(chain), childComplete);

    return childComplete;
}

static GstPadProbeReturn pending_pad_event_cb(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(data);
    GstAVSpliterChain *chain = ppad->chain;
    GstAVSpliterBin *avspliter = chain->avspliter;

    if (event == nullptr || ppad == nullptr || chain == nullptr || avspliter == nullptr) {
        return GST_PAD_PROBE_DROP;
    }

    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_EOS: {
            GST_DEBUG_OBJECT(pad, "Received EOS on pad %s:%s without fixed caps, this stream ended too early",
                             GST_DEBUG_PAD_NAME(pad));
            chain->isDead = TRUE;
            gst_object_replace(reinterpret_cast<GstObject **>(&chain->currentPad), nullptr);
            EXPOSE_LOCK(avspliter);
            if (gst_avspliter_chain_is_complete(avspliter->chain)) {
                gst_avspliter_bin_expose(avspliter);
            }
            EXPOSE_UNLOCK(avspliter);
            break;
        }
        default:
            break;
    }

    return GST_PAD_PROBE_OK;
}

static void pending_pad_caps_notify_cb(GstPad *pad, GParamSpec *unused, GstAVSpliterChain *chain)
{
    GstElement *element = GST_ELEMENT_CAST(gst_pad_get_parent(pad));
    if (pad == nullptr || element == nullptr || chain == nullptr) {
        return;
    }

    GST_LOG_OBJECT(pad, "Notified caps for pad %s:%s", GST_DEBUG_PAD_NAME(pad));

    CHAIN_LOCK(chain);
    for (GList *node = chain->pendingPads; node != nullptr; node = node->next) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(node->data);
        if (ppad->pad == pad) {
            pending_pad_free(ppad);
            chain->pendingPads = g_list_delete_link(chain->pendingPads, node);
            break;
        }
    }
    CHAIN_UNLOCK(chain);

    pad_added_cb(element, pad, chain);
    gst_object_unref(element);
}

static void pending_pad_free(GstPendingPad *ppad)
{
    if (ppad->eventProbeId != 0) {
        gst_pad_remove_probe(ppad->pad, ppad->eventProbeId);
    }
    if (ppad->notifyCapsSignalId != 0) {
        g_signal_handler_disconnect(ppad->pad, ppad->notifyCapsSignalId);
    }
    gst_object_unref(ppad->pad);
    g_slice_free(GstPendingPad, ppad);
}

static GstAVSpliterChain *add_children_chain(GstAVSpliterChain *chain, AnalyzeParams *params)
{
    if (chain->currentPad != nullptr) {
        gst_object_unref(chain->currentPad);
        chain->currentPad = nullptr;
    }

    if (chain->elements != nullptr) {
        GstAVSpliterElement *demux = GST_AVSPLITER_ELEMENT(chain->elements->data);
        /* if this is not a dynamic pad demuxer, directly set the no more pads flag */
        if (demux == nullptr || demux->noMorePadsSignalId == 0) {
            chain->noMorePads = TRUE;
        }
    }

    /* for demuxer's downstream, we add new children chain to current chain */
    GstAVSpliterChain *oldchain = chain;
    chain = gst_avspliter_chain_new(chain->avspliter, oldchain, params->pad, params->caps);
    g_return_val_if_fail(chain != nullptr, nullptr);

    CHAIN_LOCK(oldchain);
    oldchain->childChains = g_list_prepend(oldchain->childChains, chain);
    if (oldchain->childChains == nullptr) {
        gst_avspliter_chain_free(chain);
        GST_ELEMENT_ERROR(oldchain->avspliter, CORE, FAILED, ("failed to process new pad for demuxer"), (""));
        CHAIN_UNLOCK(oldchain);
        return nullptr;
    }
    CHAIN_UNLOCK(oldchain);

    return chain;
}

static gboolean analyze_caps_for_non_parser_converter(AnalyzeParams *params)
{
    if (!params->isParserConverter) {
        if (!gst_caps_is_fixed(params->caps)) {
            return FALSE;
        } else {
            // if we can get caps, it means caps event received at this pad.
            params->caps = gst_pad_get_current_caps(params->pad);
            if (params->caps == nullptr) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**
 * check whether this caps has good type and whether we need to expose the pad directly.
 * - if the caps is invalid, or error happened, return ANALYZE_UNKNOWN_TYPE.
 * - if the caps is not fixed, return ANALYZE_DEFER
 * - if the caps is fixed, emit the autoplug-continue to app for ensureing whether we need to
 *   expose the pad directly.
 *
 * otherwise, we can continue to add new element to process this caps, then, return ANALYZE_CONTINUE
 * and a list of compatible element factories for this caps.
 */
static AnalyzeResult analyze_caps_before_continue(GstAVSpliterChain *chain, AnalyzeParams *params)
{
    if (params->caps == nullptr || gst_caps_is_empty(params->caps)) {
        return ANALYZE_UNKNOWN_TYPE;
    }

    if (gst_caps_is_any(params->caps)) {
        return ANALYZE_DEFER;
    }

    gboolean appcontinue = TRUE;
    if (gst_caps_is_fixed(params->caps)) {
        appcontinue = gst_avspliter_bin_emit_appcontinue(
            chain->avspliter, GST_PAD_CAST(params->avspliterPad), params->caps);
    }

    if (!appcontinue) {
        GST_LOG_OBJECT(chain->avspliter, "Pad is final. auto-plug-continue: %d", appcontinue) ;
        return ANALYZE_EXPOSE;
    }

    if (!analyze_caps_for_non_parser_converter(params)) {
        GST_DEBUG_OBJECT(chain->avspliter, "no final caps set yet, delayig autopluging");
        return ANALYZE_DEFER;
    }

    params->factories = gst_avspliter_bin_find_elem_factories(chain->avspliter, params->caps);
    if (params->factories == nullptr) {
        return ANALYZE_UNKNOWN_TYPE;
    }

    return ANALYZE_CONTINUE;
}

static GstCaps *filter_caps_for_capsfilter(AnalyzeParams *params)
{
    GstCaps *filteredCaps = gst_caps_new_empty();
    g_return_val_if_fail(filteredCaps != nullptr, nullptr);

    for (GList *node = params->factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST(node->data);
        if (factory == nullptr) {
            continue;
        }

        GST_DEBUG("Trying factory %s", gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
        if (gst_element_get_factory(params->src) == factory ||
            gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_PARSER)) {
            GST_DEBUG("Skipping factory");
            continue;
        }

        const GList *tmps = gst_element_factory_get_static_pad_templates(factory);
        for (; tmps != nullptr; tmps = tmps->next) {
            GstStaticPadTemplate *st = reinterpret_cast<GstStaticPadTemplate *>(tmps->data);
            if (st == nullptr || st->direction != GST_PAD_SINK || st->presence != GST_PAD_ALWAYS) {
                continue;
            }
            GstCaps *tcaps = gst_static_pad_template_get_caps(st);
            if (tcaps == nullptr) {
                continue;
            }
            GstCaps *intersection = gst_caps_intersect_full(tcaps, params->caps, GST_CAPS_INTERSECT_FIRST);
            filteredCaps = gst_caps_merge(filteredCaps, intersection);
            gst_caps_unref(tcaps);
        }
    }

    // Append the parser caps to prevent any not-negotiated errors
    filteredCaps = gst_caps_merge(filteredCaps, gst_caps_ref(params->caps));
    return filteredCaps;
}

/**
 * for parser_converter, the output caps maybe unfixed, we try add a capfilter to downstream
 * to fixate the output caps of this chain. TRUE if success, or we need to setup a signal
 * at the capsfilter's srcpad for autoplug-continue.
 */
static AnalyzeResult add_capsfilter_for_parser_converter(GstAVSpliterChain *chain, AnalyzeParams *params)
{
    if (!params->isParserConverter) {
        return ANALYZE_CONTINUE;
    }

    g_return_val_if_fail(chain->elements != nullptr, ANALYZE_UNKNOWN_TYPE);
    GstAVSpliterElement *avspliterElem = GST_AVSPLITER_ELEMENT(chain->elements->data);

    GstCaps *filteredCaps = filter_caps_for_capsfilter(params);
    g_return_val_if_fail(filteredCaps != nullptr, ANALYZE_UNKNOWN_TYPE);

    avspliterElem->capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    if (avspliterElem->capsfilter == nullptr) {
        gst_caps_unref(filteredCaps);
        GST_WARNING_OBJECT(chain->avspliter, "can not create capsfilter");
        return ANALYZE_UNKNOWN_TYPE;
    }

    g_object_set(G_OBJECT(avspliterElem->capsfilter), "caps", filteredCaps, nullptr);
    gst_caps_unref(filteredCaps);
    filteredCaps = nullptr;
    (void)gst_element_set_state(avspliterElem->capsfilter, GST_STATE_PAUSED);
    (void)gst_bin_add(GST_BIN_CAST(chain->avspliter), GST_ELEMENT_CAST(gst_object_ref(avspliterElem->capsfilter)));

    gst_avspliter_pad_set_target(params->avspliterPad, nullptr);
    GstPad *newPad = gst_element_get_static_pad(avspliterElem->capsfilter, "sink");
    if (newPad == nullptr) {
        GST_WARNING_OBJECT(chain->avspliter, "can not get capsfilter's sinkpad");
        return ANALYZE_UNKNOWN_TYPE;
    }

    (void)gst_pad_link_full(params->pad, newPad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref(newPad);
    newPad = gst_element_get_static_pad(avspliterElem->capsfilter, "src");
    if (newPad == nullptr) {
        GST_WARNING_OBJECT(chain->avspliter, "can not get capsfilter's srcpad");
        return ANALYZE_UNKNOWN_TYPE;
    }

    gst_avspliter_pad_set_target(params->avspliterPad, newPad);
    params->pad = newPad;

    params->caps = gst_pad_get_current_caps(params->pad);
    if (params->caps == nullptr) {
        GST_DEBUG_OBJECT(chain->avspliter, "No final caps set yet, delaying autopluggging");
        return ANALYZE_DEFER;
    }

    return ANALYZE_CONTINUE;
}

/**
 * Try to connect the pad to an element created from one of the factories, and recursively.
 */
static gboolean connect_new_pad(GstAVSpliterChain *chain, AnalyzeParams *params)
{
    GST_DEBUG_OBJECT(chain->avspliter, "pad %s:%s , chain: 0x%06" PRIxPTR ", %d factories, caps %" GST_PTR_FORMAT,
        GST_DEBUG_PAD_NAME(params->pad), FAKE_POINTER(chain), g_list_length(params->factories), params->caps);

    ConnectPadParams connParams = { 0 };
    init_connect_pad_param(params, &connParams);
    gboolean ret = FALSE;

    for (GList *node = params->factories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST(node->data);
        CHECK_AND_CONTINUE(factory != nullptr);

        GST_LOG_OBJECT(connParams.element, "try factory %" GST_PTR_FORMAT, factory);

        /* Set avspliterpad target to pad again, it might have been unset below but we came back
           here because something failed */
        gst_avspliter_pad_set_target(connParams.avspliterPad, connParams.srcpad);

        ret = check_caps_is_factory_subset(connParams.element, connParams.srccaps, factory);
        CHECK_AND_CONTINUE(ret);

        if (gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER)) {
            mark_end_pad(chain, connParams.srccaps, connParams.srcpad, connParams.avspliterPad);
            ret = TRUE;
            break;
        }

        ret = check_parser_repeat_in_chain(factory, chain, &connParams);
        CHECK_AND_CONTINUE(!ret);

        gst_avspliter_pad_set_target(connParams.avspliterPad, nullptr);

        ret = try_add_new_element(chain, &connParams, factory);
        CHECK_AND_CONTINUE(ret);

        collect_new_element_srcpads(chain, &connParams);
        if (connParams.isParserConverter || connParams.isSimpleDemuxer) {
            connect_new_element_srcpads(chain, &connParams);
        }

        ret = change_new_element_to_pause(chain, &connParams);
        CHECK_AND_CONTINUE(ret);

        connect_new_element_srcpads(chain, &connParams);
        ret = TRUE;
        break;
    }

    if (connParams.errorDetails != nullptr) {
        gboolean freeSegment = (connParams.errorDetails->len == 0 || ret);
        params->deadDetails = g_string_free(connParams.errorDetails, freeSegment);
    } else {
        params->deadDetails = nullptr;
    }
    return ret;
}

static void process_unknown_type(GstAVSpliterChain *chain, AnalyzeParams *params)
{
    GST_LOG_OBJECT(chain->avspliter, "Unknown type, posting message");

    if (chain == nullptr) {
        GST_ELEMENT_ERROR(GST_ELEMENT_CAST(chain->avspliter),
            CORE, FAILED, ("avspliterbin's chain corrupted"), (""));
        return;
    }

    chain->deadDetails = params->deadDetails;
    chain->isDead = TRUE;
    chain->endCaps = (params->caps != nullptr) ? gst_caps_ref(params->caps) : nullptr;
    gst_object_replace(reinterpret_cast<GstObject **>(&chain->currentPad), nullptr);
    gst_element_post_message(GST_ELEMENT_CAST(chain->avspliter),
        gst_missing_decoder_message_new(GST_ELEMENT_CAST(chain->avspliter), params->caps));

    EXPOSE_LOCK(chain->avspliter);
    if (gst_avspliter_chain_is_complete(chain->avspliter->chain)) {
        gst_avspliter_bin_expose(chain->avspliter);
    }
    EXPOSE_UNLOCK(chain->avspliter);

    if (params->src == chain->avspliter->typefind) {
        if (params->caps == nullptr || gst_caps_is_empty(params->caps)) {
            GST_ELEMENT_ERROR(chain->avspliter, STREAM, TYPE_NOT_FOUND,
                ("Could not determine type of stream"), (nullptr));
        }
    }
}

static GstPendingPad *pending_pad_new(GstAVSpliterChain *chain, GstPad *pad)
{
    GstPendingPad *pendingPad = g_slice_new0(GstPendingPad);
    if (pendingPad == nullptr) {
        GST_ERROR_OBJECT(chain->avspliter, "create pending pad failed");
        return nullptr;
    }

    pendingPad->pad = GST_PAD_CAST(gst_object_ref(pad));
    pendingPad->chain = chain;
    pendingPad->eventProbeId = gst_pad_add_probe(pad,
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, pending_pad_event_cb, pendingPad, nullptr);

    pendingPad->notifyCapsSignalId = g_signal_connect(
        pad, "notify::caps", G_CALLBACK(pending_pad_caps_notify_cb), chain);
    if (pendingPad->notifyCapsSignalId == 0) {
        pending_pad_free(pendingPad);
        GST_ERROR_OBJECT(chain->avspliter, "setup notify::caps failed");
        return nullptr;
    }

    return pendingPad;
}

static gboolean defer_caps_setup(GstAVSpliterChain *chain, AnalyzeParams *params)
{
    CHAIN_LOCK(chain);

    GST_LOG_OBJECT(chain->avspliter, "chain 0x%06" PRIxPTR " now has %d dynamic pads",
        FAKE_POINTER(chain), g_list_length(chain->pendingPads));

    GstPendingPad *pendingPad = pending_pad_new(chain, params->pad);
    if (pendingPad == nullptr) {
        CHAIN_UNLOCK(chain);
        return FALSE;
    }

    chain->pendingPads = g_list_prepend(chain->pendingPads, pendingPad);
    CHAIN_UNLOCK(chain);
    return TRUE;
}

static void mark_end_pad(GstAVSpliterChain *chain, GstCaps *caps, GstPad *pad, GstAVSpliterPad *dpad)
{
    GST_DEBUG_OBJECT(chain->avspliter, "pad %s:%s, chain:0x%06" PRIxPTR,
        GST_DEBUG_PAD_NAME(pad), FAKE_POINTER(chain));

    gst_avspliter_pad_active(dpad, chain);
    chain->endPad = GST_AVSPLITER_PAD_CAST(gst_object_ref(dpad));
    if (caps != nullptr) {
        chain->endCaps = gst_caps_ref(caps);
    } else {
        chain->endCaps = nullptr;
    }
}

static void process_analyze_result(GstAVSpliterChain *chain, AnalyzeParams *params, AnalyzeResult result)
{
    if (result == ANALYZE_EXPOSE) {
        mark_end_pad(chain, params->caps, params->pad, params->avspliterPad);
    }

    if (result == ANALYZE_DEFER) {
        if (!defer_caps_setup(chain, params)) {
            result = ANALYZE_UNKNOWN_TYPE;
        }
    }

    if (result == ANALYZE_UNKNOWN_TYPE) {
        process_unknown_type(chain, params);
    }

    if (params->avspliterPad != nullptr) {
        gst_object_unref(params->avspliterPad);
        params->avspliterPad = nullptr;
    }

    // for parser converter, we will change the pad to capsfilter' srcpad.
    if (params->isParserConverter) {
        gst_object_unref(params->pad);
    }

    if (params->factories != nullptr) {
        gst_plugin_feature_list_free(params->factories);
        params->factories = nullptr;
    }

    if (result != ANALYZE_UNKNOWN_TYPE) {
        g_free(params->deadDetails);
        params->deadDetails = nullptr;
    }
}

static void init_connect_pad_param(AnalyzeParams *params, ConnectPadParams *connParams)
{
    connParams->element = params->src;
    connParams->srccaps = params->caps;
    connParams->srcpad = params->pad;
    connParams->avspliterPad = params->avspliterPad;
    connParams->avspliterElem = nullptr;
    connParams->errorDetails = g_string_new("");
    connParams->isParserConverter = FALSE;
    connParams->isSimpleDemuxer = FALSE;
    connParams->nextElem = nullptr;
    connParams->sinkpad = nullptr;
    connParams->addToBin = FALSE;
    connParams->nextSrcPads = nullptr;
}

static gboolean check_parser_repeat_in_chain(GstElementFactory *factory,
    GstAVSpliterChain *chain, ConnectPadParams *params)
{
    /**
     * If the parser was used already, we would create an infinite loop here because the
     * parser apparently accepts its own output as input.
     */

    params->isParserConverter = (strstr(gst_element_factory_get_metadata(
        factory, GST_ELEMENT_METADATA_KLASS), "Parser") != nullptr);
    params->isSimpleDemuxer = check_is_simple_demuxer_factory(factory);

    if (!params->isParserConverter) {
        return FALSE;
    }

    CHAIN_LOCK(chain);

    for (GList *node = chain->elements; node != nullptr; node = node->next) {
        GstAVSpliterElement *avspliterElem = GST_AVSPLITER_ELEMENT(node->data);
        GstElement *upstreamElem = avspliterElem->element;

        if (gst_element_get_factory(upstreamElem) == factory) {
            GST_DEBUG_OBJECT(chain->avspliter,
                "Skipping factory '%s' because it was already used in this chain", FACTORY_NAME(factory));
            CHAIN_UNLOCK(chain);
            return TRUE;
        }
    }

    if (chain->parentChain != nullptr && chain->parentChain->elements != nullptr) {
        GstAVSpliterElement *avspliterElem = GST_AVSPLITER_ELEMENT(chain->parentChain->elements->data);
        if (gst_element_get_factory(avspliterElem->element) == factory) {
            GST_DEBUG_OBJECT(chain->avspliter,
                "Skipping factory '%s' because it was already used in this chain", FACTORY_NAME(factory));
            CHAIN_UNLOCK(chain);
            return TRUE;
        }
    }

    CHAIN_UNLOCK(chain);
    return FALSE;
}

static gboolean add_new_demux_element(GstAVSpliterChain *chain, ConnectPadParams *params)
{
    GstAVSpliterElement *avspliterElem = g_slice_new0(GstAVSpliterElement);
    if (avspliterElem == nullptr) {
        GST_ERROR_OBJECT(chain->avspliter, "Create GstDemuxElemen failed, no memory");
        return FALSE;
    }

    CHAIN_LOCK(chain);
    (void)gst_object_ref(params->nextElem);
    avspliterElem->element = params->nextElem;
    avspliterElem->capsfilter = nullptr;
    GList *list = g_list_prepend(chain->elements, avspliterElem);
    if (list == nullptr) {
        GST_ERROR_OBJECT(chain->avspliter, "append new demux element failed");
        g_slice_free(GstAVSpliterElement, avspliterElem);
        CHAIN_UNLOCK(chain);
        return FALSE;
    }
    params->avspliterElem = avspliterElem;
    chain->elements = list;
    chain->isDemuxer = check_is_demuxer_element(params->nextElem);
    chain->hasParser |= params->isParserConverter;
    CHAIN_UNLOCK(chain);

    return TRUE;
}

static void cleanup_add_new_element(GstAVSpliterChain *chain, ConnectPadParams *params, gboolean ret)
{
    if (!ret) {
        if (params->nextElem != nullptr) {
            gst_element_set_state(params->nextElem, GST_STATE_NULL);
            if (params->addToBin) {
                gst_bin_remove(GST_BIN_CAST(chain->avspliter), params->nextElem);
            } else {
                gst_object_unref(params->nextElem);
            }
            params->nextElem = nullptr;
        }
    } else {
        GST_LOG_OBJECT(chain->avspliter, "linked on pad %s:%s", GST_DEBUG_PAD_NAME(params->srcpad));
    }

    if (params->sinkpad != nullptr) {
        gst_object_unref(params->sinkpad);
        params->sinkpad = nullptr;
    }
}

/* need remove the demux element from chain's elements by the caller. */
static void free_demux_element(GstAVSpliterChain *chain, GstAVSpliterElement *element)
{
    if (element->padAddedSignalId != 0) {
        g_signal_handler_disconnect(element->element, element->padAddedSignalId);
    }
    if (element->padRemoveSignalId != 0) {
        g_signal_handler_disconnect(element->element, element->padRemoveSignalId);
    }
    if (element->noMorePadsSignalId != 0) {
        g_signal_handler_disconnect(element->element, element->noMorePadsSignalId);
    }

    for (GList *node = chain->pendingPads; node != nullptr;) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(node->data);
        if (GST_PAD_PARENT(ppad->pad) != element->element) {
            node = node->next;
            continue;
        }
        pending_pad_free(ppad);
        GList *next = node->next;
        chain->pendingPads = g_list_delete_link(chain->pendingPads, node);
        node = next;
    }

    if (element->capsfilter != nullptr) {
        gst_bin_remove(GST_BIN(chain->avspliter), element->capsfilter);
        gst_element_set_state(element->capsfilter, GST_STATE_NULL);
        gst_object_unref(element->capsfilter);
        element->capsfilter = nullptr;
    }

    gst_bin_remove(GST_BIN(chain->avspliter), element->element);
    gst_element_set_state(element->element, GST_STATE_NULL);
    gst_object_unref(element->element);

    g_slice_free(GstAVSpliterElement, element);
}

static gboolean try_add_new_element(GstAVSpliterChain *chain, ConnectPadParams *params, GstElementFactory *factory)
{
    gboolean ret = FALSE;

    do {
        params->nextElem = gst_element_factory_create(factory, nullptr);
        if (params->nextElem == nullptr) {
            gst_avspliter_bin_collect_errmsg(chain->avspliter, nullptr, params->errorDetails,
                "Could not create an element from %s", FACTORY_NAME(factory));
            continue;
        }

        /* Filter errors, this will prevent the element from causing the pipeline to error while we
         * just only test this element. */
        gst_avspliter_bin_add_errfilter(chain->avspliter, params->nextElem);

        /* we don't yet want the bin to control the element's state */
        gst_element_set_locked_state(params->nextElem, TRUE);

        if (!gst_bin_add(GST_BIN_CAST(chain->avspliter), params->nextElem)) {
            gst_avspliter_bin_collect_errmsg(chain->avspliter, params->nextElem, params->errorDetails,
                "Couldn't add %s to the bin", GST_ELEMENT_NAME(params->nextElem));
            break;
        }
        params->addToBin = TRUE;

        GST_OBJECT_LOCK(params->nextElem);
        if (params->nextElem->sinkpads != nullptr && params->nextElem->sinkpads->data != nullptr) {
            params->sinkpad = GST_PAD_CAST(gst_object_ref(params->nextElem->sinkpads->data));
        }
        GST_OBJECT_UNLOCK(params->nextElem);

        if (params->sinkpad == nullptr) {
            gst_avspliter_bin_collect_errmsg(chain->avspliter, params->nextElem, params->errorDetails,
                "Element %s doesn't have a sink pad", GST_ELEMENT_NAME(params->nextElem));
            break;
        }

        if (gst_pad_link_full(params->srcpad, params->sinkpad, GST_PAD_LINK_CHECK_NOTHING) != GST_PAD_LINK_OK) {
            gst_avspliter_bin_collect_errmsg(chain->avspliter, params->nextElem, params->errorDetails,
                "Link failed on pad %s:%s", GST_DEBUG_PAD_NAME(params->sinkpad));
            break;
        }

        if (gst_element_set_state(params->nextElem, GST_STATE_READY) == GST_STATE_CHANGE_FAILURE) {
            gst_avspliter_bin_collect_errmsg(chain->avspliter, params->nextElem, params->errorDetails,
                "Couldn't set %s to READY", GST_ELEMENT_NAME(params->nextElem));
            break;
        }

        if (!gst_pad_query_accept_caps(params->sinkpad, params->srccaps)) {
            gst_avspliter_bin_collect_errmsg(chain->avspliter, params->nextElem, params->errorDetails,
                "Element %s does not accept caps", GST_ELEMENT_NAME(params->nextElem));
            break;
        }

        ret = TRUE;
    } while (0);

    ret = ret && add_new_demux_element(chain, params);
    cleanup_add_new_element(chain, params, ret);
    return ret;
}

static void pad_added_cb(GstElement *element, GstPad *pad, GstAVSpliterChain *chain)
{
    if (element == nullptr || pad == nullptr || chain == nullptr) {
        return;
    }

    GST_DEBUG_OBJECT(pad, "pad added, chain: 0x%06" PRIXPTR, FAKE_POINTER(chain));

    // FIXME: for some streams, i.e. ogg, this maybe normal, output log to trace it.
    if (chain->noMorePads) {
        GST_WARNING_OBJECT(chain->avspliter, "chain has already no more pads, but pad_added called");
        return;
    }

    GstCaps *caps = get_pad_caps(pad);
    gst_avspliter_chain_analyze_new_pad(element, pad, caps, chain);
    if (caps != nullptr) {
        gst_caps_unref(caps);
        caps = nullptr;
    }

    EXPOSE_LOCK(chain->avspliter);
    if (gst_avspliter_chain_is_complete(chain->avspliter->chain)) {
        GST_LOG_OBJECT(chain->avspliter,
            "that was the last dynamic pad, now attempting to expose the bin");
        if (!gst_avspliter_bin_expose(chain->avspliter)) {
            GST_WARNING_OBJECT(chain->avspliter, "Couldn't expose bin");
        }
    }
    EXPOSE_UNLOCK(chain->avspliter);
}

static void pad_removed_cb(GstElement *element, GstPad *pad, GstAVSpliterChain *chain)
{
    if (element == nullptr || pad == nullptr || chain == nullptr) {
        return;
    }

    GST_LOG_OBJECT(pad, "pad remove, chain:0x%06" PRIXPTR, FAKE_POINTER(chain));

    CHAIN_LOCK(chain);
    for (GList *node = chain->pendingPads; node != nullptr; node = node->next) {
        GstPendingPad *ppad = reinterpret_cast<GstPendingPad *>(node->data);
        if (ppad->pad == pad) {
            pending_pad_free(ppad);
            ppad = nullptr;
            chain->pendingPads = g_list_delete_link(chain->pendingPads, node);
            break;
        }
    }
    CHAIN_UNLOCK(chain);
}

static void no_more_pads_cb(GstElement *element, GstAVSpliterChain *chain)
{
    if (element == nullptr || chain == nullptr) {
        return;
    }

    GST_LOG_OBJECT(element, "got no more pads");

    CHAIN_LOCK(chain);
    if (chain->elements != nullptr) {
        GstAVSpliterElement *avspliterElem = reinterpret_cast<GstAVSpliterElement *>(chain->elements->data);
        if (avspliterElem->element != element) {
            GST_LOG_OBJECT(chain->avspliter,
                "no-more-pads from chain old element '%s'", GST_OBJECT_NAME(element));
            CHAIN_UNLOCK(chain);
            return;
        }
    } else if (!chain->isDemuxer) {
        GST_LOG_OBJECT(chain->avspliter,
            "no-more-pads from a non-demuxer element '%s'", GST_OBJECT_NAME(element));
        CHAIN_UNLOCK(chain);
        return;
    }

    GST_DEBUG_OBJECT(chain->avspliter, "setting bin to complete");

    chain->noMorePads = TRUE;
    CHAIN_UNLOCK(chain);

    EXPOSE_LOCK(chain->avspliter);
    if (gst_avspliter_chain_is_complete(chain->avspliter->chain)) {
        gst_avspliter_bin_expose(chain->avspliter);
    }
    EXPOSE_UNLOCK(chain->avspliter);

    gst_element_no_more_pads(GST_ELEMENT_CAST(chain->avspliter));
}

static void collect_new_element_srcpads(GstAVSpliterChain *chain, ConnectPadParams *params)
{
    GstElement *element = params->avspliterElem->element;

    GST_DEBUG_OBJECT(chain->avspliter, "Attempting to connect element %s [chain: 0x%06" PRIXPTR "] further",
        GST_ELEMENT_NAME(element), FAKE_POINTER(chain));

    GstElementClass *klass = GST_ELEMENT_GET_CLASS(element);
    g_return_if_fail(klass != nullptr);
    gboolean dynamic = TRUE;

    for (GList *pads = klass->padtemplates; pads != nullptr; pads = g_list_next(pads)) {
        GstPadTemplate *templ = GST_PAD_TEMPLATE(pads->data);
        if (templ == nullptr || GST_PAD_TEMPLATE_DIRECTION(templ) != GST_PAD_SRC) {
            continue;
        }
        const gchar *templName = GST_PAD_TEMPLATE_NAME_TEMPLATE(templ);
        GST_DEBUG_OBJECT(chain->avspliter, "got a source pad template %s", templName);

        if (GST_PAD_TEMPLATE_PRESENCE(templ) == GST_PAD_REQUEST) {
            GST_DEBUG_OBJECT(params->avspliterElem, "ignore request padtemplate %s", templName);
            continue;
        }

        GstPad *pad = gst_element_get_static_pad(element, templName);
        if (pad != nullptr) {
            GST_DEBUG_OBJECT(chain->avspliter, "got the pad for template %s", templName);
            params->nextSrcPads = g_list_prepend(params->nextSrcPads, pad);
            continue;
        }

        if (GST_PAD_TEMPLATE_PRESENCE(templ) == GST_PAD_SOMETIMES) {
            GST_DEBUG_OBJECT(chain->avspliter, "did not get the sometimes pad of template %s", templName);
            dynamic = TRUE;
        } else {
            GST_WARNING_OBJECT(chain->avspliter, "Could not get the pad for always template %s", templName);
        }
    }

    if (dynamic) {
        GST_LOG_OBJECT(chain->avspliter, "Adding signals to element %s in chain 0x%06" PRIXPTR,
            GST_ELEMENT_NAME(element), FAKE_POINTER(chain));
        params->avspliterElem->padAddedSignalId = g_signal_connect(
            element, "pad-added", G_CALLBACK(pad_added_cb), chain);
        params->avspliterElem->padRemoveSignalId = g_signal_connect(
            element, "pad-removed", G_CALLBACK(pad_removed_cb), chain);
        params->avspliterElem->noMorePadsSignalId = g_signal_connect(
            element, "no-more-pads", G_CALLBACK(no_more_pads_cb), chain);
    }
}

static void connect_new_element_srcpads(GstAVSpliterChain *chain, ConnectPadParams *params)
{
    for (GList *node = params->nextSrcPads; node != nullptr; node = g_list_next(node)) {
        GstPad *srcpad = GST_PAD_CAST(node->data);
        GstCaps *srccaps = get_pad_caps(srcpad);
        gst_avspliter_chain_analyze_new_pad(params->avspliterElem->element, srcpad, srccaps, chain);
        if (srccaps != nullptr) {
            gst_caps_unref(srccaps);
            srccaps = nullptr;
        }
        gst_object_unref(srcpad);
    }

    g_list_free(params->nextSrcPads);
    params->nextSrcPads = nullptr;
}

static gboolean send_sticky_event(GstPad *pad, GstEvent **event, gpointer userdata)
{
    if (pad == nullptr || userdata == nullptr || event == nullptr) {
        return FALSE;
    }

    GstPad *targetPad = GST_PAD_CAST(userdata);
    gboolean ret = gst_pad_send_event(targetPad, gst_event_ref(*event));
    if (!ret) {
        GST_WARNING_OBJECT(targetPad,
            "Send sticky event %" GST_PTR_FORMAT " failed, event from %" GST_PTR_FORMAT, *event, pad);
    }
    return ret;
}

static void send_sticky_events(GstPad *pad)
{
    GstPad *peer = gst_pad_get_peer(pad);
    gst_pad_sticky_events_foreach(pad, send_sticky_event, peer);
    gst_object_unref(peer);
}

static gboolean change_new_element_to_pause(GstAVSpliterChain *chain, ConnectPadParams *params)
{
    /* lock element's sinkpad stream lock so no data reaches the possible new element added when caps
       are sent by current element while we're still sending sticky events */

    GST_PAD_STREAM_LOCK(params->sinkpad);

    if (gst_element_set_state(params->nextElem, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE) {
        send_sticky_events(params->srcpad);
        GST_PAD_STREAM_UNLOCK(params->sinkpad);

        gst_avspliter_bin_remove_errfilter(chain->avspliter, params->nextElem, nullptr);
        gst_element_set_locked_state(params->nextElem, FALSE);
        return TRUE;
    }

    GST_PAD_STREAM_UNLOCK(params->sinkpad);

    gst_avspliter_bin_collect_errmsg(chain->avspliter, params->nextElem, params->errorDetails,
        "Couldn't set %s to PAUSED", GST_ELEMENT_NAME(params->nextElem));
    g_list_free_full(params->nextSrcPads, (GDestroyNotify)gst_object_unref);
    params->nextSrcPads = nullptr;

    // remove all elements in this chain until current new element.
    CHAIN_LOCK(chain);

    GstElement *tmpElem = nullptr;
    do {
        if (chain->elements == nullptr) {
            break;
        }

        GstAVSpliterElement *tmpAVSpliterElem = GST_AVSPLITER_ELEMENT(chain->elements->data);
        tmpElem = tmpAVSpliterElem->element;
        free_demux_element(chain, tmpAVSpliterElem);
        chain->elements = g_list_delete_link(chain->elements, chain->elements);
    } while (tmpElem != params->nextElem);

    CHAIN_UNLOCK(chain);
    return FALSE;
}
