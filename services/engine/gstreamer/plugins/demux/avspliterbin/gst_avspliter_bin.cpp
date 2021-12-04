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
#include "gst_avspliter_bin.h"
#include <stdarg.h>
#include "gst/pbutils/missing-plugins.h"
#include "gst/pbutils/descriptions.h"
#include "gst/playback/gstrawcaps.h"
#include "gst/playback/gstplaybackutils.h"
#include "gst_avspliter_utils.h"
#include "gst_avspliter_pad.h"
#include "gst_avspliter_chain.h"
#include "gst_avspliter_bin_priv.h"

/* generic templates */
static GstStaticPadTemplate g_avspliterSinkTemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate g_avspliterSrcTemplate = GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

enum {
    SIGNAL_HAVE_TYPE,
    SIGNAL_AUTOPLUG_CONTINUE,
    LAST_SIGNAL
};

enum {
    PROP_0,
};

GST_DEBUG_CATEGORY_STATIC (gst_avspliter_bin_debug);
#define GST_CAT_DEFAULT gst_avspliter_bin_debug

static guint gst_avspliter_bin_signals[LAST_SIGNAL] = { 0 };

static void gst_avspliter_bin_dispose(GObject *object);
static void gst_avspliter_bin_finalize(GObject *object);
static void gst_avspliter_bin_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec);
static void gst_avspliter_bin_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_avspliter_bin_change_state(GstElement *element, GstStateChange transition);
static void gst_avspliter_bin_handle_message(GstBin *bin, GstMessage *message);
static void found_type_callback(GstElement *typefind, guint probability, GstCaps *caps, GstAVSpliterBin *avspliter);
static gboolean gst_avspliter_bin_autoplug_continue(GstElement *element, GstPad *pad, GstCaps *caps);
static void unblock_pads(GstAVSpliterBin *avspliter);
static GList *try_collect_all_endpads(GstAVSpliterBin *avspliter);
static gboolean check_already_exposed_pads(GstAVSpliterBin *avspliter, GList *endpads);
static void expose_end_pads_to_app(GstAVSpliterBin *avspliter, GList *endpads);
static void build_typefind(GstAVSpliterBin *avspliter);

#define gst_avspliter_bin_parent_class parent_class
G_DEFINE_TYPE(GstAVSpliterBin, gst_avspliter_bin, GST_TYPE_BIN);

static gboolean gst_avspliter_bin_boolean_accumulator(
    GSignalInvocationHint *iHint, GValue *returnAccu, const GValue *handlerReturn, gpointer dummy)
{
    (void)iHint;
    (void)dummy;
    gboolean myboolean = g_value_get_boolean(handlerReturn);
    g_value_set_boolean(returnAccu, myboolean);
    return myboolean;
}

static void gst_avspliter_bin_class_init(GstAVSpliterBinClass *klass)
{
    GObjectClass *gobjectClass = G_OBJECT_CLASS(klass);
    g_return_if_fail(gobjectClass != nullptr);
    GstElementClass *elementClass = GST_ELEMENT_CLASS(klass);
    g_return_if_fail(elementClass != nullptr);
    GstBinClass *binClass = GST_BIN_CLASS(klass);
    g_return_if_fail(binClass != nullptr);

    gobjectClass->dispose = gst_avspliter_bin_dispose;
    gobjectClass->finalize = gst_avspliter_bin_finalize;
    gobjectClass->set_property = gst_avspliter_bin_set_property;
    gobjectClass->get_property = gst_avspliter_bin_get_property;

    /**
     * @brief This signal is emitted whenever AVSpliterBin finds a new stream. It is emitted before looking for any
     * elements that can handle that stream.
     *
     * Invocation of signal handlers stops after the first signal handler return FALSE. Signal handlers are
     * invoked in the order they were connected in. If no handler connected, defaultly TRUE.
     *
     * Returns: TRUE if you wish AVSpliterBin to look for elements that can handle the given caps. If FALSE, those
     * caps will be considered as final and the pad will be exposed as such (see 'pad-added' signal of GstElement).
     */
    gst_avspliter_bin_signals[SIGNAL_AUTOPLUG_CONTINUE] = g_signal_new("autoplug-continue",
        G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(GstAVSpliterBinClass, autoplug_continue),
        gst_avspliter_bin_boolean_accumulator, nullptr, g_cclosure_marshal_generic,
        G_TYPE_BOOLEAN, 2, GST_TYPE_PAD, GST_TYPE_CAPS);

    gst_avspliter_bin_signals[SIGNAL_HAVE_TYPE] = g_signal_new("have-type",
        G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(GstAVSpliterBinClass, have_type),
        nullptr, nullptr, g_cclosure_marshal_generic, G_TYPE_NONE, 2,
        G_TYPE_UINT, GST_TYPE_CAPS | G_SIGNAL_TYPE_STATIC_SCOPE);

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&g_avspliterSinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&g_avspliterSrcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "AVSpliter Bin", "Generic/Bin/Parser/Demuxer",
        "De-multiplex and parse to elementary stream",
        "OpenHarmony");

    elementClass->change_state = GST_DEBUG_FUNCPTR(gst_avspliter_bin_change_state);
    binClass->handle_message = GST_DEBUG_FUNCPTR(gst_avspliter_bin_handle_message);
    klass->autoplug_continue = gst_avspliter_bin_autoplug_continue;

    g_type_class_ref(GST_TYPE_AVSPLITER_PAD); // TODO: why?
}

static void gst_avspliter_bin_init(GstAVSpliterBin *avspliter)
{
    g_return_if_fail(avspliter != nullptr);

    g_mutex_init(&avspliter->factoriesLock);
    g_mutex_init(&avspliter->exposeLock);
    g_mutex_init(&avspliter->dynlock);
    avspliter->factories = nullptr;
    avspliter->factoriesCookie = 0;
    avspliter->shutDown = FALSE;
    avspliter->blockedPads = nullptr;
    avspliter->chain = nullptr;
    avspliter->filtered = nullptr;
    avspliter->filteredErrors = nullptr;
    avspliter->haveType = FALSE;
    avspliter->haveTypeSignalId = 0;
    avspliter->npads = 0;
    avspliter->srcTmpl = &g_avspliterSrcTemplate;
    build_typefind(avspliter);
}

static void gst_avspliter_bin_dispose(GObject *object)
{
    GstAVSpliterBin *avspliter = GST_AVSPLITER_BIN(object);
    g_return_if_fail(avspliter != nullptr);

    if (avspliter->factories != nullptr) {
        gst_plugin_feature_list_free(avspliter->factories);
        avspliter->factories = nullptr;
    }

    if (avspliter->chain != nullptr) {
        gst_avspliter_chain_free(avspliter->chain);
        avspliter->chain = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_avspliter_bin_finalize(GObject *object)
{
    GstAVSpliterBin *avspliter = GST_AVSPLITER_BIN(object);
    g_return_if_fail(avspliter != nullptr);

    g_mutex_clear(&avspliter->exposeLock);
    g_mutex_clear(&avspliter->dynlock);
    g_mutex_clear(&avspliter->factoriesLock);

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_avspliter_bin_set_property(GObject *object, guint propId, const GValue *value, GParamSpec *pspec)
{
    (void)value;

    switch(propId) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static void gst_avspliter_bin_get_property(GObject *object, guint propId, GValue *value, GParamSpec *pspec)
{
    (void)value;

    switch (propId) {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
            break;
    }
}

static gboolean gst_avspliter_bin_autoplug_continue(GstElement *element, GstPad *pad, GstCaps *caps)
{
    static GstStaticCaps rawcaps = GST_STATIC_CAPS(DEFAULT_RAW_CAPS);

    GST_DEBUG_OBJECT(element, "caps %" GST_PTR_FORMAT, caps);

    /* If it matches our target caps, expose it */
    if (gst_caps_can_intersect(caps, gst_static_caps_get(&rawcaps))) {
        GST_DEBUG_OBJECT(element, "autoplug-continue returns FALSE");
        return FALSE;
    }

    GST_DEBUG_OBJECT(element, "autoplug-continue returns TRUE");
    return TRUE;
}

static gboolean gst_avspliter_bin_ready_to_pause(GstAVSpliterBin *avspliter)
{
    EXPOSE_LOCK(avspliter);
    if (avspliter->chain != nullptr) {
        gst_avspliter_chain_free(avspliter->chain);
        avspliter->chain = nullptr;
    }
    EXPOSE_UNLOCK(avspliter);

    DYN_LOCK(avspliter);
    GST_LOG_OBJECT(avspliter, "clearing shutdown flag");
    avspliter->shutDown = FALSE;
    DYN_UNLOCK(avspliter);
    avspliter->haveType = FALSE;

    avspliter->haveTypeSignalId =
        g_signal_connect(avspliter->typefind, "have-type", G_CALLBACK(found_type_callback), avspliter);
    if (avspliter->haveTypeSignalId == 0) {
        GST_ELEMENT_ERROR(avspliter, CORE, FAILED, (nullptr), ("have-type signal connect failed!"));
        return FALSE;
    }

    return TRUE;
}

static GstStateChangeReturn gst_avspliter_bin_change_state(GstElement *element, GstStateChange transition)
{
    GstAVSpliterBin *avspliter = GST_AVSPLITER_BIN(element);
    g_return_val_if_fail(avspliter != nullptr, GST_STATE_CHANGE_FAILURE);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (avspliter->typefind == nullptr) {
                gst_element_post_message(element, gst_missing_element_message_new(element, "typefind"));
                GST_ELEMENT_ERROR(avspliter, CORE, MISSING_PLUGIN, (nullptr), ("no typefind!"));
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            if (!gst_avspliter_bin_ready_to_pause(avspliter)) {
                return GST_STATE_CHANGE_FAILURE;
            }
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            if (avspliter->haveTypeSignalId != 0) {
                g_signal_handler_disconnect(avspliter->typefind, avspliter->haveTypeSignalId);
                avspliter->haveTypeSignalId = 0;
            }
            DYN_LOCK(avspliter);
            GST_LOG_OBJECT(avspliter, "setting shutdown flag");
            avspliter->shutDown = TRUE;
            unblock_pads(avspliter);
            DYN_UNLOCK(avspliter);
            break;
        default:
            break;
    }

    GstStateChangeReturn ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT(avspliter, "element failed to change states");
        return GST_STATE_CHANGE_FAILURE;
    }

    switch (transition) {
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            EXPOSE_LOCK(avspliter);
            if (avspliter->chain != nullptr) {
                gst_avspliter_chain_free(avspliter->chain); // maybe time-consuming, considering start a thread
                avspliter->chain = nullptr;
            }
            EXPOSE_UNLOCK(avspliter);
            break;
        default:
            break;
    }

    return ret;
}

static void gst_avspliter_bin_handle_message(GstBin *bin, GstMessage *msg)
{
    GstAVSpliterBin *avspliter = GST_AVSPLITER_BIN(bin);
    g_return_if_fail(avspliter != nullptr && msg != nullptr);

    gboolean drop = FALSE;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            DYN_LOCK(avspliter);
            drop = avspliter->shutDown;
            DYN_UNLOCK(avspliter);

            if (!drop) {
                GST_OBJECT_LOCK(avspliter);
                drop = (g_list_find(avspliter->filtered, GST_MESSAGE_SRC(msg)) != nullptr);
                if (drop) {
                    avspliter->filteredErrors = g_list_prepend(avspliter->filteredErrors, gst_message_ref(msg));
                }
                GST_OBJECT_UNLOCK(avspliter);
            }
            break;
        }
        default:
            break;
    }

    if (drop) {
        gst_message_unref(msg);
    } else {
        GST_BIN_CLASS(parent_class)->handle_message(bin, msg);
    }
}

static void build_typefind(GstAVSpliterBin *avspliter)
{
    gboolean connectSrc = FALSE;
    GstPad *pad = nullptr;
    GstPadTemplate *padTmpl = nullptr;
    GstPad *ghostPad = nullptr;

    do {
        avspliter->typefind = gst_element_factory_make("typefind", "typefind");
        CHECK_AND_BREAK_LOG(avspliter, avspliter->typefind != nullptr, "can't find typefind elemen");

        gboolean ret = gst_bin_add(GST_BIN(avspliter), avspliter->typefind);
        CHECK_AND_BREAK_LOG(avspliter, ret, "can not add typefind element");

        pad = gst_element_get_static_pad(avspliter->typefind, "sink");
        CHECK_AND_BREAK_LOG(avspliter, pad != nullptr, "can not get sink pad from typefind");

        padTmpl = gst_static_pad_template_get(&g_avspliterSinkTemplate);
        CHECK_AND_BREAK_LOG(avspliter, padTmpl != nullptr, "can not get sink pad template");

        ghostPad = gst_ghost_pad_new_from_template("sink", pad, padTmpl);
        CHECK_AND_BREAK_LOG(avspliter, ghostPad != nullptr, "create ghost pad failed");
        (void)gst_pad_set_active(ghostPad, TRUE);
        (void)gst_element_add_pad(GST_ELEMENT(avspliter), ghostPad);

        connectSrc = TRUE;
    } while (0);

    if (!connectSrc) {
        if (avspliter->typefind != nullptr) {
            gst_object_unref(avspliter->typefind);
            avspliter->typefind = nullptr;
        }
    }

    if (pad != nullptr) {
        gst_object_unref(pad);
        pad = nullptr;
    }

    if (padTmpl != nullptr) {
        gst_object_unref(padTmpl);
    }
}

static void found_type_callback(GstElement *typefind, guint probability, GstCaps *caps, GstAVSpliterBin *avspliter)
{
    GST_DEBUG_OBJECT(avspliter, "typefind found caps %" GST_PTR_FORMAT, caps);
    g_return_if_fail(typefind != nullptr && caps != nullptr && avspliter != nullptr);

    if (gst_structure_has_name(gst_caps_get_structure(caps, 0), "text/plain")) {
        GST_ELEMENT_ERROR(avspliter, STREAM, WRONG_TYPE, ("This appears to be a text file"),
                          ("AVSpliterBin can not process plain text failes"));
        return;
    }
    // not yet support dynamically chaning caps from the typefind element.
    if (avspliter->haveType || avspliter->chain != nullptr) {
        return;
    }
    avspliter->haveType = TRUE;

    g_signal_emit(avspliter, gst_avspliter_bin_signals[SIGNAL_HAVE_TYPE], 0, probability, caps);

    GstPad *srcpad = gst_element_get_static_pad(typefind, "src");
    GstPad *sinkpad = gst_element_get_static_pad(typefind, "sink");

    if (srcpad == nullptr || sinkpad == nullptr) {
        gst_object_unref(srcpad);
        gst_object_unref(sinkpad);
    }

    /**
     * need stream lock here to prevent race with shutdown state change which might yank
     * away e.g. avspliter chain while building stuff here. In the GstElement, change state
     * from pause to ready will firstly deactive all pads, which need this stream lock.
     * In most cases, the stream lock has already held, but we grab it anyway.
     */
    GST_PAD_STREAM_LOCK(sinkpad);
    avspliter->chain = gst_avspliter_chain_new(avspliter, nullptr, srcpad, caps);
    if (avspliter->chain == nullptr) {
        GST_ELEMENT_ERROR(avspliter, RESOURCE, FAILED, ("create chain failed when have type"), (""));
        GST_PAD_STREAM_UNLOCK(sinkpad);
        gst_object_unref(srcpad);
        gst_object_unref(sinkpad);
        return;
    }

    gst_avspliter_chain_analyze_new_pad(typefind, srcpad, caps, avspliter->chain);
    GST_PAD_STREAM_UNLOCK(sinkpad);
    gst_object_unref(srcpad);
    gst_object_unref(sinkpad);
}

/* Must only be called if the toplevel chain is complete and blocked, called with expose block */
gboolean gst_avspliter_bin_expose(GstAVSpliterBin *avspliter)
{
    GST_DEBUG_OBJECT(avspliter, "Exposing avspliter bin");

    DYN_LOCK(avspliter);
    if (avspliter->shutDown) {
        GST_WARNING_OBJECT(avspliter, "shutting down, aborting exposing.");
        DYN_UNLOCK(avspliter);
        return FALSE;
    }
    DYN_UNLOCK(avspliter);

    GList *endpads = try_collect_all_endpads(avspliter);
    if (endpads == nullptr) {
        return FALSE;
    }

    if (check_already_exposed_pads(avspliter, endpads)) {
        return TRUE;
    }

    endpads = g_list_sort(endpads, (GCompareFunc)sort_pads);

    DYN_LOCK(avspliter);
    if (avspliter->shutDown) {
        GST_WARNING_OBJECT(avspliter, "shutting down, aborting exposing.");
        DYN_UNLOCK(avspliter);
        return FALSE;
    }

    expose_end_pads_to_app(avspliter, endpads);
    DYN_UNLOCK(avspliter);

    g_list_free(endpads);
    endpads = nullptr;
    gst_element_no_more_pads(GST_ELEMENT_CAST(avspliter));

    GST_DEBUG_OBJECT(avspliter, "Exposed everything");
    return TRUE;
}

void gst_avspliter_bin_add_errfilter(GstAVSpliterBin *avspliter, GstElement *element)
{
    GST_OBJECT_LOCK(avspliter);
    avspliter->filtered = g_list_prepend(avspliter->filtered, element);
    GST_OBJECT_UNLOCK(avspliter);
}

void gst_avspliter_bin_remove_errfilter(GstAVSpliterBin *avspliter, GstElement *element, GstMessage **error)
{
    GST_OBJECT_LOCK(avspliter);
    avspliter->filtered = g_list_remove(avspliter->filtered, element);
    if (error != nullptr) {
        *error = nullptr;
    }
    GList *node = avspliter->filteredErrors;
    while (node != nullptr) {
        GstMessage *msg = GST_MESSAGE_CAST(node->data);
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT_CAST(element)) {
            if (error != nullptr) {
                gst_message_replace(error, msg);
            }
            gst_message_unref(msg);
            avspliter->filteredErrors = g_list_delete_link(avspliter->filteredErrors, node);
            node = avspliter->filteredErrors;
        } else {
            node = node->next;
        }
    }
    GST_OBJECT_UNLOCK(avspliter);
}

void gst_avspliter_bin_collect_errmsg(GstAVSpliterBin *avspliter, GstElement *element,
    GString *errorDetails, const gchar *format, ...)
{
    va_list args;
    va_start (args, format);
    GST_WARNING_OBJECT(avspliter, format, args);
    g_string_append_printf(errorDetails, format, args);
    va_end(args);

    if (element == nullptr) {
        return;
    }

    GstMessage *errorMsg = nullptr;
    gst_avspliter_bin_remove_errfilter(avspliter, element, &errorMsg);
    if (errorMsg != nullptr) {
        gchar *errorStr = error_message_to_string(errorMsg);
        if (errorStr != nullptr) {
            g_string_append_printf(errorDetails, "\n%s", errorStr);
            g_free(errorStr);
        }
        gst_message_unref(errorMsg);
    }
}

gboolean gst_avspliter_bin_emit_appcontinue(GstAVSpliterBin *avspliter, GstPad *pad, GstCaps *caps)
{
    gboolean appcontinue = TRUE;

    g_signal_emit(G_OBJECT(avspliter), gst_avspliter_bin_signals[SIGNAL_AUTOPLUG_CONTINUE],
            0, pad, caps, &appcontinue);

    return appcontinue;
}

GList *gst_avspliter_bin_find_elem_factories(GstAVSpliterBin *avspliter, GstCaps *caps)
{
    GST_DEBUG_OBJECT(avspliter, "Finding factories");
    g_mutex_lock(&avspliter->factoriesLock);

    guint cookie = gst_registry_get_feature_list_cookie(gst_registry_get());
    if (avspliter->factories == nullptr || avspliter->factoriesCookie != cookie) {
        if (avspliter->factories != nullptr) {
            gst_plugin_feature_list_free(avspliter->factories);
        }
        avspliter->factories = gst_element_factory_list_get_elements(
            GST_ELEMENT_FACTORY_TYPE_DECODABLE, GST_RANK_MARGINAL);
        avspliter->factories = g_list_sort(avspliter->factories, gst_playback_utils_compare_factories_func);
        avspliter->factoriesCookie = cookie;
    }
    GList *list = gst_element_factory_list_filter(avspliter->factories,
        caps, GST_PAD_SINK, gst_caps_is_fixed(caps));

    g_mutex_unlock(&avspliter->factoriesLock);
    GST_DEBUG_OBJECT(avspliter, "find factories finish");
    return list;
}

/* Must be called with expose lock. */
static GList *try_collect_all_endpads(GstAVSpliterBin *avspliter)
{
    GList *endpads = nullptr;
    gboolean missingPlugin = FALSE;
    GString *missingPluginDetails = g_string_new("");

    CHAIN_LOCK(avspliter->chain);
    if (!gst_avspliter_chain_expose(avspliter->chain, &endpads, &missingPlugin, missingPluginDetails)) {
        g_list_free_full(endpads, (GDestroyNotify)gst_object_unref);
        g_string_free(missingPluginDetails, TRUE);
        GST_ERROR_OBJECT(avspliter, "Broken chain tree");
        CHAIN_UNLOCK(avspliter->chain);
        return nullptr;
    }
    CHAIN_UNLOCK(avspliter->chain);

    if (endpads != nullptr) {
        g_string_free(missingPluginDetails, TRUE);
        return endpads;
    }

    if (missingPlugin) {
        if (missingPluginDetails->len > 0) {
            gchar *details = g_string_free(missingPluginDetails, FALSE);
            GST_ELEMENT_ERROR(avspliter, CORE, MISSING_PLUGIN, (nullptr),
                ("no suitable plugins found:\n%s", details));
            g_free(details);
        } else {
            g_string_free(missingPluginDetails, TRUE);
            GST_ELEMENT_ERROR(avspliter, CORE, MISSING_PLUGIN, (nullptr), ("no suitable plugins found"));
        }
    } else {
        g_string_free(missingPluginDetails, TRUE);
        GST_WARNING_OBJECT(avspliter, "All streams finished without buffers. ");
        GST_ELEMENT_ERROR(avspliter, STREAM, FAILED, (nullptr), ("all streams without buffers."));
    }
    return nullptr;
}

static gboolean check_already_exposed_pads(GstAVSpliterBin *avspliter, GList *endpads)
{
    gboolean allExposed = FALSE;
    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstAVSpliterPad *avspliterPad = reinterpret_cast<GstAVSpliterPad *>(tmp->data);
        allExposed &= avspliterPad->exposed;
    }

    if (allExposed) {
        GST_INFO_OBJECT(avspliter, "Everything was exposed already !");
        g_list_free_full(endpads, (GDestroyNotify)gst_object_unref);
        return TRUE;
    }

    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstAVSpliterPad *avspliterPad = reinterpret_cast<GstAVSpliterPad *>(tmp->data);
        if (avspliterPad->exposed) {
            GST_DEBUG_OBJECT(avspliterPad, "blocking exposed pad");
            gst_avspliter_pad_set_block(avspliterPad, TRUE);
        }
    }

    return FALSE;
}

static void expose_end_pads_to_app(GstAVSpliterBin *avspliter, GList *endpads)
{
    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstAVSpliterPad *avspliterPad = reinterpret_cast<GstAVSpliterPad *>(tmp->data);
        gchar *padname = g_strdup_printf("src_%u", avspliter->npads);
        avspliter->npads += 1;
        GST_DEBUG_OBJECT(avspliter, "About to expose avspliter pad %s as %s", GST_OBJECT_NAME(avspliterPad), padname);
        gst_object_set_name(GST_OBJECT(avspliterPad), padname);
        g_free(padname);

        gst_pad_sticky_events_foreach(GST_PAD_CAST(avspliterPad), print_sticky_event, avspliterPad);

        if (!avspliterPad->exposed) {
            avspliterPad->exposed = TRUE;
            if (!gst_element_add_pad(GST_ELEMENT(avspliter), GST_PAD_CAST(avspliterPad))) {
                GST_WARNING_OBJECT(avspliter, "error adding pad to bin");
                avspliterPad->exposed = FALSE;
                continue;
            }
        }
        GST_INFO_OBJECT(avspliterPad, "added new avspliter pad");
    }

    for (GList *tmp = endpads; tmp != nullptr; tmp = tmp->next) {
        GstAVSpliterPad *avspliterPad = reinterpret_cast<GstAVSpliterPad *>(tmp->data);
        if (avspliterPad->exposed) {
            GST_DEBUG_OBJECT(avspliterPad, "unblocking");
            gst_avspliter_pad_set_block(avspliterPad, FALSE);
            GST_DEBUG_OBJECT(avspliterPad, "unblocking");
        }

        gst_object_unref(avspliterPad);
    }
}

/* call with dyn_lock held */
static void unblock_pads(GstAVSpliterBin *avspliter)
{
    GST_LOG_OBJECT(avspliter, "unblocking pads");

    for (GList *tmp = avspliter->blockedPads; tmp != nullptr; tmp = tmp->next) {
        if (tmp->data == nullptr) {
            continue;
        }
        GstAVSpliterPad *avspliterPad = GST_AVSPLITER_PAD_CAST(tmp->data);
        GST_DEBUG_OBJECT(avspliterPad, "unblocking");
        gst_avspliter_pad_do_set_block(avspliterPad, FALSE);
        GST_DEBUG_OBJECT(avspliterPad, "unblocked");
    }

    if (avspliter->blockedPads != nullptr) {
        GST_WARNING_OBJECT(avspliter, "the blocked pads list is not nullptr");
        g_list_free(avspliter->blockedPads);
    }
    avspliter->blockedPads = nullptr;
}

gboolean plugin_init(GstPlugin *plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_avspliter_bin_debug, "avspliterbin", 0, "avspliter bin");
    return gst_element_register(plugin, "avspliterbin", GST_RANK_NONE, GST_TYPE_AVSPLITER_BIN);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    _avspliter_bin,
    "Gstreamer AVSpliter Bin",
    plugin_init,
    PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)